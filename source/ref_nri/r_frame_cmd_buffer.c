#include "r_frame_cmd_buffer.h"

#include "r_local.h"
#include "r_resource.h"

#include "r_model.h"

#include "ri_conversion.h"
#include "ri_renderer.h"
#include "ri_conversion_mtl.h"
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
	struct r_frame_set_s *activeSet = R_GetActiveFrameSet();
	const hash_t hash = hash_data_hsieh( HASH_INITIAL_VALUE + rsh.frameSetCount, data, size );
	if( req->cookie != hash ) {
		req->cookie = hash;
		struct RIBufferScratchAllocReq_s scratchReq = RIAllocBufferFromScratchAlloc( &rsh.device, &activeSet->uboScratchAlloc, size );
		// Scratch UBOs reuse the same backing buffer handle each frame at varying offsets, so the buffer's
		// handle is not a stable identity; the content hash above IS the cookie (set directly, not via a
		// builder — RIDescriptorUniformBuffer needs an RIBuffer_s, which the scratch allocator doesn't expose).
		req->type = RI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
#if ( DEVICE_IMPL_VULKAN )
		req->vk.buffer.buffer = scratchReq.block.vk.buffer;
		req->vk.buffer.offset = scratchReq.bufferOffset;
		req->vk.buffer.range = size;
#endif
#if ( DEVICE_IMPL_MTL )
		req->mtl.buffer = scratchReq.block.mtl.buffer;
		req->mtl.offset = scratchReq.bufferOffset;
		req->mtl.range = size;
#endif
		memcpy( (uint8_t*)scratchReq.pMappedAddress + scratchReq.bufferOffset, data, size );
		RIFinishScrachReq( &rsh.device, &scratchReq );
	}
}

void FR_ConfigurePipelineAttachment( struct pipeline_desc_s *desc, enum RI_Format_e *formats, size_t numAttachment, enum RI_Format_e depthFormat ) {
	assert(desc);
	assert( numAttachment <= Q_ARRAY_COUNT( desc->colorAttachments ) );
	desc->numColorsAttachments = numAttachment;
	for(size_t i = 0; i < numAttachment; i++) {
		desc->colorAttachments[i] = formats[i];
	}
	desc->depthFormat = depthFormat;
}

void FR_CmdBeginRendering( struct RIDevice_s *device, struct FrameState_s *cmd, const struct RIRenderingDesc_s *desc )
{
	assert( cmd );
	cmd->activeRendering = *desc;
	RICmdBeginRendering( device, &cmd->handle, desc );

	// Re-dirty everything the draw path flushes lazily. Only slots holding a live buffer are marked:
	// re-binding an empty slot would hand Metal a nil buffer. Redundant but harmless on Vulkan, which
	// keeps both backends on one code path rather than making Metal a special case at the call sites.
	cmd->dirty |= CMD_DIRT_VIEWPORT | CMD_DIRT_SCISSOR;
	if( IsRIBufferValid( rsh.device.renderer, &cmd->indexBuffer ) )
		cmd->dirty |= CMD_DIRT_INDEX_BUFFER;
	for( uint32_t slot = 0; slot < MAX_VERTEX_BINDINGS; slot++ ) {
		if( IsRIBufferValid( rsh.device.renderer, &cmd->vertexBuffers[slot] ) )
			cmd->dirtyVertexBuffers |= ( 0x1u << slot );
	}
}

void FR_CmdClearAttachments( struct FrameState_s *cmd, bool clearColors, const float clearColor[4], bool clearDepth )
{
	assert( cmd );
#if ( DEVICE_IMPL_VULKAN )
	{
		if( cmd->pipeline.numColorsAttachments == 0 )
			return;
		size_t numClear = 0;
		VkClearRect clearRect[MAX_COLOR_ATTACHMENTS + 1] = { 0 };
		VkClearAttachment clearAttach[MAX_COLOR_ATTACHMENTS + 1] = { 0 };
		if( clearColors ) {
			for( size_t i = 0; i < cmd->pipeline.numColorsAttachments; i++ ) {
				assert( numClear < Q_ARRAY_COUNT( clearAttach ) );
				clearRect[numClear].baseArrayLayer = 0;
				clearRect[numClear].rect = RIViewportToRect2D( &cmd->viewport );
				clearRect[numClear].layerCount = 1;
				clearAttach[numClear].colorAttachment = i;
				for( size_t c = 0; c < 4; c++ )
					clearAttach[numClear].clearValue.color.float32[c] = clearColor ? clearColor[c] : 0.0f;
				clearAttach[numClear].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				numClear++;
			}
		}
		if( clearDepth ) {
			assert( numClear < Q_ARRAY_COUNT( clearAttach ) );
			clearRect[numClear].baseArrayLayer = 0;
			clearRect[numClear].rect = RIViewportToRect2D( &cmd->viewport );
			clearRect[numClear].layerCount = 1;
			clearAttach[numClear].clearValue.depthStencil.depth = 1.0f;
			clearAttach[numClear].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			numClear++;
		}
		if( numClear > 0 )
			vkCmdClearAttachments( cmd->handle.vk.cmd, numClear, clearAttach, numClear, clearRect );
	}
#endif
#if ( DEVICE_IMPL_MTL )
	{
		// Metal has no mid-pass clear; end the encoder and re-begin the same pass with the requested
		// attachments cleared. FR_CmdBeginRendering re-dirties the lazily-flushed state and the fresh
		// encoderEpoch invalidates residency caching, so the restart is transparent to the draw path.
		struct RIRenderingDesc_s desc = cmd->activeRendering;
		for( uint32_t i = 0; i < desc.colorNum; i++ ) {
			desc.colors[i].clear = clearColors;
			for( size_t c = 0; c < 4; c++ )
				desc.colors[i].clearColor[c] = clearColor ? clearColor[c] : 0.0f;
		}
		desc.depth.clear = clearDepth && desc.hasDepth;
		RICmdEndRendering( &rsh.device, &cmd->handle );
		FR_CmdBeginRendering( &rsh.device, cmd, &desc );
	}
#endif
}

void FR_CmdDraw( struct FrameState_s *cmd, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		if( cmd->dirty & CMD_DIRT_VIEWPORT ) {
			VkViewport viewport = RIToVKViewport( &cmd->viewport );
			vkCmdSetViewport( cmd->handle.vk.cmd, 0, 1, &viewport );
			cmd->dirty &= ~CMD_DIRT_VIEWPORT;
		}

		if( cmd->dirty & CMD_DIRT_SCISSOR ) {
			VkRect2D scissor = RIToVKRect2D( &cmd->scissor );
			vkCmdSetScissor( cmd->handle.vk.cmd, 0, 1, &scissor );
			cmd->dirty &= ~CMD_DIRT_SCISSOR;
		}
		vkCmdDraw( cmd->handle.vk.cmd, vertexNum, Q_MAX( 1, instanceNum ), baseVertex, baseInstance );
	}
#endif
#if ( DEVICE_IMPL_MTL )
	{
		struct mtlc_render_command_encoder enc = cmd->handle.mtl.encoder;
		if( cmd->dirty & CMD_DIRT_VIEWPORT ) {
			mtlc_render_command_encoder_set_viewport( enc, RIToMTLViewport( &cmd->viewport ) );
			cmd->dirty &= ~CMD_DIRT_VIEWPORT;
		}
		if( cmd->dirty & CMD_DIRT_SCISSOR ) {
			mtlc_render_command_encoder_set_scissor_rect( enc, RIToMTLScissorRect( &cmd->scissor ) );
			cmd->dirty &= ~CMD_DIRT_SCISSOR;
		}
		// vkCmdDraw with vertexCount 0 is a legal no-op; Metal validation rejects it. The frontend does
		// issue empty draws (surfaces whose element list came out empty), so match Vulkan and skip.
		if( vertexNum == 0 )
			return;
		mtlc_render_command_encoder_draw_primitives( enc, RITopologyToMTL( cmd->pipeline.topology ), baseVertex, vertexNum,
													 Q_MAX( 1, instanceNum ), baseInstance );
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
			VkViewport viewport = RIToVKViewport( &cmd->viewport );
			vkCmdSetViewport( cmd->handle.vk.cmd, 0, 1, &viewport );
			cmd->dirty &= ~CMD_DIRT_VIEWPORT;
		}

		if( cmd->dirty & CMD_DIRT_SCISSOR ) {
			VkRect2D scissor = RIToVKRect2D( &cmd->scissor );
			vkCmdSetScissor( cmd->handle.vk.cmd, 0, 1, &scissor );
			cmd->dirty &= ~CMD_DIRT_SCISSOR;
		}
		vkCmdDrawIndexed( cmd->handle.vk.cmd, indexNum, Q_MAX( 1, instanceNum ), baseIndex, baseVertex, baseInstance );
	}
#endif
#if ( DEVICE_IMPL_MTL )
	{
		struct mtlc_render_command_encoder enc = cmd->handle.mtl.encoder;
		uint32_t vertexSlot = 0;
		for( uint32_t attr = cmd->dirtyVertexBuffers; attr > 0; attr = ( attr >> 1 ), vertexSlot++ ) {
			if( cmd->dirtyVertexBuffers & ( 1 << vertexSlot ) ) {
				// Vertex buffers live at Metal buffer slots R_MTL_VERTEX_BUFFER_BASE+ so they never collide
				// with the low descriptor/UBO slots SPIRV-Cross assigns (see RP_ResolvePipeline).
				mtlc_render_command_encoder_set_vertex_buffer( enc, cmd->vertexBuffers[vertexSlot].mtl.buffer,
															   cmd->offsets[vertexSlot], R_MTL_VERTEX_BUFFER_BASE + vertexSlot );
			}
		}
		cmd->dirtyVertexBuffers = 0;

		if( cmd->dirty & CMD_DIRT_VIEWPORT ) {
			mtlc_render_command_encoder_set_viewport( enc, RIToMTLViewport( &cmd->viewport ) );
			cmd->dirty &= ~CMD_DIRT_VIEWPORT;
		}
		if( cmd->dirty & CMD_DIRT_SCISSOR ) {
			mtlc_render_command_encoder_set_scissor_rect( enc, RIToMTLScissorRect( &cmd->scissor ) );
			cmd->dirty &= ~CMD_DIRT_SCISSOR;
		}
		cmd->dirty &= ~CMD_DIRT_INDEX_BUFFER;

		// vkCmdDrawIndexed with indexCount 0 is a legal no-op; Metal validation rejects it
		// ("indexCount(0) must be non-zero"), and the frontend does issue such draws at map start.
		if( indexNum == 0 )
			return;

		// Metal's drawIndexedPrimitives has no firstIndex parameter; fold baseIndex into the byte offset.
		const uint64_t indexSize = ( cmd->indexType == RI_INDEX_TYPE_16 ) ? 2 : 4;
		const uint64_t indexOffset = cmd->indexBufferOffset + (uint64_t)baseIndex * indexSize;
		mtlc_render_command_encoder_draw_indexed_primitives( enc, RITopologyToMTL( cmd->pipeline.topology ), indexNum,
															 RIIndexTypeToMTL( cmd->indexType ), cmd->indexBuffer.mtl.buffer, indexOffset,
															 Q_MAX( 1, instanceNum ), (mtlc_integer)baseVertex, baseInstance );
	}
#endif
}

