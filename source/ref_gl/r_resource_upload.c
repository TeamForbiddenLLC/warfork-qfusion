#include "r_nriimp.h"
#include "r_renderer.h"
#include <assert.h>
#include "stb_ds.h"
#include "r_local.h"

#include "r_resource_upload.h"

#define NUMBER_COMMAND_SETS 3 

typedef struct {
  NriMemory* memory;
  NriBuffer* buffer;
} temporary_resource_buf_t;

typedef struct command_set_s{
  size_t offset;
  NriCommandAllocator* allocator;
  NriCommandBuffer* cmd;
  NriFence* fence;
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

static bool __R_AllocFromStageBuffer(resource_command_set_t *set, size_t reqSize, resource_stage_response_t *res ) {
	size_t allocSize = R_Align( reqSize, 4 ); // we round up to multiples of uint32_t
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
	NRI_ABORT_ON_FAILURE( r_nri.coreI.CreateBuffer( r_nri.device, &bufferDesc, &temp.buffer ) );

	NriMemoryDesc memoryDesc = {};
	r_nri.coreI.GetBufferMemoryInfo( temp.buffer, NriMemoryLocation_HOST_UPLOAD, &memoryDesc );
	NRI_ABORT_ON_FAILURE( r_nri.coreI.AllocateMemory( r_nri.device, NRI_ALL_NODES, memoryDesc.type, memoryDesc.size, &temp.memory ) );

	NriBufferMemoryBindingDesc bindBufferDesc = {
		.memory = temp.memory,
		.buffer = temp.buffer,
	};
	NRI_ABORT_ON_FAILURE( r_nri.coreI.BindBufferMemory( r_nri.device, &bindBufferDesc, 1 ) );
	arrput(set->temporary, temp );

  	res->cpuMapping = r_nri.coreI.MapBuffer(temp.buffer, 0, NRI_WHOLE_SIZE);
	res->backing = temp.buffer;
	res->byteOffset = 0;
	return true;
}

static void R_UploadBeginCommandSet( struct command_set_s *set )
{

	if( syncIndex >= NUMBER_COMMAND_SETS ) {
		r_nri.coreI.Wait( set->fence, 1 + syncIndex - NUMBER_COMMAND_SETS );
		r_nri.coreI.ResetCommandAllocator( set->allocator );
	   for(size_t i = 0; i < arrlen(set->temporary); i++) {
	     r_nri.coreI.DestroyBuffer(set->temporary[i].buffer);
	     r_nri.coreI.FreeMemory(set->temporary[i].memory);
	   }
	  arrsetlen(set->temporary, 0);
	}
  stageBuffer.remaningSpace += set->reservedStageMemory; 
  set->reservedStageMemory = 0;
	r_nri.coreI.BeginCommandBuffer( set->cmd, NULL, 0 );
}

static void R_UploadEndCommandSet(struct command_set_s* set) {
	const NriCommandBuffer* cmdBuffers[] = {
    set->cmd
	};
	r_nri.coreI.EndCommandBuffer( set->cmd );
	NriQueueSubmitDesc queueSubmit = {};
	queueSubmit.commandBuffers = cmdBuffers;
	queueSubmit.commandBufferNum = 1;
	r_nri.coreI.QueueSubmit( cmdQueue, &queueSubmit );
	r_nri.coreI.QueueSignal( cmdQueue, set->fence, 1 + syncIndex );
  syncIndex++;
}

void R_InitResourceUpload()
{
 NRI_ABORT_ON_FAILURE(r_nri.coreI.GetCommandQueue( r_nri.device, NriCommandQueueType_GRAPHICS, &cmdQueue))
 NriBufferDesc stageBufferDesc = { .size = SizeOfStageBufferByte };
 NRI_ABORT_ON_FAILURE( r_nri.coreI.CreateBuffer( r_nri.device, &stageBufferDesc, &stageBuffer.buffer ) )

  NriResourceGroupDesc resourceGroupDesc = { 
    .buffers = &stageBuffer.buffer, 
    .bufferNum = 1, 
    .memoryLocation = NriMemoryLocation_HOST_UPLOAD
  };
  assert(r_nri.helperI.CalculateAllocationNumber( r_nri.device, &resourceGroupDesc ) == 1 );
  NRI_ABORT_ON_FAILURE( r_nri.helperI.AllocateAndBindMemory( r_nri.device, &resourceGroupDesc, &stageBuffer.memory ) );

  for(size_t i = 0; i < Q_ARRAY_COUNT(commandSets); i++) {
    NRI_ABORT_ON_FAILURE( r_nri.coreI.CreateCommandAllocator(cmdQueue, &commandSets[i].allocator) );
    NRI_ABORT_ON_FAILURE( r_nri.coreI.CreateCommandBuffer(commandSets[i].allocator, &commandSets[i].cmd) );
  }

  // we just keep the buffer always mapped
  stageBuffer.cpuMappedBuffer = r_nri.coreI.MapBuffer(stageBuffer.buffer, 0, NRI_WHOLE_SIZE);
  stageBuffer.tailOffset = 0;
  stageBuffer.remaningSpace = SizeOfStageBufferByte;
  activeSet = 0;
  R_UploadBeginCommandSet( &commandSets[0] );
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
	{
		NriBufferTransitionBarrierDesc bufferBarriers[] = { 
			{ .buffer = action->internal.backing, 
				.prevAccess = action->currentAccess, 
				.nextAccess = NriAccessBits_COPY_DESTINATION } };
		NriTransitionBarrierDesc transitions = {};
		transitions.buffers = bufferBarriers;
		transitions.bufferNum = Q_ARRAY_COUNT( bufferBarriers );
		r_nri.coreI.CmdPipelineBarrier( commandSets[activeSet].cmd, &transitions, NULL, NriBarrierDependency_COPY_STAGE );
	}
	r_nri.coreI.CmdCopyBuffer( commandSets[activeSet].cmd, action->target, 0, action->byteOffset, action->internal.backing, 0, action->internal.byteOffset, action->numBytes );
}

void R_ResourceBeginCopyTexture( texture_upload_desc_t *desc )
{
	assert( desc->target );
	const NriDeviceDesc *deviceDesc = r_nri.coreI.GetDeviceDesc( r_nri.device );

	const uint32_t sliceRowNum = max( desc->slicePitch / desc->rowPitch, 1u );
	const uint64_t alignedRowPitch = R_Align( desc->rowPitch, deviceDesc->uploadBufferTextureRowAlignment );
	const uint64_t alignedSlicePitch = R_Align( sliceRowNum * alignedRowPitch, deviceDesc->uploadBufferTextureSliceAlignment );
	const uint64_t contentSize = alignedSlicePitch * fmax( desc->sliceNum, 1u );
	
	desc->alignRowPitch = alignedRowPitch ;
	desc->alignSlicePitch = alignedSlicePitch;

	resource_stage_response_t res = {};
	R_AllocTemporaryBuffer( &commandSets[activeSet], contentSize, &res );
	desc->internal.byteOffset = res.byteOffset;
	desc->data = res.cpuMapping;
	desc->internal.backing = res.backing;
}

void R_ResourceEndCopyTexture( texture_upload_desc_t* desc) {
	const NriTextureDesc* textureDesc = r_nri.coreI.GetTextureDesc( desc->target);
 
  NriTextureTransitionBarrierDesc textureTransitionBarrierDesc = {};
  textureTransitionBarrierDesc.texture = desc->target;
  textureTransitionBarrierDesc.prevState = desc->currentAccessAndLayout;
  textureTransitionBarrierDesc.nextState.layout = NriTextureLayout_COPY_DESTINATION; 
  textureTransitionBarrierDesc.nextState.acessBits = NriAccessBits_COPY_DESTINATION; 
  textureTransitionBarrierDesc.arraySize = desc->arrayOffset;
  textureTransitionBarrierDesc.mipNum = desc->mipOffset;

  NriTransitionBarrierDesc transitionBarriers = {};
  transitionBarriers.textureNum = 1;
  transitionBarriers.textures = &textureTransitionBarrierDesc;
  r_nri.coreI.CmdPipelineBarrier(commandSets[activeSet].cmd, &transitionBarriers, NULL, NriBarrierDependency_COPY_STAGE);
  
  NriTextureRegionDesc destRegionDesc = { .arrayOffset = desc->arrayOffset, .mipOffset = desc->mipOffset, .width = textureDesc->width, .height = textureDesc->height, .depth = textureDesc->depth };
  NriTextureDataLayoutDesc srcLayoutDesc = {
    .offset = desc->internal.byteOffset,
    .rowPitch = desc->alignRowPitch, 
    .slicePitch = desc->alignSlicePitch,
  };
  r_nri.coreI.CmdUploadBufferToTexture(commandSets[activeSet].cmd, desc->target, &destRegionDesc, desc->internal.backing, &srcLayoutDesc); 
}

void R_ResourceSubmit() {
  R_UploadEndCommandSet(&commandSets[activeSet]);
  activeSet = (activeSet +1) % NUMBER_COMMAND_SETS;
  R_UploadEndCommandSet(&commandSets[activeSet]);
}



