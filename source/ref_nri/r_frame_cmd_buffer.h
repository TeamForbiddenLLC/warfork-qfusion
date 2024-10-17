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

enum CmdStateResetBits {
  STATE_RESET_VERTEX_ATTACHMENT
};
enum CmdResetBits {
	CMD_RESET_INDEX_BUFFER = 0x1,
	CMD_RESET_VERTEX_BUFFER = 0x2,
};

enum CmdStateDirtyBits {
	CMD_DIRT_INDEX_BUFFER = 0x1,
	CMD_DIRT_VIEWPORT = 0x2,
	CMD_DIRT_SCISSOR = 0x4,
};

// the serialized state of the pipeline
struct frame_cmd_state_s {
	uint32_t dirty;

	size_t numStreams;
	NriVertexStreamDesc streams[MAX_STREAMS];
	size_t numAttribs;
	NriVertexAttributeDesc attribs[MAX_ATTRIBUTES];

	uint32_t numColorAttachments;
	NriDescriptor const *colorAttachment[MAX_COLOR_ATTACHMENTS];
	NriRect scissors[MAX_COLOR_ATTACHMENTS];
	NriViewport viewports[MAX_COLOR_ATTACHMENTS];
	NriDescriptor const *depthAttachment;

	NriBuffer *vertexBuffers[MAX_VERTEX_BINDINGS];
	uint64_t offsets[MAX_VERTEX_BINDINGS];
	uint32_t dirtyVertexBuffers;

	NriBuffer *indexBuffer;
	uint64_t indexBufferOffset;
	NriIndexType indexType;

	// binding
	struct NriDescriptor *bindings[DESCRIPTOR_SET_MAX][DESCRIPTOR_MAX_BINDINGS];

	struct {
		NriFormat colorFormats[MAX_COLOR_ATTACHMENTS];
		NriFormat depthFormat;

		bool blendEnabled;
		NriCullMode cullMode;
		NriBlendFactor colorSrcFactor;
		NriBlendFactor colorDstFactor;

		float depthRangeMin;
		float depthRangeMax;
		NriColorWriteBits colorWriteMask;
		NriCompareFunc compareFunc;
		bool depthWrite;
	} pipelineLayout;
};

struct frame_tex_buffers_s {
	NriRect screen; 	
	NriDescriptor *colorAttachment;
	NriTexture *colorTexture;

	// used for post processing
	struct pogo_buffers_s {
		NriDescriptor *colorAttachment;
		NriTexture *colorTexture;
	} pogoBuffers[2];

	NriDescriptor *depthAttachment;
	NriTexture* depthTexture;

	NriAccessLayoutStage currentLayout;

	size_t memoryLen;
	NriMemory* memory[8];
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
	uint64_t frameCount; // this value is bound by NUMBER_FRAMES_FLIGHT
	struct block_buffer_pool_s uboBlockBuffer; 
	struct frame_cmd_state_s state;
	struct frame_tex_buffers_s textureBuffers;
	
	NriCommandAllocator *allocator;
	NriCommandBuffer *cmd;

	NriDescriptor** frameTemporaryDesc; // temporary frame descriptors that are recycled at the end of the frame	

	// list of objects to free
	NriTexture**  freeTextures;
	NriMemory** freeMemory;

	// default global ubo for the scene
	struct ubo_frame_instance_s uboSceneFrame;
	struct ubo_frame_instance_s uboSceneObject;
	struct ubo_frame_instance_s uboPassObject;

	// additional frame state
	struct draw_element_s drawElements;
	struct draw_element_s drawShadowElements;

	int stackCmdBeingRendered;
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
void FR_CmdResetAttachmentToBackbuffer(struct frame_cmd_buffer_s *cmd);
void FR_CmdSetTextureAttachment( struct frame_cmd_buffer_s *cmd, 
																const NriFormat *colorformats, 
																const NriDescriptor **colorAttachments, 
																const NriViewport* viewports, 
																const NriRect* scissors, 
																size_t numAttachments, 
																const NriFormat depthFormat, 
																NriDescriptor *depthAttachment );

void FR_CmdResetCmdState(struct frame_cmd_buffer_s *cmd,enum CmdStateResetBits bits);
void FR_CmdSetVertexBuffer( struct frame_cmd_buffer_s *cmd, uint32_t slot, NriBuffer *buffer, uint64_t offset );
void FR_CmdSetIndexBuffer( struct frame_cmd_buffer_s *cmd, NriBuffer *buffer, uint64_t offset, NriIndexType indexType );
void FR_CmdResetCommandState(struct frame_cmd_buffer_s *cmd, enum CmdResetBits bits);

void FR_CmdSetScissor( struct frame_cmd_buffer_s *cmd, const NriRect *scissors, size_t numAttachments ); 
void FR_CmdSetScissorAll( struct frame_cmd_buffer_s *cmd, const NriRect scissors); 

void FR_CmdSetViewportAll( struct frame_cmd_buffer_s *cmd, const NriViewport scissors );

void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance );

void FR_CmdBeginRendering(struct frame_cmd_buffer_s* cmd);
void FR_CmdEndRendering(struct frame_cmd_buffer_s* cmd);

#endif
