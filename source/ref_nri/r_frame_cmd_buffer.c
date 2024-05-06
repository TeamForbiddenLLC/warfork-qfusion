#include "r_frame_cmd_buffer.h"
#include "r_local.h"

void FR_SetTexture(struct frame_cmd_buffer_s* cmd, uint32_t set, uint32_t binding, NriDescriptor* texture) {
  assert(cmd);
}


void FR_ResetCmdState(struct frame_cmd_buffer_s* cmd) {
  memset(&cmd->state, 0, sizeof(struct frame_cmd_state_s));
}


