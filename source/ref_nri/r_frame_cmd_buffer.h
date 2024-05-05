#ifndef R_FRAME_CMD_BUFFER_H
#define R_FRAME_CMD_BUFFER_H

#include "r_nri.h"
#include "r_vattribs.h"

#define DESCRIPTOR_MAX_BINDINGS 32


//enum BindingType { BINDING_TYPE_2D_VIEW };

// the serialized state of the pipeline
// 
struct frame_cmd_state_s {
  // pipeline state
  vattrib_t attrib;
  NriCullMode cullMode;

  // binding
  struct NriDescriptor* bindings[DESCRIPTOR_SET_MAX][DESCRIPTOR_MAX_BINDINGS];
  uint32_t dirtBits; // marks descriptor sets that are dirty
};

struct pipline_compile_s {
};

struct frame_cmd_buffer_s {
  NriCommandAllocator* allocator;
  NriCommandBuffer* buffer;
  struct frame_cmd_state_s state;
};

void FR_PipelineSetCull(NriCullMode mode);

void FR_SetTexture(struct frame_cmd_buffer_s* cmd, uint32_t set, uint32_t binding, NriDescriptor* texture);

#endif
