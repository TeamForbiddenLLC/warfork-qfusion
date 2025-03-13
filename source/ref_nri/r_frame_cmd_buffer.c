#include "r_frame_cmd_buffer.h"

#include "r_local.h"
#include "r_resource.h"

#include "r_model.h"

#include "ri_conversion.h"
#include "ri_types.h"
#include "stb_ds.h"
#include "qhash.h"

void FR_CmdResetCmdState( struct FrameState_s *cmd, enum CmdStateResetBits bits )
{
	memset( cmd->vertexBuffers, 0, sizeof( struct RIBuffer_s ) * MAX_VERTEX_BINDINGS );
	cmd->dirtyVertexBuffers = 0;
}

void FR_CmdSetDepthRangeAll(struct FrameState_s *cmd, float depthMin, float depthMax) {
	assert(cmd);
	cmd->viewport.depthMin = depthMin;
	cmd->viewport.depthMax = depthMax;
	cmd->dirty |= CMD_DIRT_VIEWPORT;
}

void FR_CmdSetScissor( struct FrameState_s *cmd, const struct RIRect_s scissors )
{
	assert(cmd);
	cmd->scissor = scissors;
	cmd->dirty |= CMD_DIRT_SCISSOR;
}

void FR_CmdSetViewport( struct FrameState_s *cmd, const struct RIViewport_s viewport )
{
	assert(cmd);
	cmd->viewport = viewport;
	cmd->dirty |= CMD_DIRT_VIEWPORT;
}

void FR_CmdSetIndexBuffer( struct FrameState_s *cmd, struct RIBuffer_s* buffer, uint64_t offset, enum RIIndexType_e indexType )
{
	assert(cmd);
	cmd->dirty |= CMD_DIRT_INDEX_BUFFER;
	cmd->indexType = indexType;
	cmd->indexBufferOffset = offset;
	cmd->indexBuffer = *buffer;
}

void FR_CmdSetVertexBuffer( struct FrameState_s *cmd, uint32_t slot, struct RIBuffer_s *buffer, uint64_t offset )
{
	assert(cmd);
	assert( slot < MAX_VERTEX_BINDINGS );
	cmd->dirtyVertexBuffers |= ( 0x1 << slot );
	cmd->offsets[slot] = offset;
	cmd->vertexBuffers[slot] = *buffer;
}

void FR_CmdResetCommandState( struct FrameState_s *cmd, enum CmdResetBits bits)
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
		cmd->pipeline.depthBiasConstant = 0.0f;
		cmd->pipeline.depthBiasClamp = 0.0f;
		cmd->pipeline.depthBiasSlope = 0.0f;
		cmd->pipeline.compareFunc = RI_COMPARE_ALWAYS;
	}
	
}

void UpdateFrameUBO( struct FrameState_s *cmd, struct RIDescriptor_s *req, void *data, size_t size )
{

	const hash_t hash = hash_data( HASH_INITIAL_VALUE, data, size );
	if( req->cookie != hash ) {
		req->cookie = hash;
		struct RIBufferScratchAllocReq_s scratchReq = RIAllocBufferFromScratchAlloc(&rsh.device, &rsh.frameSets[rsh.frameSetCount % NUMBER_FRAMES_FLIGHT].uboScratchAlloc, size);
		req->vk.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		req->vk.buffer.buffer = scratchReq.block.vk.buffer;
		req->vk.buffer.offset = scratchReq.bufferOffset;
		req->vk.buffer.range = size; 
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


void FrameCmdBufferFree(struct FrameState_s* cmd) {
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

void FR_ConfigurePipelineAttachment( struct pipeline_desc_s *desc, enum RI_Format_e *formats, size_t numAttachment, enum RI_Format_e depthFormat ) {
	assert(desc);
	desc->numColorsAttachments = numAttachment;
	for(size_t i = 0; i < numAttachment; i++) {
		desc->colorAttachments[i] = formats[i];
	}
	desc->depthFormat = depthFormat;
}


//#if ( DEVICE_IMPL_VULKAN )
//void FR_VK_FillPipelineAttachmentFromPipeline( struct pipeline_desc_s *pipeline, const VkRenderingInfo *renderInfo )
//{
//	pipeline->numColorsAttachments = renderInfo->colorAttachmentCount;
//	for( size_t i = 0; i < renderInfo->colorAttachmentCount; i++ ) {
//		pipeline->colorAttachments[i] = renderInfo->pColorAttachments[i].imageLayout 
//	}
//}
//#endif

void ResetFrameCmdBuffer(  struct FrameState_s *cmd )
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

void FR_CmdDraw( struct FrameState_s *cmd, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		if( cmd->dirty & CMD_DIRT_VIEWPORT ) {
			VkViewport viewports[MAX_COLOR_ATTACHMENTS];
			for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
				viewports[i] = RIToVKViewport( &cmd->viewport );
			}
			vkCmdSetViewport( cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), viewports );
			cmd->dirty &= ~CMD_DIRT_VIEWPORT;
		}

		if( cmd->dirty & CMD_DIRT_SCISSOR ) {
			VkRect2D scissors[MAX_COLOR_ATTACHMENTS];
			for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
				scissors[i] = RIToVKRect2D( &cmd->scissor );
			}
			vkCmdSetScissor( cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), scissors );
			cmd->dirty &= ~CMD_DIRT_SCISSOR;
		}
		vkCmdDraw( cmd->handle.vk.cmd, vertexNum, Q_MAX( 1, instanceNum ), baseVertex, baseInstance );
	}
#endif
}

void FR_CmdDrawElements( struct FrameState_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t vertexSlot = 0;
		for( uint32_t attr = cmd->dirtyVertexBuffers; attr > 0; attr = ( attr >> 1 ), vertexSlot++ ) {
			if( cmd->dirtyVertexBuffers & ( 1 << vertexSlot ) ) {
				VkBuffer buffer[1] = { cmd->vertexBuffers[vertexSlot].vk.buffer };
				const uint64_t offset[1] = { cmd->offsets[vertexSlot] };
				vkCmdBindVertexBuffers( cmd->handle.vk.cmd, vertexSlot, 1, buffer, offset );
			}
		}
		cmd->dirtyVertexBuffers = 0;

		if( cmd->dirty & CMD_DIRT_INDEX_BUFFER ) {
			vkCmdBindIndexBuffer( cmd->handle.vk.cmd, cmd->indexBuffer.vk.buffer, cmd->indexBufferOffset, ri_vk_RIIndexTypeToVK( cmd->indexType ) );
			cmd->dirty &= ~CMD_DIRT_INDEX_BUFFER;
		}

		if( cmd->dirty & CMD_DIRT_VIEWPORT ) {
			VkViewport viewports[MAX_COLOR_ATTACHMENTS];
			for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
				viewports[i] = RIToVKViewport( &cmd->viewport );
			}
			vkCmdSetViewport( cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), viewports );
			cmd->dirty &= ~CMD_DIRT_VIEWPORT;
		}

		if( cmd->dirty & CMD_DIRT_SCISSOR ) {
			VkRect2D scissors[MAX_COLOR_ATTACHMENTS];
			for( size_t i = 0; i < FR_CmdNumViewports( cmd ); i++ ) {
				scissors[i] = RIToVKRect2D( &cmd->scissor );
			}
			vkCmdSetScissor( cmd->handle.vk.cmd, 0, FR_CmdNumViewports( cmd ), scissors );
			cmd->dirty &= ~CMD_DIRT_SCISSOR;
		}
		vkCmdDrawIndexed( cmd->handle.vk.cmd, indexNum, Q_MAX( 1, instanceNum ), baseIndex, baseVertex, baseInstance );
	}
#endif
}

