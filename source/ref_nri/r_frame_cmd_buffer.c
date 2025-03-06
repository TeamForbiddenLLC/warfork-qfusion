#include "r_frame_cmd_buffer.h"

#include "r_local.h"
#include "r_nri.h"
#include "r_resource.h"

#include "r_model.h"

#include "ri_conversion.h"
#include "ri_types.h"
#include "stb_ds.h"
#include "qhash.h"


//void FR_CmdResetAttachmentToBackbuffer( struct frame_cmd_buffer_s *cmd )
//{
//	//const NriTextureDesc *colorDesc = rsh.nri.coreI.GetTextureDesc( cmd->textureBuffers.colorTexture );
//	//const NriTextureDesc *depthDesc = rsh.nri.coreI.GetTextureDesc( cmd->textureBuffers.depthTexture );
//
//	//const NriDescriptor *colorAttachments[] = { cmd->textureBuffers.colorAttachment };
//	//const struct NriViewport viewports[] = {
//	//	( NriViewport ){ 
//	//		.x = 0, 
//	//		.y = 0, 
//	//		.width = cmd->textureBuffers.screen.width, 
//	//		.height = cmd->textureBuffers.screen.height, 
//	//		.depthMin = 0.0f, 
//	//		.depthMax = 1.0f 
//	//	} 
//	//};
//	//const struct NriRect scissors[] = { cmd->textureBuffers.screen };
//	//const NriFormat colorFormats[] = { colorDesc->format };
//
//	//FR_CmdSetTextureAttachment( cmd, colorFormats, colorAttachments, viewports, scissors, 1, depthDesc->format, cmd->textureBuffers.depthAttachment );
//}

void FR_CmdResetCmdState( struct frame_cmd_buffer_s *cmd, enum CmdStateResetBits bits )
{
	memset( cmd->vertexBuffers, 0, sizeof( struct RIBufferHandle_s ) * MAX_VERTEX_BINDINGS );
	cmd->dirtyVertexBuffers = 0;
}

void FR_CmdSetDepthRangeAll(struct frame_cmd_buffer_s *cmd, float depthMin, float depthMax) {
	assert(cmd);
	cmd->viewport.depthMin = depthMin;
	cmd->viewport.depthMax = depthMax;
	cmd->dirty |= CMD_DIRT_VIEWPORT;
}

void FR_CmdSetScissor( struct frame_cmd_buffer_s *cmd, const struct RIRect_s scissors )
{
	assert(cmd);
	//for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
	//	assert( scissors.x >= 0 && scissors.y >= 0 );
	//	cmd->state.scissors[i] = scissors;
	//}
	cmd->scissor = scissors;
	cmd->dirty |= CMD_DIRT_SCISSOR;
}

void FR_CmdSetViewport( struct frame_cmd_buffer_s *cmd, const struct RIViewport_s viewport )
{
	cmd->viewport = viewport;
	cmd->dirty |= CMD_DIRT_VIEWPORT;
}

void FR_CmdSetIndexBuffer( struct frame_cmd_buffer_s *cmd, struct RIBufferHandle_s* buffer, uint64_t offset, enum RIIndexType_e indexType )
{
	cmd->dirty |= CMD_DIRT_INDEX_BUFFER;
	cmd->indexType = indexType;
	cmd->indexBufferOffset = offset;
	cmd->indexBuffer = *buffer;
}

void FR_CmdSetVertexBuffer( struct frame_cmd_buffer_s *cmd, uint32_t slot, struct RIBufferHandle_s *buffer, uint64_t offset )
{
	assert( slot < MAX_VERTEX_BINDINGS );
	cmd->dirtyVertexBuffers |= ( 0x1 << slot );
	cmd->offsets[slot] = offset;
	cmd->vertexBuffers[slot] = *buffer;
}

void FR_CmdResetCommandState( struct frame_cmd_buffer_s *cmd, enum CmdResetBits bits)
{
	if(bits & CMD_RESET_INDEX_BUFFER) {
		cmd->dirty &= ~CMD_DIRT_INDEX_BUFFER;
	}
	if(bits & CMD_RESET_VERTEX_BUFFER) {
		cmd->dirtyVertexBuffers = 0;
	}
	if(bits & CMD_RESET_DEFAULT_PIPELINE_LAYOUT ) {
		cmd->pipeline.colorSrcFactor = RI_BLEND_ONE;
		cmd->pipeline.colorDstFactor = RI_BLEND_ZERO;
		cmd->pipeline.colorWriteMask = RI_COLOR_WRITE_RGB;
		cmd->pipeline.colorBlendEnabled = false;
		cmd->pipeline.flippedViewport = false;
		cmd->pipeline.depthWrite = false;
		cmd->pipeline.cullMode = RI_CULL_MODE_FRONT;
		cmd->pipeline.depthBias = (struct frame_pipeline_depth_bias_s ){0};
		cmd->pipeline.compareFunc= RI_COMPARE_ALWAYS;
	}
	
}

void TryCommitFrameUBOInstance( struct RIDevice_s *device, struct frame_cmd_buffer_s *cmd, struct RIDescriptor_s *desc, void *data, size_t size )
{
	assert(cmd);
	const hash_t hash = hash_data( HASH_INITIAL_VALUE, data, size );
	if(hash == desc->cookie)
		return;
	desc->cookie = hash;
	struct RIBufferScratchAllocReq_s req = RIAllocBufferFromScratchAlloc( device, &rsh.frameSets[rsh.frameSetCount % NUMBER_FRAMES_FLIGHT].uboScratchAlloc, size );
	GPU_VULKAN_BLOCK( (device->renderer ), ( {
		memcpy(((uint8_t*)req.pMappedAddress) + req.bufferOffset, data, size);
		desc->vk.buffer.info.buffer = req.block.vk.buffer;
		desc->vk.buffer.info.offset = req.bufferOffset;
		desc->vk.buffer.info.range = req.bufferSize;
	} ) )
}

void UpdateFrameUBO( struct frame_cmd_buffer_s *cmd, struct RIDescriptor_s *req, void *data, size_t size )
{

	const hash_t hash = hash_data( HASH_INITIAL_VALUE, data, size );
	if( req->cookie != hash ) {
		req->cookie = hash;
		struct RIBufferScratchAllocReq_s scratchReq = RIAllocBufferFromScratchAlloc(&rsh.device, &rsh.frameSets[rsh.frameSetCount % NUMBER_FRAMES_FLIGHT].uboScratchAlloc, size);
		req->vk.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		req->vk.buffer.info.buffer = scratchReq.block.vk.buffer;
		req->vk.buffer.info.offset = scratchReq.bufferOffset;
		req->vk.buffer.info.range = size; 
		memcpy(scratchReq.block.pMappedAddress + scratchReq.bufferOffset, data, size);


		//struct block_buffer_pool_req_s poolReq = BlockBufferPoolReq( &rsh.nri, &cmd->uboBlockBuffer, size );
		//void *mem = rsh.nri.coreI.MapBuffer( poolReq.buffer, poolReq.bufferOffset, poolReq.bufferSize );
		//memcpy( mem, data, size );
		//rsh.nri.coreI.UnmapBuffer( poolReq.buffer );

		//NriBufferViewDesc bufferDesc = { .buffer = poolReq.buffer, .size = poolReq.bufferSize, .offset = poolReq.bufferOffset, .viewType = NriBufferViewType_CONSTANT };

		//NriDescriptor *descriptor = NULL;
		//NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBufferView( &bufferDesc, &descriptor ) );
		//arrpush( cmd->frameTemporaryDesc, descriptor );
		//req->descriptor = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
		//req->hash = hash;
		//req->req = poolReq;
	}
}

//void R_CmdState_RestoreAttachment(struct frame_cmd_buffer_s* cmd, const struct frame_cmd_save_attachment_s* save) {
//	assert(cmd->stackCmdBeingRendered == 0);
//	cmd->state.numColorAttachments = save->numColorAttachments;
//	memcpy(cmd->state.pipelineLayout.colorFormats, save->colorFormats, sizeof(NriFormat) * cmd->state.numColorAttachments);
//	memcpy(cmd->state.colorAttachment, save->colorAttachment, sizeof(NriDescriptor*) * cmd->state.numColorAttachments);
//	memcpy(cmd->state.scissors, save->scissors, sizeof(NriRect) * FR_CmdNumViewports(cmd));
//	memcpy(cmd->state.viewports, save->viewports, sizeof(NriViewport) * FR_CmdNumViewports(cmd));
//	cmd->state.depthAttachment = save->depthAttachment;
//	cmd->state.dirty |= (CMD_DIRT_SCISSOR | CMD_DIRT_VIEWPORT);
//}
//
//
//struct frame_cmd_save_attachment_s R_CmdState_StashAttachment(struct frame_cmd_buffer_s* cmd) {
//	struct frame_cmd_save_attachment_s save;
//	save.numColorAttachments = cmd->state.numColorAttachments;
//	memcpy(save.colorFormats, cmd->state.pipelineLayout.colorFormats, sizeof(NriFormat) * cmd->state.numColorAttachments);
//	memcpy(save.colorAttachment, cmd->state.colorAttachment, sizeof(NriDescriptor*) * cmd->state.numColorAttachments);
//	memcpy(save.scissors, cmd->state.scissors, sizeof(NriRect) * FR_CmdNumViewports(cmd));
//	memcpy(save.viewports, cmd->state.viewports, sizeof(NriViewport) * FR_CmdNumViewports(cmd));
//	save.depthAttachment = cmd->state.depthAttachment;
//	return save;
//}


void FrameCmdBufferFree(struct frame_cmd_buffer_s* cmd) {
	//for( size_t i = 0; i < arrlen( cmd->freeTextures ); i++ ) {
	//	rsh.nri.coreI.DestroyTexture( cmd->freeTextures[i] );
	//}
	//for( size_t i = 0; i < arrlen( cmd->freeBuffers ); i++ ) {
	//	rsh.nri.coreI.DestroyBuffer( cmd->freeBuffers[i] );
	//}
	//for( size_t i = 0; i < arrlen( cmd->freeMemory ); i++ ) {
	//	rsh.nri.coreI.FreeMemory( cmd->freeMemory[i] );
	//}
	//for( size_t i = 0; i < arrlen( cmd->frameTemporaryDesc ); i++ ) {
	//	rsh.nri.coreI.DestroyDescriptor( cmd->frameTemporaryDesc[i] );
	//}
	//arrfree(cmd->freeMemory);
	//arrfree(cmd->freeTextures);
	//arrfree(cmd->freeBuffers);
	//arrfree(cmd->frameTemporaryDesc);
	//FreeRIScratchAlloc( &rsh.nri, &cmd->uboBlockBuffer );
	//
	//memset( &cmd->uboSceneFrame, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboSceneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboPassObject, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboBoneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboLight, 0, sizeof( struct ubo_frame_instance_s ) );

	//rsh.nri.coreI.DestroyCommandBuffer(cmd->cmd);
	//rsh.nri.coreI.DestroyCommandAllocator(cmd->allocator);

}
void ResetFrameCmdBuffer(  struct frame_cmd_buffer_s *cmd )
{
	//const uint32_t swapchainIndex = RISwapchainAcquireNextTexture( &rsh.device, &rsh.riSwapchain );
	////cmd->pogoAttachment[0] = &rsh.pogoAttachment[2 * swapchainIndex];
	////cmd->pogoAttachment[1] = &rsh.pogoAttachment[( 2 * swapchainIndex ) + 1];
	////cmd->colorAttachment = &rsh.colorAttachment[swapchainIndex];
	////cmd->depthAttachment = &rsh.depthAttachment[swapchainIndex];
	//cmd->textureBuffers = rsh.backBuffers[rsh.nri.swapChainI.AcquireNextSwapChainTexture( rsh.swapchain )];

	//// TODO: need to re-work this logic
	//arrsetlen( cmd->freeMemory, 0 );
	//arrsetlen( cmd->freeTextures, 0 );
	//arrsetlen( cmd->freeBuffers, 0 );
	//arrsetlen( cmd->frameTemporaryDesc, 0);
	////RIResetScratchAlloc( &cmd->uboBlockBuffer );

	//memset( &cmd->uboSceneFrame, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboSceneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboPassObject, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboBoneObject, 0, sizeof( struct ubo_frame_instance_s ) );
	//memset( &cmd->uboLight, 0, sizeof( struct ubo_frame_instance_s ) );
}

void FR_CmdDraw( struct frame_cmd_buffer_s *cmd, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance )
{
	GPU_VULKAN_BLOCK( ( &rsh.renderer ), ( {
						  if( cmd->dirty & CMD_DIRT_VIEWPORT ) {
							  VkViewport viewports[MAX_COLOR_ATTACHMENTS];
							  for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
								  viewports[i] = RIToVKViewport( &cmd->viewport );
							  }
							  vkCmdSetViewport(cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), viewports );
							  cmd->dirty &= ~CMD_DIRT_VIEWPORT;
						  }

						  if( cmd->dirty & CMD_DIRT_SCISSOR ) {
							  VkRect2D scissors[MAX_COLOR_ATTACHMENTS];
							  for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
								  scissors[i] = RIToVKRect2D( &cmd->scissor );
							  }
							  vkCmdSetScissor(cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), scissors );
							  cmd->dirty &= ~CMD_DIRT_SCISSOR;
						  }
						  //NriDrawDesc drawDesc = { 0 };
						  //drawDesc.vertexNum = vertexNum;
						  //drawDesc.instanceNum = max( 1, instanceNum );
						  //drawDesc.baseVertex = baseVertex;
						  //drawDesc.baseInstance = baseInstance;
						  //rsh.nri.coreI.CmdDraw( cmd->cmd, &drawDesc );
							vkCmdDraw(cmd->handle.vk.cmd, vertexNum, Q_MAX(1, instanceNum), baseVertex, baseInstance );
					  } ) );
}


void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance )
{
	GPU_VULKAN_BLOCK( ( &rsh.renderer ), ( {
						  uint32_t vertexSlot = 0;
						  for( uint32_t attr = cmd->dirtyVertexBuffers; attr > 0; attr = ( attr >> 1 ), vertexSlot++ ) {
							  if( cmd->dirtyVertexBuffers & ( 1 << vertexSlot ) ) {
								  VkBuffer buffer[1] = { cmd->vertexBuffers[vertexSlot].vk.buffer };
								  const uint64_t offset[1] = { cmd->offsets[vertexSlot] };
								  vkCmdBindVertexBuffers(cmd->handle.vk.cmd, vertexSlot, 1, buffer, offset );
							  }
						  }

						  if( cmd->dirty & CMD_DIRT_INDEX_BUFFER ) {
								vkCmdBindIndexBuffer(cmd->handle.vk.cmd, cmd->indexBuffer.vk.buffer, cmd->indexBufferOffset, ri_vk_RIIndexTypeToVK(cmd->indexType));
							  //rsh.nri.coreI.CmdSetIndexBuffer( cmd->cmd, cmd->state.indexBuffer, cmd->state.indexBufferOffset, cmd->state.indexType );
							  cmd->dirty &= ~CMD_DIRT_INDEX_BUFFER;
						  }

						  if( cmd->dirty & CMD_DIRT_VIEWPORT ) {
							  VkViewport viewports[MAX_COLOR_ATTACHMENTS];
							  for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
								  viewports[i] = RIToVKViewport( &cmd->viewport );
							  }
							  vkCmdSetViewport(cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), viewports );
							  cmd->dirty &= ~CMD_DIRT_VIEWPORT;
						  }

						  if( cmd->dirty & CMD_DIRT_SCISSOR ) {
							  VkRect2D scissors[MAX_COLOR_ATTACHMENTS];
							  for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
								  scissors[i] = RIToVKRect2D( &cmd->scissor );
							  }
							  vkCmdSetScissor(cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), scissors );
							  cmd->dirty &= ~CMD_DIRT_SCISSOR;
						  }

						  //NriDrawIndexedDesc drawDesc = { 0 };
						  //drawDesc.indexNum = indexNum;
						  //drawDesc.instanceNum = max( 1, instanceNum );
						  //drawDesc.baseIndex = baseIndex;
						  //drawDesc.baseVertex = baseVertex;
						  //drawDesc.baseInstance = baseInstance;
						  //rsh.nri.coreI.CmdDrawIndexed( cmd->cmd, &drawDesc );
							vkCmdDrawIndexed(cmd->handle.vk.cmd, indexNum,  Q_MAX( 1, instanceNum ), baseIndex, baseVertex, baseInstance );
					  } ) );
}


void FR_CmdBeginRendering( struct frame_cmd_buffer_s *cmd )
{
	//cmd->stackCmdBeingRendered++;
	//if( cmd->stackCmdBeingRendered > 1 ) {
	//	return;
	//}

	//NriAttachmentsDesc attachmentsDesc = {0};
	//attachmentsDesc.colorNum = cmd->state.numColorAttachments;
	//attachmentsDesc.colors = cmd->state.colorAttachment;
	//attachmentsDesc.depthStencil = cmd->state.depthAttachment;
	//rsh.nri.coreI.CmdBeginRendering( cmd->cmd, &attachmentsDesc );
	//rsh.nri.coreI.CmdSetViewports( cmd->cmd, cmd->state.viewports, FR_CmdNumViewports( cmd ) );
	//rsh.nri.coreI.CmdSetScissors( cmd->cmd, cmd->state.scissors, FR_CmdNumViewports( cmd ) );
	//cmd->state.dirty &= ~(CMD_DIRT_VIEWPORT | CMD_DIRT_SCISSOR);
}

void FR_CmdEndRendering( struct frame_cmd_buffer_s *cmd )
{
//	cmd->stackCmdBeingRendered--;
//	if( cmd->stackCmdBeingRendered > 0 ) {
//		return;
//	}
//	rsh.nri.coreI.CmdEndRendering( cmd->cmd );
}

//void TransitionPogoBufferToShaderResource(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo) {
//		NriTextureBarrierDesc transitionBarriers = { 0 };
//		if( pogo->isAttachment ) {
//			transitionBarriers.before = (NriAccessLayoutStage){	
//			.layout = NriLayout_COLOR_ATTACHMENT, 
//			.access = NriAccessBits_COLOR_ATTACHMENT, 
//			};
//		} else {
//			return;
//		}
//		transitionBarriers.after = ( NriAccessLayoutStage ){ 
//			.layout = NriLayout_SHADER_RESOURCE, 
//			.access = NriAccessBits_SHADER_RESOURCE, 
//			.stages = NriStageBits_FRAGMENT_SHADER 
//		};
//		transitionBarriers.texture = pogo->colorTexture;
//		NriBarrierGroupDesc barrierGroupDesc = { 0 };
//		barrierGroupDesc.textureNum = 1;
//		barrierGroupDesc.textures = &transitionBarriers;
//		rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
//		pogo->isAttachment = false;
//}

//void TransitionPogoBufferToAttachment(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo) {
//		NriTextureBarrierDesc transitionBarriers = { 0 };
//		if( pogo->isAttachment ) {
//			return;
//		} else {
//			transitionBarriers.before = (NriAccessLayoutStage){	
//				.layout = NriLayout_SHADER_RESOURCE, 
//				.access = NriAccessBits_SHADER_RESOURCE, 
//			};
//		}
//		transitionBarriers.after = ( NriAccessLayoutStage ){ 
//			.layout = NriLayout_COLOR_ATTACHMENT, 
//			.access = NriAccessBits_COLOR_ATTACHMENT, 
//			.stages = NriStageBits_COLOR_ATTACHMENT 
//		};
//		transitionBarriers.texture = pogo->colorTexture;
//		NriBarrierGroupDesc barrierGroupDesc = { 0 };
//		barrierGroupDesc.textureNum = 1;
//		barrierGroupDesc.textures = &transitionBarriers;
//		rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
//		pogo->isAttachment = true;
//}

//void initGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *alloc, const struct gpu_frame_ele_ring_desc_s *desc ) {
//	assert(alloc);
//	memset(alloc, 0, sizeof(struct gpu_frame_ele_allocator_s ));
//	
//	assert(desc);
//	alloc->usageBits = desc->usageBits;
//	alloc->elementStride = desc->elementStride;
//	alloc->tail = 0;
//	alloc->head = 1;
//	assert(alloc->elementStride > 0);
//	NriBufferDesc bufferDesc = { 
//		.size = desc->elementStride * desc->numElements, 
//		.structureStride = alloc->elementStride, 
//		.usage = desc->usageBits
//	};
//	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &bufferDesc, &alloc->buffer ) )
//	NriResourceGroupDesc resourceGroupDesc = { 
//		.buffers = &alloc->buffer , 
//		.bufferNum = 1, 
//		.memoryLocation = NriMemoryLocation_HOST_UPLOAD 
//	};
//	assert( rsh.nri.helperI.CalculateAllocationNumber(rsh.nri.device, &resourceGroupDesc ) == 1 );
//	NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &alloc->memory ) );
//
//}
//void freeGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *alloc ) {
//	assert(alloc->memory != NULL);
//	assert(alloc->buffer != NULL);
//	rsh.nri.coreI.FreeMemory( alloc->memory );
//	rsh.nri.coreI.DestroyBuffer( alloc->buffer );
//
//}
//
//void GPUFrameEleAlloc( struct frame_cmd_buffer_s *cmd, struct gpu_frame_ele_allocator_s* alloc, size_t numElements, struct gpu_frame_ele_req_s *req ) {
//	assert(cmd);
//	const NriBufferDesc* bufferDesc = rsh.nri.coreI.GetBufferDesc(alloc->buffer);
//	size_t maxBufferElements = bufferDesc->size / alloc->elementStride; 
//
//	// reclaim segments that are unused 
//	while( alloc->tail != alloc->head && cmd->frameCount >= (alloc->segment[alloc->tail].frameNum + NUMBER_FRAMES_FLIGHT) ) {
//		assert(alloc->numElements >= alloc->segment[alloc->tail].numElements);
//		alloc->numElements -= alloc->segment[alloc->tail].numElements;
//		alloc->elementOffset = ( alloc->elementOffset + alloc->segment[alloc->tail].numElements ) % maxBufferElements;
//		alloc->segment[alloc->tail].numElements = 0;
//		alloc->segment[alloc->tail].frameNum = 0;
//		alloc->tail = (alloc->tail + 1) % Q_ARRAY_COUNT(alloc->segment); 
//	}
//
//	// the frame has change
//	if(cmd->frameCount != alloc->segment[alloc->head].frameNum) {
//		alloc->head = (alloc->head + 1) % Q_ARRAY_COUNT(alloc->segment);
//		alloc->segment[alloc->head].frameNum = cmd->frameCount;
//		alloc->segment[alloc->head].numElements = 0;
//		assert(alloc->head != alloc->tail); // this shouldn't happen
//	}
//
//	size_t elmentEndOffset = (alloc->elementOffset + alloc->numElements) % maxBufferElements; 
//	assert(alloc->elementOffset < maxBufferElements);
//	assert(elmentEndOffset < maxBufferElements);
//	// we don't have enough space to fit into the end of the buffer give up the remaning and move the cursor to the start
//	if( alloc->elementOffset < elmentEndOffset && elmentEndOffset + numElements > maxBufferElements ) {
//		const uint32_t remaining = ( maxBufferElements - elmentEndOffset );
//		alloc->segment[alloc->head].numElements += remaining;
//		alloc->numElements += remaining;
//		elmentEndOffset = 0;
//		assert((alloc->elementOffset + alloc->numElements) % maxBufferElements == 0);
//	}
//	size_t remainingSpace = 0;
//	if(elmentEndOffset < alloc->elementOffset) { // the buffer has wrapped around
//		remainingSpace = alloc->elementOffset - elmentEndOffset;
//	} else {
//		remainingSpace = maxBufferElements - elmentEndOffset;
//	}
//	assert(remainingSpace <= maxBufferElements);
//
//	// there is not enough avalaible space we need to reallocate
//	if( numElements > remainingSpace ) {
//		// attach it to the cmd buffer to be cleaned up on the round trip
//		arrpush( cmd->freeBuffers, alloc->buffer );
//		arrpush( cmd->freeMemory, alloc->memory );
//		for( size_t i = 0; i < Q_ARRAY_COUNT( alloc->segment ); i++ ) {
//			alloc->segment[i].numElements = 0;
//			alloc->segment[i].frameNum = 0;
//		}
//		alloc->numElements = 0;
//		alloc->elementOffset = 0;
//		elmentEndOffset = 0; // move the cursor to the beginning
//
//		do {
//			maxBufferElements = ( maxBufferElements + ( maxBufferElements >> 1 ) );
//		} while( maxBufferElements < numElements );
//		NriBufferDesc initBufferDesc = { 
//			.size =  maxBufferElements * alloc->elementStride, 
//			.structureStride = alloc->elementStride, 
//			.usage = alloc->usageBits 
//		};
//
//		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &initBufferDesc, &alloc->buffer ) )
//		NriResourceGroupDesc resourceGroupDesc = { .buffers = &alloc->buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
//		assert( rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc ) == 1 );
//		NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &alloc->memory ) );
//	}
//	alloc->segment[alloc->head].numElements += numElements;
//	alloc->numElements += numElements;
//	
//	req->elementOffset = elmentEndOffset;
//	req->buffer = alloc->buffer;
//	req->elementStride = alloc->elementStride;
//	req->numElements = numElements; // includes the padding on the end of the buffer
//}

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


