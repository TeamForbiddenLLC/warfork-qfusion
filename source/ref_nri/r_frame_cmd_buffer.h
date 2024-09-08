#ifndef R_FRAME_CMD_BUFFER_H
#define R_FRAME_CMD_BUFFER_H

#include "r_nri.h"
#include "r_resource.h"
#include "r_vattribs.h"
#include "r_block_buffer_pool.h"

typedef struct mesh_vbo_s mesh_vbo_t;
typedef struct mfog_s mfog_t;
typedef struct entity_s entity_t;
typedef struct shader_s shader_t;

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


struct draw_element_s {
	uint32_t firstVert;
	uint32_t numVerts;
	uint32_t firstElem;
	uint32_t numElems;
	uint32_t numInstances;
};

struct ubo_frame_instance_s {
	hash_t hash;
	struct nri_descriptor_s descriptor;
	struct block_buffer_pool_req_s req; 
};

struct frame_cmd_buffer_s {
	uint8_t frameCount; // this value is bound by NUMBER_FRAMES_FLIGHT
	struct block_buffer_pool_s uboBlockBuffer; 
	struct frame_cmd_state_s cmdState;
	struct pipeline_layout_config_s layoutConfig;
	struct frame_backbuffer_s backBuffer;
	NriCommandAllocator *allocator;
	NriCommandBuffer *cmd;

	NriDescriptor** frameTemporaryDesc; // temporary frame descriptors that are recycled at the end of the frame	

	// default global ubo for the scene
	struct ubo_frame_instance_s uboSceneFrame;	
	struct ubo_frame_instance_s uboSceneObject;	

	// additional frame state
	struct draw_element_s drawElements;
	struct draw_element_s drawShadowElements;
};

void ResetFrameCmdBuffer(struct nri_backend_s* backend,struct frame_cmd_buffer_s* cmd);
void UpdateFrameUBO( struct frame_cmd_buffer_s *cmd, struct ubo_frame_instance_s *frame, void *data, size_t size );

struct frame_buffer_req_s {
	struct block_buffer_pool_req_s frame;
	struct block_buffer_pool_req_s obj;
};

// I can't figure out frame state so I need a way to work out when
// to trash the cb and rebuild ...
// this is basd off the additiona/obj
//struct block_buffer_pool_req_s FR_ShaderObjReqCB(struct frame_cmd_buffer_s *cmd, const struct ObjectCB* cb); 
//// this is basd off the additiona/frame
//struct block_buffer_pool_req_s FR_ShaderFrameReqCB( struct frame_cmd_buffer_s *cmd, const struct FrameCB* cb);

// cmd buffer 
void FR_CmdSetVertexInput( struct frame_cmd_buffer_s *cmd, uint32_t slot, NriBuffer *buffer, uint64_t offset );
void FR_CmdSetScissor(struct frame_cmd_buffer_s* cmd, int x, int y, int w, int h );
void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance );
#endif