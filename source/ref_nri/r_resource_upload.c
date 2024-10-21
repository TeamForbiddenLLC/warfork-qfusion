#include "r_resource_upload.h"
#include "../gameshared/q_math.h"
#include "r_nri.h"
#include "stb_ds.h"


#include <assert.h>

#include "r_local.h"

#define NUMBER_COMMAND_SETS 3

typedef struct {
	NriMemory *memory;
	NriBuffer *buffer;
} temporary_resource_buf_t;

typedef struct command_set_s {
	size_t offset;
	NriCommandAllocator *allocator;
	NriCommandBuffer *cmd;
	uint32_t reservedStageMemory;
	temporary_resource_buf_t *temporary;

	NriTexture** seenTextures;
	NriBuffer** seenBuffers;
} resource_command_set_t;

typedef struct resource_stage_buffer_s {
	NriMemory *memory;
	NriBuffer *buffer;
	size_t tailOffset;
	size_t remaningSpace;
	void *cpuMappedBuffer;
} resource_stage_buffer_t;

typedef struct {
	NriBuffer *backing;
	uint64_t byteOffset;
	void *cpuMapping;
} resource_stage_response_t;

static NriBufferBarrierDesc* transitionBuffers = NULL; // buffers that need to be transitioned back to frame buffer
static NriTextureBarrierDesc* transitionTextures = NULL; // textures that need to be transitioned back to frame buffer

static uint32_t syncIndex = 0;
static uint32_t activeSet = 0;
static resource_stage_buffer_t stageBuffer = {};
static resource_command_set_t commandSets[NUMBER_COMMAND_SETS] = {};
static NriCommandQueue *cmdQueue = NULL;
static NriFence *uploadFence = NULL;

static bool __R_AllocFromStageBuffer( resource_command_set_t *set, size_t reqSize, resource_stage_response_t *res )
{
	size_t allocSize = ALIGN( reqSize, 4 ); // we round up to multiples of uint32_t
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
		
		stageBuffer.remaningSpace -= remainingSpace;
		set->reservedStageMemory += remainingSpace; // give the remaning space to the requesting set
		stageBuffer.tailOffset = 0;
	}

	res->byteOffset = stageBuffer.tailOffset;
	res->backing = stageBuffer.buffer;
	res->cpuMapping = ( (uint8_t *)stageBuffer.cpuMappedBuffer ) + stageBuffer.tailOffset;

	set->reservedStageMemory += allocSize;

	stageBuffer.tailOffset += allocSize;
	stageBuffer.remaningSpace -= allocSize;
	return true;
}

static bool R_AllocTemporaryBuffer( resource_command_set_t *set, size_t reqSize, resource_stage_response_t *res )
{
	assert( res );
	if( __R_AllocFromStageBuffer( set, reqSize, res ) ) {
		return true;
	}
	// Com_Printf( "Creating temporary buffer ran out space in staging" );
	temporary_resource_buf_t temp = {};
	NriBufferDesc bufferDesc = { .size = reqSize };
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &bufferDesc, &temp.buffer ) );

	struct NriMemoryDesc memoryDesc = {};
	rsh.nri.coreI.GetBufferMemoryDesc( rsh.nri.device, &bufferDesc, NriMemoryLocation_HOST_UPLOAD, &memoryDesc );

	NriAllocateMemoryDesc allocateMemoryDesc = {};
	allocateMemoryDesc.size = memoryDesc.size;
	allocateMemoryDesc.type = memoryDesc.type;
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.AllocateMemory( rsh.nri.device, &allocateMemoryDesc, &temp.memory ) );

	NriBufferMemoryBindingDesc bindBufferDesc = {
		.memory = temp.memory,
		.buffer = temp.buffer,
	};
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.BindBufferMemory( rsh.nri.device, &bindBufferDesc, 1 ) );
	arrput( set->temporary, temp );

	res->cpuMapping = rsh.nri.coreI.MapBuffer( temp.buffer, 0, NRI_WHOLE_SIZE );
	res->backing = temp.buffer;
	res->byteOffset = 0;
	return true;
}

static void R_UploadBeginCommandSet( uint32_t setIndex )
{
	struct command_set_s *set = &commandSets[activeSet];
	stageBuffer.remaningSpace += set->reservedStageMemory;
	set->reservedStageMemory = 0;
	rsh.nri.coreI.BeginCommandBuffer( set->cmd, 0 );
}

static void R_UploadEndCommandSet( uint32_t setIndex )
{
	struct command_set_s *set = &commandSets[activeSet];
	const NriCommandBuffer *cmdBuffers[] = { set->cmd };

	NriFenceSubmitDesc signalFence = { 0 };
	signalFence.fence = uploadFence;
	signalFence.value = 1 + syncIndex;

	rsh.nri.coreI.EndCommandBuffer( set->cmd );
	NriQueueSubmitDesc queueSubmit = { 0 };
	queueSubmit.commandBuffers = cmdBuffers;
	queueSubmit.commandBufferNum = 1;
	queueSubmit.signalFenceNum = 1;
	queueSubmit.signalFences = &signalFence;
	rsh.nri.coreI.QueueSubmit( cmdQueue, &queueSubmit );
	syncIndex++;
}

void R_InitResourceUpload()
{
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.GetCommandQueue( rsh.nri.device, NriCommandQueueType_GRAPHICS, &cmdQueue ) )
	NriBufferDesc stageBufferDesc = { .size = SizeOfStageBufferByte };
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &stageBufferDesc, &stageBuffer.buffer ) )

	NriResourceGroupDesc resourceGroupDesc = { .buffers = &stageBuffer.buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
	assert( rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc ) == 1 );
	NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &stageBuffer.memory ) );
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateFence( rsh.nri.device, 0, &uploadFence ) );

	for( size_t i = 0; i < Q_ARRAY_COUNT( commandSets ); i++ ) {
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateCommandAllocator( cmdQueue, &commandSets[i].allocator ) );
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateCommandBuffer( commandSets[i].allocator, &commandSets[i].cmd ) );
	}

	// we just keep the buffer always mapped
	stageBuffer.cpuMappedBuffer = rsh.nri.coreI.MapBuffer( stageBuffer.buffer, 0, NRI_WHOLE_SIZE );
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
	action->internal.backing = res.backing;
	action->data = res.cpuMapping;
}

void R_ResourceEndCopyBuffer( buffer_upload_desc_t *action )
{
	const NriBufferDesc* bufferDesc = rsh.nri.coreI.GetBufferDesc( action->target);

	bool seen = false;
	for(size_t i = 0; i < arrlen(commandSets[activeSet].seenBuffers); i++) {
		if(commandSets[activeSet].seenBuffers[i] == action->target) {
			seen = true;
			break;
		}
	}
	if(!seen) {
		NriBufferBarrierDesc transitionBarriers = {0};
		transitionBarriers.buffer = action->target;
		transitionBarriers.before = action->before;
		transitionBarriers.after = (NriAccessStage){
			.access = NriAccessBits_COPY_DESTINATION,
			.stages = NriStageBits_COPY
		};

		if(bufferDesc->usage & NriBufferUsageBits_INDEX_BUFFER) {
			transitionBarriers.after.access |= NriAccessBits_INDEX_BUFFER;
			transitionBarriers.after.stages |= NriStageBits_INDEX_INPUT;
		}
		if(bufferDesc->usage & NriBufferUsageBits_VERTEX_BUFFER) {
			transitionBarriers.after.access |= NriAccessBits_VERTEX_BUFFER;
			transitionBarriers.after.stages |= NriStageBits_VERTEX_SHADER;
		}
		
		NriBarrierGroupDesc barrierGroupDesc = { 0 };
		barrierGroupDesc.bufferNum = 1;
		barrierGroupDesc.buffers = &transitionBarriers;
		rsh.nri.coreI.CmdBarrier( commandSets[activeSet].cmd, &barrierGroupDesc );

		NriBufferBarrierDesc postTransition = {
			.before = transitionBarriers.after,
			.after = action->after,
			.buffer = action->target
		};
		arrput(transitionBuffers, postTransition);
		arrput(commandSets[activeSet].seenBuffers, action->target);
	}

	rsh.nri.coreI.CmdCopyBuffer( commandSets[activeSet].cmd, action->target, action->byteOffset, action->internal.backing, action->internal.byteOffset, action->numBytes );
}

void R_ResourceBeginCopyTexture( texture_upload_desc_t *desc )
{
	assert( desc->target );
	const NriDeviceDesc *deviceDesc = rsh.nri.coreI.GetDeviceDesc( rsh.nri.device );

	const uint64_t alignedRowPitch = ALIGN( desc->rowPitch, deviceDesc->uploadBufferTextureRowAlignment );
	const uint64_t alignedSlicePitch = ALIGN( desc->sliceNum * alignedRowPitch, deviceDesc->uploadBufferTextureSliceAlignment );

	desc->alignRowPitch = alignedRowPitch;
	desc->alignSlicePitch = alignedSlicePitch;

	resource_stage_response_t res = {};
	R_AllocTemporaryBuffer( &commandSets[activeSet], alignedSlicePitch, &res );

	desc->internal.byteOffset = res.byteOffset;
	desc->data = res.cpuMapping;
	desc->internal.backing = res.backing;
}

void R_ResourceEndCopyTexture( texture_upload_desc_t *desc )
{
	const NriTextureDesc *textureDesc = rsh.nri.coreI.GetTextureDesc( desc->target );

	bool seen = false;
	for(size_t i = 0; i < arrlen(commandSets[activeSet].seenTextures); i++) {
		if(commandSets[activeSet].seenTextures[i] == desc->target) {
			seen = true;
			break;
		}
	}
	
	if(!seen) {
		NriTextureBarrierDesc transitionBarriers = { 0 };
		transitionBarriers.before = desc->before;
		transitionBarriers.texture = desc->target;
		transitionBarriers.after = (NriAccessLayoutStage){	
			.layout = NriLayout_COPY_DESTINATION, 
			.access = NriAccessBits_COPY_DESTINATION, 
			.stages = NriStageBits_COPY 
		};

		NriBarrierGroupDesc barrierGroupDesc = { 0 };
		barrierGroupDesc.textureNum = 1;
		barrierGroupDesc.textures = &transitionBarriers;
		rsh.nri.coreI.CmdBarrier( commandSets[activeSet].cmd, &barrierGroupDesc );

		const NriTextureBarrierDesc postTransition = {
			.before = transitionBarriers.after ,
			.after = desc->after,
			.texture = desc->target
		};
		arrpush(transitionTextures, postTransition );
		arrpush(commandSets[activeSet].seenTextures, desc->target);
	}
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

	rsh.nri.coreI.CmdUploadBufferToTexture( commandSets[activeSet].cmd, desc->target, &destRegionDesc, desc->internal.backing, &srcLayoutDesc );
}

// void R_ResourceTransitionTexture( NriTexture *texture, NriAccessLayoutStage currentAccessAndLayout )
// {
// 	NriTextureBarrierDesc transitionBarriers = { 0 };
// 	transitionBarriers.before = currentAccessAndLayout;
// 	transitionBarriers.after = ResourceUploadTexturePostAccess;
// 	transitionBarriers.texture = texture;

// 	NriBarrierGroupDesc barrierGroupDesc = { 0 };
// 	barrierGroupDesc.textureNum = 1;
// 	barrierGroupDesc.textures = &transitionBarriers;
// 	rsh.nri.coreI.CmdBarrier( commandSets[activeSet].cmd, &barrierGroupDesc );
// }

void R_ResourceSubmit()
{
	arrsetlen(commandSets[activeSet].seenBuffers, 0);
	arrsetlen(commandSets[activeSet].seenTextures, 0);

	R_UploadEndCommandSet( activeSet );
	activeSet = ( activeSet + 1 ) % NUMBER_COMMAND_SETS;
	if( syncIndex >= NUMBER_COMMAND_SETS ) {
		struct command_set_s *set = &commandSets[activeSet];
		rsh.nri.coreI.Wait( uploadFence, 1 + syncIndex - NUMBER_COMMAND_SETS );
		rsh.nri.coreI.ResetCommandAllocator( set->allocator );
		for( size_t i = 0; i < arrlen( set->temporary ); i++ ) {
			rsh.nri.coreI.DestroyBuffer( set->temporary[i].buffer );
			rsh.nri.coreI.FreeMemory( set->temporary[i].memory );
		}
		arrsetlen( set->temporary, 0 );
	}
	R_UploadBeginCommandSet( activeSet );
}

void R_FlushTransitionBarrier(NriCommandBuffer* cmd) {
	// size_t numTextureBarrier = 0;
	// NriTextureBarrierDesc textureBarriers[128] = {}; 
	// for(size_t i = 0; i < arrlen(transitionTextures); i++) {

	// 	NriAccessLayoutStage textureShaderAccess = {
	// 		.layout = NriLayout_SHADER_RESOURCE,
	// 		.access = NriAccessBits_SHADER_RESOURCE,
	// 		.stages = NriStageBits_FRAGMENT_SHADER 
	// 	};

	// 	textureBarriers[numTextureBarrier++] = (NriTextureBarrierDesc){
	// 		.texture = transitionTextures[i],
	// 		.before = ResourceUploadTexturePostAccess,
	// 		.after = textureShaderAccess
	// 	};
	// 	if(numTextureBarrier >= Q_ARRAY_COUNT(textureBarriers)) {
	// 		NriBarrierGroupDesc barrierGroupDesc = {0};
	// 		barrierGroupDesc.textureNum = numTextureBarrier;
	// 		barrierGroupDesc.textures = textureBarriers;
	// 		rsh.nri.coreI.CmdBarrier(cmd, &barrierGroupDesc);
	// 		numTextureBarrier = 0;
	// 	}
	// }
	NriBarrierGroupDesc barrierGroupDesc = {0};
	barrierGroupDesc.textureNum = arrlen(transitionTextures);
	barrierGroupDesc.textures = transitionTextures;
	barrierGroupDesc.bufferNum = arrlen(transitionBuffers);
	barrierGroupDesc.buffers = transitionBuffers;
	rsh.nri.coreI.CmdBarrier(cmd, &barrierGroupDesc);
	arrsetlen(transitionBuffers, 0);
	arrsetlen(transitionTextures, 0);

	// if(numTextureBarrier >= 0) {
	// }
	//
}
