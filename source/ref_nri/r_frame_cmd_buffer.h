#ifndef R_FRAME_CMD_BUFFER_H
#define R_FRAME_CMD_BUFFER_H

#include "r_nri.h"
#include "r_vattribs.h"
#include "r_block_buffer_pool.h"


typedef struct mesh_vbo_s mesh_vbo_t;

#define DESCRIPTOR_MAX_BINDINGS 32
#define MAX_VERTEX_BINDINGS 24 

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
	// pipeline state

	//size_t numSlots;
  //size_t numVertexInputs;
  
  NriBuffer* vertexBuffers[MAX_VERTEX_BINDINGS];
  uint64_t offsets[MAX_VERTEX_BINDINGS];
	uint32_t dirtyVertexBuffers;


	// binding
	struct NriDescriptor *bindings[DESCRIPTOR_SET_MAX][DESCRIPTOR_MAX_BINDINGS];
	uint32_t dirtyDescriptorSets; // marks descriptor sets that are dirty
};

struct pipeline_state_s {
	NriCullMode cullMode;
	vattrib_t attrib;	
	vattrib_t halfAttrib;	
};

struct backbuffer_s {
	NriDescriptor *colorAttachment;
	NriTexture *texture;
	NriAccessLayoutStage currentLayout;
};

// hack to store some additional managed state 
struct frame_additional_data {
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
};

struct frame_cmd_buffer_s {
	struct block_buffer_pool_s uboBlockBuffer; 
	struct frame_cmd_state_s cmdState;
	struct pipeline_state_s layoutState;
	struct backbuffer_s backBuffer;
	NriCommandAllocator *allocator;
	NriCommandBuffer *cmd;
	
	struct frame_additional_data additional;
};

// cmd buffer 
void FR_CmdSetOpqaueState( struct frame_cmd_buffer_s *cmd );
void FR_CmdSetDefaultState( struct frame_cmd_buffer_s *cmd );
void FR_CmdSetTexture( struct frame_cmd_buffer_s *cmd, uint32_t set, uint32_t binding, NriDescriptor *texture );
void FR_CmdSetVertexInput( struct frame_cmd_buffer_s *cmd, uint32_t slot, NriBuffer *buffer, uint64_t offset );
void FR_CmdSetScissor(struct frame_cmd_buffer_s* cmd, int x, int y, int w, int h );

// pipeline state
void FR_SetPipelineSetCull( struct frame_cmd_buffer_s *cmd,NriCullMode mode );
void FR_SetPipelineVertexAttrib( struct frame_cmd_buffer_s *cmd, vattrib_t attrib, vattrib_t halfAttrib );

void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance);
#endif
