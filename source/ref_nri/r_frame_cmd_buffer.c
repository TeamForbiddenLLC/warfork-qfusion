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
	}
}

void FR_ConfigurePipelineAttachment( struct pipeline_desc_s *desc, enum RI_Format_e *formats, size_t numAttachment, enum RI_Format_e depthFormat ) {
	assert(desc);
	desc->numColorsAttachments = numAttachment;
	for(size_t i = 0; i < numAttachment; i++) {
		desc->colorAttachments[i] = formats[i];
	}
	desc->depthFormat = depthFormat;
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

