#include "r_frame_cmd_buffer.h"
#include "r_local.h"
#include "r_resource.h"
#include "r_hasher.h"
#include "stb_ds.h"

#include "r_model.h"

void FR_CmdSetScissor( struct frame_cmd_buffer_s *cmd, int x, int y, int w, int h )
{
	if(cmd->cmdState.scissor.x != x ||
		 cmd->cmdState.scissor.y != y ||
		 cmd->cmdState.scissor.w != w ||
		 cmd->cmdState.scissor.h != h ) {
		cmd->cmdState.dirty |= CMD_DIRTY_VIEWPORT;
	}

	cmd->cmdState.scissor.x = x;
	cmd->cmdState.scissor.y = y;
	cmd->cmdState.scissor.w = w;
	cmd->cmdState.scissor.h = h;
}

void FR_CmdResetAttachmentToBackbuffer( struct frame_cmd_buffer_s *cmd ) {
	const NriDescriptor *colorAttachments[] = { cmd->textureBuffers.colorAttachment };
	FR_CmdSetTextureAttachment( cmd, colorAttachments, 1, cmd->textureBuffers.depthAttachment );
}

void FR_CmdResetCmdState(struct frame_cmd_buffer_s *cmd,enum CmdStateResetBits bits) {
	memset(cmd->cmdState.vertexBuffers, 0, sizeof(NriBuffer*) * MAX_VERTEX_BINDINGS);
	cmd->cmdState.dirtyVertexBuffers = 0;
}


void FR_CmdSetTextureAttachment( struct frame_cmd_buffer_s *cmd, NriDescriptor **colorAttachments, size_t numColors, NriDescriptor *depthAttachment )
{
	cmd->cmdState.numColorAttachments = numColors;
	memcpy( cmd->cmdState.colorAttachment, colorAttachments, sizeof( struct NriDescriptor * ) * numColors );
	cmd->cmdState.depthAttachment = depthAttachment;
}

void FR_CmdSetIndexBuffer( struct frame_cmd_buffer_s *cmd, NriBuffer *buffer, uint64_t offset, NriIndexType indexType)
{
	cmd->cmdState.indexType = indexType;
	cmd->cmdState.dirtyIndexBuffer = true;
	cmd->cmdState.indexBufferOffset = offset;
	cmd->cmdState.indexBuffer = buffer;
}

void FR_CmdSetVertexBuffer( struct frame_cmd_buffer_s *cmd, uint32_t slot, NriBuffer *buffer, uint64_t offset )
{
	assert( slot < MAX_VERTEX_BINDINGS );
	if( cmd->cmdState.vertexBuffers[slot] == buffer && cmd->cmdState.offsets[slot] == offset ) {
		return;
	}
	cmd->cmdState.dirtyVertexBuffers |= ( 0x1 << slot );
	cmd->cmdState.offsets[slot] = offset;
	cmd->cmdState.vertexBuffers[slot] = buffer;
}

static inline bool __isValidFog(const mfog_t* fog) {
	return fog && fog->shader;
}

void UpdateFrameUBO(struct frame_cmd_buffer_s *cmd,struct ubo_frame_instance_s* ubo,void * data, size_t size) {
	const hash_t hash = hash_data( HASH_INITIAL_VALUE, data, size );
	if( ubo->hash != hash ) {
		struct block_buffer_pool_req_s poolReq = BlockBufferPoolReq( &rsh.nri, &cmd->uboBlockBuffer, sizeof( struct ObjectCB ) );
		memcpy( poolReq.address, data, size );
	
		NriBufferViewDesc bufferDesc = {
			.buffer = poolReq.buffer,
			.size = poolReq.bufferSize,
			.offset = poolReq.bufferOffset,
			.format = NriBufferViewType_CONSTANT
		};

		NriDescriptor* descriptor = NULL;
		NRI_ABORT_ON_FAILURE(rsh.nri.coreI.CreateBufferView(&bufferDesc, &descriptor));
		arrpush(cmd->frameTemporaryDesc, descriptor);
		ubo->descriptor = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
		ubo->hash = hash;
		ubo->req = poolReq;
	}
}

void ResetFrameCmdBuffer(struct nri_backend_s* backend,struct frame_cmd_buffer_s* cmd) {
	cmd->textureBuffers = rsh.backBuffers[rsh.nri.swapChainI.AcquireNextSwapChainTexture( rsh.swapchain )];
	for( size_t i = 0; i < arrlen( cmd->frameTemporaryDesc ); i++ ) {
		backend->coreI.DestroyDescriptor( cmd->frameTemporaryDesc[i] );
	}
	arrsetlen( cmd->frameTemporaryDesc, 0 );
	BlockBufferPoolReset( &cmd->uboBlockBuffer );
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
	for( uint32_t attr = cmd->cmdState.dirtyVertexBuffers; attr > 0; attr = ( attr >> 1 ), vertexSlot++ ) {
		if( cmd->cmdState.dirtyVertexBuffers & ( 1 << vertexSlot ) ) {
			const NriBuffer *buffer[1] = { cmd->cmdState.vertexBuffers[vertexSlot] };
			const uint64_t offset[1] = { cmd->cmdState.offsets[vertexSlot] };
			rsh.nri.coreI.CmdSetVertexBuffers( cmd->cmd, vertexSlot, 1, buffer, offset );
		}
	}

	if(cmd->cmdState.dirtyIndexBuffer) {
		rsh.nri.coreI.CmdSetIndexBuffer(cmd->cmd, cmd->cmdState.indexBuffer, cmd->cmdState.indexBufferOffset, cmd->cmdState.indexType);
	}

	cmd->cmdState.dirtyIndexBuffer = false;
	cmd->cmdState.dirtyVertexBuffers = 0;

	if( cmd->cmdState.dirty & CMD_DIRTY_VIEWPORT ) {
		const NriViewport viewport = { cmd->cmdState.viewport[0], cmd->cmdState.viewport[1], cmd->cmdState.viewport[2], cmd->cmdState.viewport[3], 0, 1.0f };
		rsh.nri.coreI.CmdSetViewports( cmd->cmd, &viewport, 1 );
		cmd->cmdState.dirty &= ~CMD_DIRTY_VIEWPORT;
	}

	NriDrawIndexedDesc drawDesc = { 0 };
	drawDesc.baseIndex = indexNum;
	drawDesc.instanceNum = instanceNum;
	drawDesc.baseIndex = baseIndex;
	drawDesc.baseVertex = baseVertex;
	drawDesc.baseInstance = baseInstance;
	rsh.nri.coreI.CmdDrawIndexed( cmd->cmd, &drawDesc );
}
