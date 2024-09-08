#include "r_nri.h"
#include "stb_ds.h"
#include "r_resource_upload.h"
#include "../gameshared/q_math.h"

#include <assert.h>

#define NUMBER_COMMAND_SETS 3 

typedef struct {
  NriMemory* memory;
  NriBuffer* buffer;
} temporary_resource_buf_t;

typedef struct command_set_s{
  size_t offset;
  NriCommandAllocator* allocator;
  NriCommandBuffer* cmd;
  uint32_t reservedStageMemory;
  temporary_resource_buf_t* temporary;
} resource_command_set_t;

typedef struct resource_stage_buffer_s{
  NriMemory* memory;
  NriBuffer *buffer;
  size_t tailOffset;
  size_t remaningSpace;
  void *cpuMappedBuffer;
} resource_stage_buffer_t;

typedef struct {
  NriBuffer* backing;
  uint64_t byteOffset;
  void* cpuMapping;
} resource_stage_response_t;

static uint32_t syncIndex = 0;
static uint32_t activeSet = 0;
static resource_stage_buffer_t stageBuffer = {};
static resource_command_set_t commandSets[NUMBER_COMMAND_SETS] = {};
static NriCommandQueue* cmdQueue = NULL;
static NriFence* uploadFence = NULL;

static struct nri_backend_s* backend;

static bool __R_AllocFromStageBuffer(resource_command_set_t *set, size_t reqSize, resource_stage_response_t *res ) {
	size_t allocSize = ALIGN(reqSize, 4 ); // we round up to multiples of uint32_t
	if( allocSize > stageBuffer.remaningSpace ) {
		// we are out of avaliable space from staging
		return false;
	}

  // we are past the end of the buffer
	if( stageBuffer.tailOffset + allocSize > SizeOfStageBufferByte ) {
		const size_t remainingSpace = ( SizeOfStageBufferByte - stageBuffer.tailOffset ); // remaining space at the end of the buffer this unusable
		if( allocSize > stageBuffer.remaningSpace - remainingSpace ) {
			return false;
		}
		set->reservedStageMemory += remainingSpace; // give the reamining space to the requesting set 
		stageBuffer.tailOffset = 0;
	}

  res->byteOffset = stageBuffer.tailOffset;
  res->backing = stageBuffer.buffer;
  res->cpuMapping = ((uint8_t *)stageBuffer.cpuMappedBuffer ) + stageBuffer.tailOffset;

  stageBuffer.tailOffset += allocSize;
	set->reservedStageMemory += allocSize;
	stageBuffer.remaningSpace -= allocSize;
	return true;
}

static bool R_AllocTemporaryBuffer( resource_command_set_t *set, size_t reqSize, resource_stage_response_t *res )
{
  assert(res);
	if( __R_AllocFromStageBuffer( set, reqSize, res ) ) {
		return true;
	}
	Com_Printf( "Creating temporary buffer ran out space in staging" );
	temporary_resource_buf_t temp = {};
	NriBufferDesc bufferDesc = { .size = reqSize };
	NRI_ABORT_ON_FAILURE( backend->coreI.CreateBuffer( backend->device, &bufferDesc, &temp.buffer ) );

	struct NriMemoryDesc memoryDesc = {};
	backend->coreI.GetBufferMemoryDesc(backend->device, &bufferDesc, NriMemoryLocation_HOST_UPLOAD, &memoryDesc );

	NriAllocateMemoryDesc allocateMemoryDesc = {};
	allocateMemoryDesc.size = memoryDesc.size;
	allocateMemoryDesc.type = memoryDesc.type;
	NRI_ABORT_ON_FAILURE( backend->coreI.AllocateMemory( backend->device, &allocateMemoryDesc, &temp.memory ) );

	NriBufferMemoryBindingDesc bindBufferDesc = {
		.memory = temp.memory,
		.buffer = temp.buffer,
	};
	NRI_ABORT_ON_FAILURE( backend->coreI.BindBufferMemory( backend->device, &bindBufferDesc, 1 ) );
	arrput(set->temporary, temp );

	res->cpuMapping = backend->coreI.MapBuffer( temp.buffer, 0, NRI_WHOLE_SIZE );
	res->backing = temp.buffer;
	res->byteOffset = 0;
	return true;
}

static void R_UploadBeginCommandSet( uint32_t setIndex)
{
	struct command_set_s* set = &commandSets[activeSet];
  stageBuffer.remaningSpace += set->reservedStageMemory; 
  set->reservedStageMemory = 0;
	backend->coreI.BeginCommandBuffer( set->cmd, 0 );
}

static void R_UploadEndCommandSet(uint32_t setIndex) {
	struct command_set_s* set = &commandSets[activeSet];
	const NriCommandBuffer* cmdBuffers[] = {
    set->cmd
	};

	NriFenceSubmitDesc signalFence = {0};
	signalFence.fence = uploadFence;
	signalFence.value = 1 + syncIndex;

	backend->coreI.EndCommandBuffer( set->cmd );
	NriQueueSubmitDesc queueSubmit = {0};
	queueSubmit.commandBuffers = cmdBuffers;
	queueSubmit.commandBufferNum = 1;
	queueSubmit.signalFenceNum = 1;
	queueSubmit.signalFences = &signalFence;
	backend->coreI.QueueSubmit( cmdQueue, &queueSubmit );
  syncIndex++;
}

void R_InitResourceUpload(struct nri_backend_s* nri)
{
	assert(nri);
	backend = nri;

	NRI_ABORT_ON_FAILURE( backend->coreI.GetCommandQueue( backend->device, NriCommandQueueType_GRAPHICS, &cmdQueue ) )
	NriBufferDesc stageBufferDesc = { .size = SizeOfStageBufferByte };
	NRI_ABORT_ON_FAILURE( backend->coreI.CreateBuffer( backend->device, &stageBufferDesc, &stageBuffer.buffer ) )

	NriResourceGroupDesc resourceGroupDesc = { .buffers = &stageBuffer.buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
	assert( backend->helperI.CalculateAllocationNumber( backend->device, &resourceGroupDesc ) == 1 );
	NRI_ABORT_ON_FAILURE( backend->helperI.AllocateAndBindMemory( backend->device, &resourceGroupDesc, &stageBuffer.memory ) );
	NRI_ABORT_ON_FAILURE( backend->coreI.CreateFence( backend->device, 0, &uploadFence ) );

	for( size_t i = 0; i < Q_ARRAY_COUNT( commandSets ); i++ ) {
		NRI_ABORT_ON_FAILURE( backend->coreI.CreateCommandAllocator( cmdQueue, &commandSets[i].allocator ) );
		NRI_ABORT_ON_FAILURE( backend->coreI.CreateCommandBuffer( commandSets[i].allocator, &commandSets[i].cmd ) );
	}

	// we just keep the buffer always mapped
	stageBuffer.cpuMappedBuffer = backend->coreI.MapBuffer( stageBuffer.buffer, 0, NRI_WHOLE_SIZE );
	stageBuffer.tailOffset = 0;
	stageBuffer.remaningSpace = SizeOfStageBufferByte;
	activeSet = 0;
	R_UploadBeginCommandSet( 0 );
}

void R_ResourceBeginCopyBuffer( buffer_upload_desc_t *action )
{
	assert( action->target );
	resource_stage_response_t res = {};
	R_AllocTemporaryBuffer( &commandSets[activeSet], action->numBytes, &res );
	action->internal.byteOffset = res.byteOffset;
	action->data = res.cpuMapping;
	action->internal.backing = res.backing;
}

void R_ResourceEndCopyBuffer( buffer_upload_desc_t *action )
{
 // {
 //   NriBufferTransitionBarrierDesc bufferBarriers[] = { 
 //   	{ .buffer = action->internal.backing, 
 //   		.prevAccess = action->currentAccess, 
 //   		.nextAccess = NriAccessBits_COPY_DESTINATION } };
 //   NriTransitionBarrierDesc transitions = {};
 //   transitions.buffers = bufferBarriers;
 //   transitions.bufferNum = Q_ARRAY_COUNT( bufferBarriers );
 //   backend->coreI.CmdPipelineBarrier( commandSets[activeSet].cmd, &transitions, NULL, NriBarrierDependency_COPY_STAGE );
 // }
	
  NriBufferBarrierDesc transitionBarriers = {0};
  transitionBarriers.before = action->currentAccess;
  transitionBarriers.after.access = NriAccessBits_COPY_DESTINATION; 
  transitionBarriers.after.stages = NriStageBits_COPY; 
  transitionBarriers.buffer = action->target;//otextureTransitionBarrierDesc;
	
	NriBarrierGroupDesc barrierGroupDesc = {0};
	barrierGroupDesc.bufferNum = 1;
	barrierGroupDesc.buffers = &transitionBarriers;
  backend->coreI.CmdBarrier( commandSets[activeSet].cmd, &barrierGroupDesc);
}

void R_ResourceBeginCopyTexture( texture_upload_desc_t *desc )
{
	assert( desc->target );
	const NriDeviceDesc *deviceDesc = backend->coreI.GetDeviceDesc( backend->device );

	const uint64_t alignedRowPitch = ALIGN( desc->rowPitch, deviceDesc->uploadBufferTextureRowAlignment );
	const uint64_t alignedSlicePitch =  ALIGN( desc->sliceNum * alignedRowPitch, deviceDesc->uploadBufferTextureSliceAlignment );
	//const uint64_t contentSize = alignedSlicePitch * fmax( desc->sliceNum, 1u );
	
	desc->alignRowPitch = alignedRowPitch;
	desc->alignSlicePitch = alignedSlicePitch;

	resource_stage_response_t res = {};
	R_AllocTemporaryBuffer( &commandSets[activeSet], alignedSlicePitch, &res );
	desc->internal.byteOffset = res.byteOffset;
	desc->data = res.cpuMapping;
	desc->internal.backing = res.backing;
}

void R_ResourceEndCopyTexture( texture_upload_desc_t* desc) {
	const NriTextureDesc* textureDesc = backend->coreI.GetTextureDesc( desc->target);

  NriTextureBarrierDesc transitionBarriers = {0};
  transitionBarriers.before = desc->currentAccessAndLayout;
  transitionBarriers.after.layout = NriLayout_COPY_DESTINATION; 
  transitionBarriers.after.stages = NriStageBits_COPY; 
  transitionBarriers.texture = desc->target;//otextureTransitionBarrierDesc;

	NriBarrierGroupDesc barrierGroupDesc = {0};
	barrierGroupDesc.textureNum = 1;
	barrierGroupDesc.textures = &transitionBarriers;
  backend->coreI.CmdBarrier(commandSets[activeSet].cmd, &barrierGroupDesc);//, NULL, NriBarrierDependency_COPY_STAGE);
  
  NriTextureRegionDesc destRegionDesc = { 
  	.layerOffset = desc->arrayOffset, 
  	.mipOffset = desc->mipOffset,
  	.x = desc->x,
  	.y = desc->y,
  	.z = desc->z,
  	.width = desc->width, 
  	.height = desc->height, 
  	.depth = textureDesc->depth 
  };
  NriTextureDataLayoutDesc srcLayoutDesc = {
    .offset = desc->internal.byteOffset,
    .rowPitch = desc->alignRowPitch, 
    .slicePitch = desc->alignSlicePitch,
  };
  backend->coreI.CmdUploadBufferToTexture(commandSets[activeSet].cmd, desc->target, &destRegionDesc, desc->internal.backing, &srcLayoutDesc); 
}

void R_ResourceSubmit() {
  R_UploadEndCommandSet(activeSet);
  activeSet = ( activeSet + 1 ) % NUMBER_COMMAND_SETS;
	if( syncIndex >= NUMBER_COMMAND_SETS ) {
		struct command_set_s *set = &commandSets[activeSet];
		backend->coreI.Wait( uploadFence, 1 + syncIndex - NUMBER_COMMAND_SETS );
		backend->coreI.ResetCommandAllocator( set->allocator );
	   for(size_t i = 0; i < arrlen(set->temporary); i++) {
	     backend->coreI.DestroyBuffer(set->temporary[i].buffer);
	     backend->coreI.FreeMemory(set->temporary[i].memory);
	   }
	  arrsetlen(set->temporary, 0);
	}
  R_UploadBeginCommandSet(activeSet);
}




