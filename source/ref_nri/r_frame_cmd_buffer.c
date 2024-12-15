#include "r_frame_cmd_buffer.h"

#include "r_local.h"
#include "r_nri.h"
#include "r_resource.h"

#include "r_model.h"

#include "stb_ds.h"
#include "qhash.h"

void FR_CmdResetAttachmentToBackbuffer( struct frame_cmd_buffer_s *cmd )
{
	const NriTextureDesc *colorDesc = rsh.nri.coreI.GetTextureDesc( cmd->textureBuffers.colorTexture );
	const NriTextureDesc *depthDesc = rsh.nri.coreI.GetTextureDesc( cmd->textureBuffers.depthTexture );

	const NriDescriptor *colorAttachments[] = { cmd->textureBuffers.colorAttachment };
	const struct NriViewport viewports[] = {
		( NriViewport ){ 
			.x = 0, 
			.y = 0, 
			.width = cmd->textureBuffers.screen.width, 
			.height = cmd->textureBuffers.screen.height, 
			.depthMin = 0.0f, 
			.depthMax = 1.0f 
		} 
	};
	const struct NriRect scissors[] = { cmd->textureBuffers.screen };
	const NriFormat colorFormats[] = { colorDesc->format };

	FR_CmdSetTextureAttachment( cmd, colorFormats, colorAttachments, viewports, scissors, 1, depthDesc->format, cmd->textureBuffers.depthAttachment );
}

void FR_CmdResetCmdState( struct frame_cmd_buffer_s *cmd, enum CmdStateResetBits bits )
{
	memset( cmd->state.vertexBuffers, 0, sizeof( NriBuffer * ) * MAX_VERTEX_BINDINGS );
	cmd->state.dirtyVertexBuffers = 0;
}

void FR_CmdSetScissor( struct frame_cmd_buffer_s *cmd, const NriRect *scissors, size_t numAttachments )
{
	assert( numAttachments < MAX_COLOR_ATTACHMENTS );
	memcpy( cmd->state.scissors, scissors, sizeof( NriRect ) * numAttachments );
	cmd->state.dirty |= CMD_DIRT_SCISSOR;
}

void FR_CmdSetScissorAll( struct frame_cmd_buffer_s *cmd, const NriRect scissors )
{
	for( size_t i = 0; i < FR_CmdNumViewports(cmd); i++ ) {
		assert( scissors.x >= 0 && scissors.y >= 0 );
		cmd->state.scissors[i] = scissors;
	}
	cmd->state.dirty |= CMD_DIRT_SCISSOR;
}

void FR_CmdSetViewportAll( struct frame_cmd_buffer_s *cmd, const NriViewport viewport ) {
	for( size_t i = 0; i < FR_CmdNumViewports(cmd); i++ ) {
		cmd->state.viewports[i] = viewport;
	}
	cmd->state.dirty |= CMD_DIRT_VIEWPORT;

}

void FR_CmdSetTextureAttachment( struct frame_cmd_buffer_s *cmd,
								 const NriFormat *colorformats,
								 const NriDescriptor **colorAttachments,
								 const NriViewport *viewports,
								 const NriRect *scissors,
								 size_t numAttachments,
								 const NriFormat depthFormat,
								 NriDescriptor *depthAttachment )
{
	assert(cmd->stackCmdBeingRendered == 0);
	assert( numAttachments < MAX_COLOR_ATTACHMENTS );
	assert( viewports );
	assert( scissors );
	cmd->state.pipelineLayout.depthFormat = depthFormat;
	cmd->state.numColorAttachments = numAttachments;
	memcpy(cmd->state.pipelineLayout.colorFormats, colorformats, sizeof(NriFormat) * numAttachments);
	memcpy( cmd->state.colorAttachment, colorAttachments, sizeof( struct NriDescriptor * ) * numAttachments );
	memcpy( cmd->state.viewports, viewports, sizeof( NriViewport ) * FR_CmdNumViewports(cmd));
	memcpy( cmd->state.scissors, scissors, sizeof( NriRect ) * FR_CmdNumViewports(cmd) );
	cmd->state.depthAttachment = depthAttachment;
	
	cmd->state.dirty |= (CMD_DIRT_SCISSOR | CMD_DIRT_VIEWPORT);
}

void FR_CmdSetIndexBuffer( struct frame_cmd_buffer_s *cmd, NriBuffer *buffer, uint64_t offset, NriIndexType indexType )
{
	cmd->state.dirty |= CMD_DIRT_INDEX_BUFFER;
	cmd->state.indexType = indexType;
	cmd->state.indexBufferOffset = offset;
	cmd->state.indexBuffer = buffer;
}

void FR_CmdSetVertexBuffer( struct frame_cmd_buffer_s *cmd, uint32_t slot, NriBuffer *buffer, uint64_t offset )
{
	assert( slot < MAX_VERTEX_BINDINGS );
	cmd->state.dirtyVertexBuffers |= ( 0x1 << slot );
	cmd->state.offsets[slot] = offset;
	cmd->state.vertexBuffers[slot] = buffer;
}

void FR_CmdResetCommandState( struct frame_cmd_buffer_s *cmd, enum CmdResetBits bits)
{
	if(bits & CMD_RESET_INDEX_BUFFER) {
		cmd->state.dirty &= ~CMD_DIRT_INDEX_BUFFER;
	}
	if(bits & CMD_RESET_VERTEX_BUFFER) {
		cmd->state.dirtyVertexBuffers = 0;
	}
	if(bits & CMD_RESET_DEFAULT_PIPELINE_LAYOUT ) {
		cmd->state.pipelineLayout.flippedViewport = false;
		cmd->state.pipelineLayout.depthWrite = false;
		cmd->state.pipelineLayout.blendEnabled = false;
		cmd->state.pipelineLayout.cullMode = NriCullMode_FRONT;
		cmd->state.pipelineLayout.depthBias = (NriDepthBiasDesc){0};
		cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_ONE;
		cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_ZERO;
		cmd->state.pipelineLayout.colorWriteMask = NriColorWriteBits_RGB;
		cmd->state.pipelineLayout.compareFunc= NriCompareFunc_ALWAYS;
	}
	
}

void UpdateFrameUBO( struct frame_cmd_buffer_s *cmd, struct ubo_frame_instance_s *ubo, void *data, size_t size )
{
	const hash_t hash = hash_data( HASH_INITIAL_VALUE, data, size );
	if( ubo->hash != hash ) {
		struct block_buffer_pool_req_s poolReq = BlockBufferPoolReq( &rsh.nri, &cmd->uboBlockBuffer, size );
		void *mem = rsh.nri.coreI.MapBuffer( poolReq.buffer, poolReq.bufferOffset, poolReq.bufferSize );
		memcpy( mem, data, size );
		rsh.nri.coreI.UnmapBuffer( poolReq.buffer );

		NriBufferViewDesc bufferDesc = { .buffer = poolReq.buffer, .size = poolReq.bufferSize, .offset = poolReq.bufferOffset, .viewType = NriBufferViewType_CONSTANT };

		NriDescriptor *descriptor = NULL;
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBufferView( &bufferDesc, &descriptor ) );
		arrpush( cmd->frameTemporaryDesc, descriptor );
		ubo->descriptor = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
		ubo->hash = hash;
		ubo->req = poolReq;
	}
}

void R_CmdState_RestoreAttachment(struct frame_cmd_buffer_s* cmd, const struct frame_cmd_save_attachment_s* save) {
	assert(cmd->stackCmdBeingRendered == 0);
	cmd->state.numColorAttachments = save->numColorAttachments;
	memcpy(cmd->state.pipelineLayout.colorFormats, save->colorFormats, sizeof(NriFormat) * cmd->state.numColorAttachments);
	memcpy(cmd->state.colorAttachment, save->colorAttachment, sizeof(NriDescriptor*) * cmd->state.numColorAttachments);
	memcpy(cmd->state.scissors, save->scissors, sizeof(NriRect) * FR_CmdNumViewports(cmd));
	memcpy(cmd->state.viewports, save->viewports, sizeof(NriViewport) * FR_CmdNumViewports(cmd));
	cmd->state.depthAttachment = save->depthAttachment;
	cmd->state.dirty |= (CMD_DIRT_SCISSOR | CMD_DIRT_VIEWPORT);
}


struct frame_cmd_save_attachment_s R_CmdState_StashAttachment(struct frame_cmd_buffer_s* cmd) {
	struct frame_cmd_save_attachment_s save;
	save.numColorAttachments = cmd->state.numColorAttachments;
	memcpy(save.colorFormats, cmd->state.pipelineLayout.colorFormats, sizeof(NriFormat) * cmd->state.numColorAttachments);
	memcpy(save.colorAttachment, cmd->state.colorAttachment, sizeof(NriDescriptor*) * cmd->state.numColorAttachments);
	memcpy(save.scissors, cmd->state.scissors, sizeof(NriRect) * FR_CmdNumViewports(cmd));
	memcpy(save.viewports, cmd->state.viewports, sizeof(NriViewport) * FR_CmdNumViewports(cmd));
	save.depthAttachment = cmd->state.depthAttachment;
	return save;
}


void FrameCmdBufferFree(struct frame_cmd_buffer_s* cmd) {
	for( size_t i = 0; i < arrlen( cmd->freeTextures ); i++ ) {
		rsh.nri.coreI.DestroyTexture( cmd->freeTextures[i] );
	}
	for( size_t i = 0; i < arrlen( cmd->freeBuffers ); i++ ) {
		rsh.nri.coreI.DestroyBuffer( cmd->freeBuffers[i] );
	}
	for( size_t i = 0; i < arrlen( cmd->freeMemory ); i++ ) {
		rsh.nri.coreI.FreeMemory( cmd->freeMemory[i] );
	}
	for( size_t i = 0; i < arrlen( cmd->frameTemporaryDesc ); i++ ) {
		rsh.nri.coreI.DestroyDescriptor( cmd->frameTemporaryDesc[i] );
	}
	arrfree(cmd->freeMemory);
	arrfree(cmd->freeTextures);
	arrfree(cmd->freeBuffers);
	arrfree(cmd->frameTemporaryDesc);
	BlockBufferPoolFree( &rsh.nri, &cmd->uboBlockBuffer );
	
	memset( &cmd->uboSceneFrame, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboSceneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboPassObject, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboBoneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboLight, 0, sizeof( struct ubo_frame_instance_s ) );

	rsh.nri.coreI.DestroyCommandBuffer(cmd->cmd);
	rsh.nri.coreI.DestroyCommandAllocator(cmd->allocator);

}
void ResetFrameCmdBuffer( struct nri_backend_s *backend, struct frame_cmd_buffer_s *cmd )
{
	cmd->textureBuffers = rsh.backBuffers[rsh.nri.swapChainI.AcquireNextSwapChainTexture( rsh.swapchain )];
	
	for( size_t i = 0; i < arrlen( cmd->freeTextures ); i++ ) {
		rsh.nri.coreI.DestroyTexture( cmd->freeTextures[i] );
	}
	for( size_t i = 0; i < arrlen( cmd->freeBuffers ); i++ ) {
		rsh.nri.coreI.DestroyBuffer( cmd->freeBuffers[i] );
	}
	for( size_t i = 0; i < arrlen( cmd->freeMemory ); i++ ) {
		rsh.nri.coreI.FreeMemory( cmd->freeMemory[i] );
	}
	for( size_t i = 0; i < arrlen( cmd->frameTemporaryDesc ); i++ ) {
		backend->coreI.DestroyDescriptor( cmd->frameTemporaryDesc[i] );
	}
	arrsetlen( cmd->freeMemory, 0 );
	arrsetlen( cmd->freeTextures, 0 );
	arrsetlen( cmd->freeBuffers, 0 );
	arrsetlen( cmd->frameTemporaryDesc, 0);
	BlockBufferPoolReset( &cmd->uboBlockBuffer );

	memset( &cmd->uboSceneFrame, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboSceneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboPassObject, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboBoneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	memset( &cmd->uboLight, 0, sizeof( struct ubo_frame_instance_s ) );
}

void FR_CmdDraw( struct frame_cmd_buffer_s *cmd, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance) {
	if(cmd->state.dirty & CMD_DIRT_VIEWPORT) {
		rsh.nri.coreI.CmdSetViewports( cmd->cmd, cmd->state.viewports, FR_CmdNumViewports(cmd));
		cmd->state.dirty &= ~CMD_DIRT_VIEWPORT;
	}

	if(cmd->state.dirty & CMD_DIRT_SCISSOR) {
		rsh.nri.coreI.CmdSetScissors( cmd->cmd, cmd->state.scissors, FR_CmdNumViewports(cmd));
		cmd->state.dirty &= ~CMD_DIRT_SCISSOR;
	}
	NriDrawDesc drawDesc = { 0 };
	drawDesc.vertexNum = vertexNum;
	drawDesc.instanceNum = max( 1, instanceNum );
	drawDesc.baseVertex = baseVertex;
	drawDesc.baseInstance = baseInstance;
	rsh.nri.coreI.CmdDraw( cmd->cmd, &drawDesc );
}

void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance )
{
	uint32_t vertexSlot = 0;
	for( uint32_t attr = cmd->state.dirtyVertexBuffers; attr > 0; attr = ( attr >> 1 ), vertexSlot++ ) {
		if( cmd->state.dirtyVertexBuffers & ( 1 << vertexSlot ) ) {
			const NriBuffer *buffer[1] = { cmd->state.vertexBuffers[vertexSlot] };
			const uint64_t offset[1] = { cmd->state.offsets[vertexSlot] };
			rsh.nri.coreI.CmdSetVertexBuffers( cmd->cmd, vertexSlot, 1, buffer, offset );
		}
	}

	if( cmd->state.dirty & CMD_DIRT_INDEX_BUFFER ) {
		rsh.nri.coreI.CmdSetIndexBuffer( cmd->cmd, cmd->state.indexBuffer, cmd->state.indexBufferOffset, cmd->state.indexType );
	}

	if(cmd->state.dirty & CMD_DIRT_VIEWPORT) {
		rsh.nri.coreI.CmdSetViewports( cmd->cmd, cmd->state.viewports, FR_CmdNumViewports(cmd) );
		cmd->state.dirty &= ~CMD_DIRT_VIEWPORT;
	}

	if(cmd->state.dirty & CMD_DIRT_SCISSOR) {
		rsh.nri.coreI.CmdSetScissors( cmd->cmd, cmd->state.scissors, FR_CmdNumViewports(cmd));
		cmd->state.dirty &= ~CMD_DIRT_SCISSOR;
	}
	
	NriDrawIndexedDesc drawDesc = { 0 };
	drawDesc.indexNum = indexNum;
	drawDesc.instanceNum = max( 1, instanceNum );
	drawDesc.baseIndex = baseIndex;
	drawDesc.baseVertex = baseVertex;
	drawDesc.baseInstance = baseInstance;
	rsh.nri.coreI.CmdDrawIndexed( cmd->cmd, &drawDesc );
}


void FR_CmdBeginRendering( struct frame_cmd_buffer_s *cmd )
{
	cmd->stackCmdBeingRendered++;
	if( cmd->stackCmdBeingRendered > 1 ) {
		return;
	}

	NriAttachmentsDesc attachmentsDesc = {0};
	attachmentsDesc.colorNum = cmd->state.numColorAttachments;
	attachmentsDesc.colors = cmd->state.colorAttachment;
	attachmentsDesc.depthStencil = cmd->state.depthAttachment;
	rsh.nri.coreI.CmdBeginRendering( cmd->cmd, &attachmentsDesc );
	rsh.nri.coreI.CmdSetViewports( cmd->cmd, cmd->state.viewports, FR_CmdNumViewports( cmd ) );
	rsh.nri.coreI.CmdSetScissors( cmd->cmd, cmd->state.scissors, FR_CmdNumViewports( cmd ) );
	cmd->state.dirty &= ~(CMD_DIRT_VIEWPORT | CMD_DIRT_SCISSOR);
}

void FR_CmdEndRendering( struct frame_cmd_buffer_s *cmd )
{
	cmd->stackCmdBeingRendered--;
	if( cmd->stackCmdBeingRendered > 0 ) {
		return;
	}
	rsh.nri.coreI.CmdEndRendering( cmd->cmd );
}

void TransitionPogoBufferToShaderResource(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo) {
		NriTextureBarrierDesc transitionBarriers = { 0 };
		if( pogo->isAttachment ) {
			transitionBarriers.before = (NriAccessLayoutStage){	
			.layout = NriLayout_COLOR_ATTACHMENT, 
			.access = NriAccessBits_COLOR_ATTACHMENT, 
			};
		} else {
			return;
		}
		transitionBarriers.after = ( NriAccessLayoutStage ){ 
			.layout = NriLayout_SHADER_RESOURCE, 
			.access = NriAccessBits_SHADER_RESOURCE, 
			.stages = NriStageBits_FRAGMENT_SHADER 
		};
		transitionBarriers.texture = pogo->colorTexture;
		NriBarrierGroupDesc barrierGroupDesc = { 0 };
		barrierGroupDesc.textureNum = 1;
		barrierGroupDesc.textures = &transitionBarriers;
		rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
		pogo->isAttachment = false;
}

void TransitionPogoBufferToAttachment(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo) {
		NriTextureBarrierDesc transitionBarriers = { 0 };
		if( pogo->isAttachment ) {
			return;
		} else {
			transitionBarriers.before = (NriAccessLayoutStage){	
				.layout = NriLayout_SHADER_RESOURCE, 
				.access = NriAccessBits_SHADER_RESOURCE, 
			};
		}
		transitionBarriers.after = ( NriAccessLayoutStage ){ 
			.layout = NriLayout_COLOR_ATTACHMENT, 
			.access = NriAccessBits_COLOR_ATTACHMENT, 
			.stages = NriStageBits_COLOR_ATTACHMENT 
		};
		transitionBarriers.texture = pogo->colorTexture;
		NriBarrierGroupDesc barrierGroupDesc = { 0 };
		barrierGroupDesc.textureNum = 1;
		barrierGroupDesc.textures = &transitionBarriers;
		rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
		pogo->isAttachment = true;
}

void FR_BindPogoBufferAttachment(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo) {
	const NriTextureDesc *depthDesc = rsh.nri.coreI.GetTextureDesc( frame->textureBuffers.depthTexture );
	TransitionPogoBufferToAttachment(frame, pogo);

	const NriDescriptor *colorAttachments[] = { pogo->colorAttachment };
	const struct NriViewport viewports[] = {
		( NriViewport ){ .x = 0, .y = 0, .width = frame->textureBuffers.screen.width, .height = frame->textureBuffers.screen.height, .depthMin = 0.0f, .depthMax = 1.0f } };
	const struct NriRect scissors[] = { frame->textureBuffers.screen };
	const NriFormat colorFormats[] = { POGO_BUFFER_TEXTURE_FORMAT };
	FR_CmdSetTextureAttachment( frame, colorFormats, colorAttachments, viewports, scissors, 1, depthDesc->format, frame->textureBuffers.depthAttachment);
}

void initGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *alloc, const struct gpu_frame_ele_ring_desc_s *desc ) {
	assert(alloc);
	memset(alloc, 0, sizeof(struct gpu_frame_ele_allocator_s ));
	
	assert(desc);
	alloc->usageBits = desc->usageBits;
	alloc->elementStride = desc->elementStride;
	alloc->tail = 0;
	alloc->head = 1;
	assert(alloc->elementStride > 0);
	NriBufferDesc bufferDesc = { 
		.size = desc->elementStride * desc->numElements, 
		.structureStride = alloc->elementStride, 
		.usage = desc->usageBits
	};
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &bufferDesc, &alloc->buffer ) )
	NriResourceGroupDesc resourceGroupDesc = { 
		.buffers = &alloc->buffer , 
		.bufferNum = 1, 
		.memoryLocation = NriMemoryLocation_HOST_UPLOAD 
	};
	assert( rsh.nri.helperI.CalculateAllocationNumber(rsh.nri.device, &resourceGroupDesc ) == 1 );
	NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &alloc->memory ) );

}
void freeGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *alloc ) {
	assert(alloc->memory != NULL);
	assert(alloc->buffer != NULL);
	rsh.nri.coreI.FreeMemory( alloc->memory );
	rsh.nri.coreI.DestroyBuffer( alloc->buffer );

}


void GPUFrameEleAlloc( struct frame_cmd_buffer_s *cmd, struct gpu_frame_ele_allocator_s* alloc, size_t numElements, struct gpu_frame_ele_req_s *req ) {
	assert(cmd);
	const NriBufferDesc* bufferDesc = rsh.nri.coreI.GetBufferDesc(alloc->buffer);
	size_t maxBufferElements = bufferDesc->size / alloc->elementStride; 

	// reclaim segments that are unused 
	while( alloc->tail != alloc->head && cmd->frameCount >= (alloc->segment[alloc->tail].frameNum + NUMBER_FRAMES_FLIGHT) ) {
		assert(alloc->numElements >= alloc->segment[alloc->tail].numElements);
		alloc->numElements -= alloc->segment[alloc->tail].numElements;
		alloc->elementOffset = ( alloc->elementOffset + alloc->segment[alloc->tail].numElements ) % maxBufferElements;
		alloc->segment[alloc->tail].numElements = 0;
		alloc->segment[alloc->tail].frameNum = 0;
		alloc->tail = (alloc->tail + 1) % Q_ARRAY_COUNT(alloc->segment); 
	}

	// the frame has change
	if(cmd->frameCount != alloc->segment[alloc->head].frameNum) {
		alloc->head = (alloc->head + 1) % Q_ARRAY_COUNT(alloc->segment);
		alloc->segment[alloc->head].frameNum = cmd->frameCount;
		alloc->segment[alloc->head].numElements = 0;
		assert(alloc->head != alloc->tail); // this shouldn't happen
	}

	size_t elmentEndOffset = (alloc->elementOffset + alloc->numElements) % maxBufferElements; 
	assert(alloc->elementOffset < maxBufferElements);
	assert(elmentEndOffset < maxBufferElements);
	// we don't have enough space to fit into the end of the buffer give up the remaning and move the cursor to the start
	if( alloc->elementOffset < elmentEndOffset && elmentEndOffset + numElements > maxBufferElements ) {
		const uint32_t remaining = ( maxBufferElements - elmentEndOffset );
		alloc->segment[alloc->head].numElements += remaining;
		alloc->numElements += remaining;
		elmentEndOffset = 0;
		assert((alloc->elementOffset + alloc->numElements) % maxBufferElements == 0);
	}
	size_t remainingSpace = 0;
	if(elmentEndOffset < alloc->elementOffset) { // the buffer has wrapped around
		remainingSpace = alloc->elementOffset - elmentEndOffset;
	} else {
		remainingSpace = maxBufferElements - elmentEndOffset;
	}
	assert(remainingSpace <= maxBufferElements);

	// there is not enough avalaible space we need to reallocate
	if( numElements > remainingSpace ) {
		// attach it to the cmd buffer to be cleaned up on the round trip
		arrpush( cmd->freeBuffers, alloc->buffer );
		arrpush( cmd->freeMemory, alloc->memory );
		for( size_t i = 0; i < Q_ARRAY_COUNT( alloc->segment ); i++ ) {
			alloc->segment[i].numElements = 0;
			alloc->segment[i].frameNum = 0;
		}
		alloc->numElements = 0;
		alloc->elementOffset = 0;
		elmentEndOffset = 0; // move the cursor to the beginning

		do {
			maxBufferElements = ( maxBufferElements + ( maxBufferElements >> 1 ) );
		} while( maxBufferElements < numElements );
		NriBufferDesc initBufferDesc = { 
			.size =  maxBufferElements * alloc->elementStride, 
			.structureStride = alloc->elementStride, 
			.usage = alloc->usageBits 
		};

		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &initBufferDesc, &alloc->buffer ) )
		NriResourceGroupDesc resourceGroupDesc = { .buffers = &alloc->buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
		assert( rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc ) == 1 );
		NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &alloc->memory ) );
	}
	alloc->segment[alloc->head].numElements += numElements;
	alloc->numElements += numElements;
	
	req->elementOffset = elmentEndOffset;
	req->buffer = alloc->buffer;
	req->elementStride = alloc->elementStride;
	req->numElements = numElements; // includes the padding on the end of the buffer
}

//void initGPUFrameAlloc(struct gpu_frame_allocator_s* alloc, const struct gpu_frame_buffer_ring_desc_s* desc) {
//	assert(alloc);
//	assert(desc);
//	memset(alloc, 0, sizeof(struct gpu_frame_allocator_s));
//	alloc->usageBits = desc->usageBits;
//	alloc->bufferAlignment = desc->alignmentReq;
//	alloc->tail = 0;
//	alloc->head = 1;
//	NriBufferDesc bufferDesc = { .size = desc->reservedSize, .structureStride = desc->alignmentReq, .usage = desc->usageBits};
//	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &bufferDesc, &alloc->buffer ) )
//	NriResourceGroupDesc resourceGroupDesc = { .buffers = &alloc->buffer , .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
//	assert( rsh.nri.helperI.CalculateAllocationNumber(rsh.nri.device, &resourceGroupDesc ) == 1 );
//	NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &alloc->memory ) );
//}
//
//void freeGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *alloc )
//{
//	if( alloc->memory ) {
//		rsh.nri.coreI.FreeMemory( alloc->memory );
//	}
//	if( alloc->buffer ) {
//		rsh.nri.coreI.DestroyBuffer( alloc->buffer );
//	}
//}
//
//void GPUFrameAlloc(struct frame_cmd_buffer_s* cmd, struct gpu_frame_allocator_s *alloc, size_t reqSize, struct gpu_frame_buffer_req_s* req) {
//	assert(cmd);
//	const NriBufferDesc* bufferDesc = rsh.nri.coreI.GetBufferDesc(alloc->buffer);
//	
//	while( alloc->tail != alloc->head && (alloc->segment[alloc->tail].frameNum + NUMBER_FRAMES_FLIGHT) >= cmd->frameCount ) {
//		alloc->allocSize -= alloc->segment[alloc->tail].allocSize;
//		alloc->tailOffset = (alloc->tailOffset + alloc->segment[alloc->tail].allocSize) % bufferDesc->size;
//		alloc->segment[alloc->tail].allocSize = 0;
//		alloc->segment[alloc->tail].frameNum = 0;
//		alloc->tail = (alloc->tail + 1) % Q_ARRAY_COUNT(alloc->segment); 
//	}
//
//	// the frame has change
//	if(cmd->frameCount != alloc->segment[alloc->head].frameNum) {
//		alloc->head = (alloc->head + 1) % Q_ARRAY_COUNT(alloc->segment);
//		alloc->segment[alloc->head].frameNum = cmd->frameCount;
//		assert(alloc->head != alloc->tail); // this shouldn't happen
//	}
//
//	size_t currentOffset = (alloc->tailOffset + alloc->allocSize) % bufferDesc->size; 
//	// we don't have enough space to fit into the end of the buffer
//	if( alloc->tailOffset < currentOffset && ALIGN( currentOffset + reqSize, alloc->bufferAlignment ) > bufferDesc->size ) {
//		const size_t remaining = (bufferDesc->size - currentOffset);
//		alloc->segment[alloc->head].allocSize += remaining;
//		alloc->allocSize += remaining;
//		currentOffset = 0;
//	}
//
//	// there is not enough avalaible space we need to reallocate
//	size_t updateOffset = ALIGN( currentOffset + reqSize, alloc->bufferAlignment );
//	if( updateOffset > alloc->tailOffset ) {
//		// attach it to the cmd buffer to be cleaned up on the round trip
//		arrpush( cmd->freeBuffers, alloc->buffer );
//		arrpush( cmd->freeMemory, alloc->memory );
//		for( size_t i = 0; i < Q_ARRAY_COUNT( alloc->segment ); i++ ) {
//			alloc->segment[i].allocSize = 0;
//			alloc->segment[i].frameNum = 0;
//		}
//		alloc->allocSize = 0;
//		alloc->tailOffset = 0;
//		updateOffset = ALIGN(reqSize, alloc->bufferAlignment );
//		NriBufferDesc bufferDesc = { .size = bufferDesc.size + ( bufferDesc.size >> 1 ), .structureStride = alloc->bufferAlignment, .usage = alloc->usageBits };
//		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &bufferDesc, &alloc->buffer ) )
//		NriResourceGroupDesc resourceGroupDesc = { .buffers = &alloc->buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
//		assert( rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc ) == 1 );
//		NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &alloc->memory ) );
//	}
//
//	req->buffer = alloc->buffer;
//	req->bufferOffset = updateOffset;
//	req->bufferSize = ( currentOffset - updateOffset ); // includes the padding on the end of the buffer
//	alloc->segment[alloc->head].allocSize += req->bufferSize;
//	alloc->allocSize += req->bufferSize;
//}


