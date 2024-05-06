#ifndef R_FRAME_CMD_BUFFER_H
#define R_FRAME_CMD_BUFFER_H

#include "r_nri.h"
#include "r_vattribs.h"

typedef struct mesh_vbo_s mesh_vbo_t;

#define DESCRIPTOR_MAX_BINDINGS 32
#define MAX_VERTEX_BINDINGS 16
// enum BindingType { BINDING_TYPE_2D_VIEW };

struct frame_cmd_vertex_input_s {
	NriBuffer *buffer;
	uint32_t slot;
	uint64_t offset;
};

// the serialized state of the pipeline
struct frame_cmd_state_s {
	// pipeline state
	vattrib_t attrib;
	NriCullMode cullMode;

    size_t numVertexInputs;
	struct frame_cmd_vertex_input_s vertexInputs[MAX_VERTEX_BINDINGS];

	// binding
	struct NriDescriptor *bindings[DESCRIPTOR_SET_MAX][DESCRIPTOR_MAX_BINDINGS];
	uint32_t dirtBits; // marks descriptor sets that are dirty
};

struct pipline_compile_s {
};

struct backbuffer_s {
	NriDescriptor *colorAttachment;
	NriTexture *texture;
	NriAccessLayoutStage currentLayout;
};

struct frame_cmd_buffer_s {
	struct frame_cmd_state_s state;
	struct backbuffer_s backBuffer;
	NriCommandAllocator *allocator;
	NriCommandBuffer *cmd;
};

void FR_PipelineSetCull( NriCullMode mode );


void FR_ResetCmdState(struct frame_cmd_buffer_s* cmd);
void FR_SetTexture(struct frame_cmd_buffer_s* cmd, uint32_t set, uint32_t binding, NriDescriptor* texture);

#endif
