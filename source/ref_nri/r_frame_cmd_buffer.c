#include "r_frame_cmd_buffer.h"
#include "r_local.h"


//void FR_ResetCmdState(struct frame_cmd_buffer_s* cmd) {
//  memset(&cmd->state, 0, sizeof(struct frame_cmd_state_s));
//}

void FR_CmdSetOpqaueState(struct frame_cmd_buffer_s* cmd) {

}
void FR_CmdSetDefaultState(struct frame_cmd_buffer_s* cmd) {

}

void FR_CmdSetScissor(struct frame_cmd_buffer_s* cmd, int x, int y, int w, int h ) {
  cmd->cmdState.scissor.x = x;
  cmd->cmdState.scissor.y = y;
  cmd->cmdState.scissor.w = w;
  cmd->cmdState.scissor.h = h;
}

void FR_CmdSetVertexInput(struct frame_cmd_buffer_s* cmd, uint32_t slot, NriBuffer* buffer, uint64_t offset) {
	assert( slot < MAX_VERTEX_BINDINGS );
	if( cmd->cmdState.vertexBuffers[slot] == buffer && cmd->cmdState.offsets[slot] == offset ) {
		return;
	}
	cmd->cmdState.dirtyVertexBuffers |= ( 0x1 << slot );
	cmd->cmdState.offsets[slot] = offset;
	cmd->cmdState.vertexBuffers[slot] = buffer;
}

void FR_CmdSetTexture(struct frame_cmd_buffer_s* cmd, uint32_t set, uint32_t binding, NriDescriptor* texture) {

}

void FR_SetPipelineSetCull( struct frame_cmd_buffer_s *cmd, NriCullMode mode ) {
  cmd->layoutState.cullMode = mode;
}

void FR_SetPipelineVertexAttrib(struct frame_cmd_buffer_s* cmd,vattrib_t attrib, vattrib_t halfAttrib) {
  cmd->layoutState.attrib = attrib;
  cmd->layoutState.halfAttrib = halfAttrib;
}

void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance) {
  
  uint32_t vertexSlot = 0;
  for( uint32_t attr = cmd->cmdState.dirtyVertexBuffers; attr > 0; attr = ( attr >> 1 ), vertexSlot++ ) {
	  if( cmd->cmdState.dirtyVertexBuffers & ( 1 << vertexSlot ) ) {
		  const NriBuffer *buffer[1] = { cmd->cmdState.vertexBuffers[vertexSlot] };
		  const uint64_t offset[1] = { cmd->cmdState.offsets[vertexSlot] };
		  rsh.nri.coreI.CmdSetVertexBuffers( cmd->cmd, vertexSlot, 1, buffer, offset );
	  }
  }
  cmd->cmdState.dirtyVertexBuffers = 0;

  NriDrawIndexedDesc drawDesc = {0};
  drawDesc.baseIndex = indexNum;
  drawDesc.instanceNum = instanceNum;
  drawDesc.baseIndex = baseIndex;
  drawDesc.baseVertex = baseVertex;
  drawDesc.baseInstance = baseInstance;
  rsh.nri.coreI.CmdDrawIndexed(cmd->cmd, &drawDesc);
}
