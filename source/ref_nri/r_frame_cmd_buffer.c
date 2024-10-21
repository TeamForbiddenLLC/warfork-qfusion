#include "r_frame_cmd_buffer.h"

#include "r_hasher.h"
#include "r_local.h"
#include "r_nri.h"
#include "r_resource.h"
#include "stb_ds.h"

#include "r_model.h"

static inline bool isNriRectSame( const NriRect r1, const NriRect r2 )
{
	return ( r1.x == r2.x && r1.y == r2.y && r1.width == r2.width && r1.height == r2.height );
}

void FR_CmdResetAttachmentToBackbuffer( struct frame_cmd_buffer_s *cmd )
{
	const NriTextureDesc *colorDesc = rsh.nri.coreI.GetTextureDesc( cmd->textureBuffers.colorTexture );
	const NriTextureDesc *depthDesc = rsh.nri.coreI.GetTextureDesc( cmd->textureBuffers.depthTexture );

	const NriDescriptor *colorAttachments[] = { cmd->textureBuffers.colorAttachment };
	const struct NriViewport viewports[] = { ( NriViewport ){
		 .x = 0, 
		 .y = 0, 
		 .width = cmd->textureBuffers.screen.width, 
		 .height = cmd->textureBuffers.screen.height,
		  .depthRangeMin = 0.0f,
		  .depthRangeMax = 1.0f
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
	assert( numAttachments == cmd->state.numColorAttachments );
	memcpy( cmd->state.scissors, scissors, sizeof( NriRect ) * numAttachments );
	cmd->state.dirty |= CMD_DIRT_SCISSOR;
}

void FR_CmdSetScissorAll( struct frame_cmd_buffer_s *cmd, const NriRect scissors )
{
	for( size_t i = 0; i < cmd->state.numColorAttachments; i++ ) {
		assert( scissors.x >= 0 && scissors.y >= 0 );
		cmd->state.scissors[i] = scissors;
	}
	cmd->state.dirty |= CMD_DIRT_SCISSOR;
}

void FR_CmdSetViewportAll( struct frame_cmd_buffer_s *cmd, const NriViewport viewport ) {
	for( size_t i = 0; i < cmd->state.numColorAttachments; i++ ) {
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
	assert( numAttachments < MAX_COLOR_ATTACHMENTS );
	assert( viewports );
	assert( scissors );
	for( size_t i = 0; i < numAttachments; i++ ) {
		cmd->state.pipelineLayout.colorFormats[i] = colorformats[i];
	}
	cmd->state.pipelineLayout.depthFormat = depthFormat;

	cmd->state.numColorAttachments = numAttachments;
	memcpy( cmd->state.colorAttachment, colorAttachments, sizeof( struct NriDescriptor * ) * numAttachments );
	memcpy( cmd->state.viewports, viewports, sizeof( NriViewport ) * numAttachments );
	memcpy( cmd->state.scissors, scissors, sizeof( NriRect ) * numAttachments );
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
	
}

static inline bool __isValidFog( const mfog_t *fog )
{
	return fog && fog->shader;
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

// struct block_buffer_pool_req_s FR_ShaderObjReqCB(struct frame_cmd_buffer_s *cmd, const struct ObjectCB* cb)
//{
//	const hash_t hash = hash_data(HASH_INITIAL_VALUE, cb, sizeof(struct ObjectCB));
//	if( cmd->objHash  != hash ) {
//		struct block_buffer_pool_req_s poolReq = BlockBufferPoolReq( &rsh.nri, &cmd->uboBlockBuffer, sizeof( struct ObjectCB ) );
//		memcpy(poolReq.address, cb, sizeof(struct ObjectCB));
//		cmd->objHash = hash;
//		cmd->objBlock = poolReq;
//	}
//	return cmd->objBlock;
// }
//
// struct block_buffer_pool_req_s FR_ShaderFrameReqCB( struct frame_cmd_buffer_s *cmd, const struct FrameCB* cb)
//{
//
//	const hash_t hash = hash_data(HASH_INITIAL_VALUE, cb, sizeof(struct FrameCB));
//	if( cmd->frameHash != hash ) {
//		struct block_buffer_pool_req_s poolReq = BlockBufferPoolReq( &rsh.nri, &cmd->uboBlockBuffer, sizeof( struct FrameCB ) );
//		memcpy(poolReq.address, cb, sizeof(struct FrameCB  ));
//		cmd->frameBlock = poolReq;
//		cmd->frameHash = hash;
//	}
//	return cmd->frameBlock;
//
// }

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
		rsh.nri.coreI.CmdSetViewports( cmd->cmd, cmd->state.viewports, cmd->state.numColorAttachments );
		cmd->state.dirty &= ~CMD_DIRT_VIEWPORT;
	}

	if(cmd->state.dirty & CMD_DIRT_SCISSOR) {
		rsh.nri.coreI.CmdSetScissors( cmd->cmd, cmd->state.scissors, cmd->state.numColorAttachments );
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

	NriAttachmentsDesc attachmentsDesc = {};
	attachmentsDesc.colorNum = cmd->state.numColorAttachments;
	attachmentsDesc.colors = cmd->state.colorAttachment;
	attachmentsDesc.depthStencil = cmd->state.depthAttachment;
	rsh.nri.coreI.CmdBeginRendering( cmd->cmd, &attachmentsDesc );
	rsh.nri.coreI.CmdSetViewports( cmd->cmd, cmd->state.viewports, cmd->state.numColorAttachments );
	rsh.nri.coreI.CmdSetScissors( cmd->cmd, cmd->state.scissors, cmd->state.numColorAttachments );
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
