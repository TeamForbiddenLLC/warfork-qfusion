#ifndef R_FRAME_CMD_BUFFER_H
#define R_FRAME_CMD_BUFFER_H

#include "r_model.h"
#include "r_nri.h"
#include "r_vattribs.h"
#include "r_block_buffer_pool.h"


typedef struct mesh_vbo_s mesh_vbo_t;

struct frame_cmd_vertex_input_s {
	NriBuffer *buffer;
	uint64_t offset;
};

// the serialized state of the pipeline
struct frame_cmd_state_s {
	uint32_t viewport[4];
	struct {
		uint16_t x;
		uint16_t y;
		uint16_t w;
		uint16_t h;
	} scissor;

  
  NriBuffer* vertexBuffers[MAX_VERTEX_BINDINGS];
  uint64_t offsets[MAX_VERTEX_BINDINGS];
	uint32_t dirtyVertexBuffers;

	// binding
	struct NriDescriptor *bindings[DESCRIPTOR_SET_MAX][DESCRIPTOR_MAX_BINDINGS];
};

struct frame_backbuffer_s {
	NriDescriptor *colorAttachment;
	NriTexture *texture;
	NriAccessLayoutStage currentLayout;
};

// hack to store some additional managed state 
struct frame_additional_data_s {
	struct {
		unsigned int firstVert;
		unsigned int numVerts;
		unsigned int firstElem;
		unsigned int numElems;
		unsigned int numInstances;
	} drawElements;
	struct {
		unsigned int firstVert;
		unsigned int numVerts;
		unsigned int firstElem;
		unsigned int numElems;
		unsigned int numInstances;
	} drawShadowElements;
	mfog_t* fog;
};

struct frame_cmd_buffer_s {
	uint8_t frameCount; // this value is bound by NUMBER_FRAMES_FLIGHT
	struct block_buffer_pool_s uboBlockBuffer; 
	struct frame_cmd_state_s cmdState;
	struct pipeline_layout_config_s layoutConfig;
	struct frame_backbuffer_s backBuffer;
	NriCommandAllocator *allocator;
	NriCommandBuffer *cmd;
	struct frame_additional_data_s additional;

	hash_t frameHash;
	struct block_buffer_pool_req_s frameCB; 
};


// I can't figure out frame state so I need a way to work out when
// to trash the cb and rebuild ...
struct block_buffer_pool_req_s FR_FrameRequestCB(struct frame_cmd_buffer_s* cmd); 

// cmd buffer 
void FR_CmdSetVertexInput( struct frame_cmd_buffer_s *cmd, uint32_t slot, NriBuffer *buffer, uint64_t offset );
void FR_CmdSetScissor(struct frame_cmd_buffer_s* cmd, int x, int y, int w, int h );
void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance );
#endif
