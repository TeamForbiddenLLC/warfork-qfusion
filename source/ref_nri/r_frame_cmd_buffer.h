#ifndef R_FRAME_CMD_BUFFER_H
#define R_FRAME_CMD_BUFFER_H

#include "NRIDescs.h"
#include "r_nri.h"
#include "r_resource.h"
#include "r_vattribs.h"
#include "r_block_buffer_pool.h"

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"

#include "../gameshared/q_sds.h"


#include "qhash.h"

#define POGO_BUFFER_TEXTURE_FORMAT NriFormat_RGBA8_UNORM

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
	CMD_RESET_DEFAULT_PIPELINE_LAYOUT = 0x4
};

enum CmdStateDirtyBits {
	CMD_DIRT_INDEX_BUFFER = 0x1,
	CMD_DIRT_VIEWPORT = 0x2,
	CMD_DIRT_SCISSOR = 0x4,
};

struct frame_cmd_save_attachment_s {
	uint32_t numColorAttachments;
	NriFormat colorFormats[MAX_COLOR_ATTACHMENTS];
	NriDescriptor const *colorAttachment[MAX_COLOR_ATTACHMENTS];
	NriRect scissors[MAX_COLOR_ATTACHMENTS];
	NriViewport viewports[MAX_COLOR_ATTACHMENTS];
	NriDescriptor const *depthAttachment;
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

	struct pipeline_state_s {
		NriFormat colorFormats[MAX_COLOR_ATTACHMENTS];
		NriFormat depthFormat;
	
		NriDepthBiasDesc depthBias;

		bool blendEnabled;
		NriTopology topology;
		NriCullMode cullMode;
		NriBlendFactor colorSrcFactor;
		NriBlendFactor colorDstFactor;

		NriColorWriteBits colorWriteMask;
		NriCompareFunc compareFunc;
		bool depthWrite;
		
		bool flippedViewport; // bodge for portals ... 
	} pipelineLayout;
};


struct frame_tex_buffers_s {
	NriDescriptor* postProcessingSampler;

	NriRect screen; 	
	NriDescriptor *colorAttachment;
	NriTexture *colorTexture;
	
	NriDescriptor *depthAttachment;
	NriTexture* depthTexture;

	// used for post processing
	struct pogo_buffers_s {
		uint8_t isAttachment:1;
		NriDescriptor *colorAttachment;
		struct nri_descriptor_s shaderDescriptor;
		NriTexture *colorTexture;
	} pogoBuffers[2];

	NriAccessLayoutStage currentLayout;
	size_t memoryLen;
	NriMemory* memory[10];
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

	// list of objects to free
	NriDescriptor** frameTemporaryDesc; // temporary frame descriptors that are recycled at the end of the frame	
	NriTexture**  freeTextures;
	NriBuffer** freeBuffers;
	NriMemory** freeMemory;

	// default global ubo for the scene
	struct ubo_frame_instance_s uboSceneFrame;
	struct ubo_frame_instance_s uboSceneObject;
	struct ubo_frame_instance_s uboPassObject;
	struct ubo_frame_instance_s uboBoneObject;
	struct ubo_frame_instance_s uboLight;

	// additional frame state
	struct draw_element_s drawElements;
	struct draw_element_s drawShadowElements;

	int stackCmdBeingRendered;
};


struct frame_cmd_save_attachment_s R_CmdState_StashAttachment(struct frame_cmd_buffer_s* cmd);
void R_CmdState_RestoreAttachment(struct frame_cmd_buffer_s* cmd, const struct frame_cmd_save_attachment_s* stashed);

void FrameCmdBufferFree(struct frame_cmd_buffer_s* cmd);
void ResetFrameCmdBuffer(struct nri_backend_s* backend,struct frame_cmd_buffer_s* cmd);
void UpdateFrameUBO( struct frame_cmd_buffer_s *cmd, struct ubo_frame_instance_s *frame, void *data, size_t size );

// cmd buffer
static inline int FR_CmdNumViewports(struct frame_cmd_buffer_s* cmd) {
	return max(cmd->state.numColorAttachments, 1);
}
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

void FR_CmdDraw( struct frame_cmd_buffer_s *cmd, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance); 
void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance );

void FR_CmdBeginRendering(struct frame_cmd_buffer_s* cmd);
void FR_CmdEndRendering(struct frame_cmd_buffer_s* cmd);

void TransitionPogoBufferToShaderResource(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo);
void TransitionPogoBufferToAttachment(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo);

void FR_BindPogoBufferAttachment(struct frame_cmd_buffer_s* frame, struct pogo_buffers_s* pogo);

// frame immediate buffer

struct gpu_frame_ele_allocator_s {
  int16_t tail;
  int16_t head;
 	uint16_t elementStride; 
  
  size_t numElements;
  size_t elementOffset;
  struct {
    uint64_t frameNum;
    size_t numElements; // size of the generation
  } segment[NUMBER_FRAMES_FLIGHT + 2]; // allocations are managed in segments
  NriBuffer* buffer;
  NriMemory* memory;
	NriBufferUsageBits usageBits;
};
struct gpu_frame_ele_ring_desc_s {
	uint32_t numElements;
	uint16_t elementStride;
	NriBufferUsageBits usageBits;
};
struct gpu_frame_ele_req_s{
	NriBuffer *buffer;
	uint16_t elementStride;
	uint32_t elementOffset;
	uint32_t numElements;
};
void initGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *ringAlloc, const struct gpu_frame_ele_ring_desc_s *desc );
void freeGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *alloc );
void GPUFrameEleAlloc( struct frame_cmd_buffer_s *cmd, struct gpu_frame_ele_allocator_s* alloc, size_t numElements, struct gpu_frame_ele_req_s *req );
static inline bool IsElementBufferContinous(size_t currentOffset, size_t currentNumElements, size_t nextOffset)
{
	return currentOffset + currentNumElements  == nextOffset;
}

//struct gpu_frame_allocator_s {
//  int16_t tail;
//  int16_t head;
//  size_t allocSize;
//  size_t tailOffset;
//  struct {
//    uint64_t frameNum;
//    size_t allocSize; // size of the generation
//  } segment[NUMBER_FRAMES_FLIGHT + 1]; // allocations are managed in segments
//  NriBuffer* buffer;
//  NriMemory* memory;
//
//	size_t bufferAlignment;
//	NriBufferUsageBits usageBits;
//};
//
//struct gpu_frame_buffer_req_s {
//	NriBuffer *buffer;
//	size_t bufferOffset;
//	size_t bufferSize;
//};
//
//struct gpu_frame_buffer_ring_desc_s {
//	size_t reservedSize;
//	size_t alignmentReq;
//	NriBufferUsageBits usageBits;
//};
//
//void initGPUFrameAlloc(struct gpu_frame_allocator_s* ringAlloc, const struct gpu_frame_buffer_ring_desc_s* desc);
//void freeGPUFrameAlloc(struct gpu_frame_allocator_s* alloc);
//void GPUFrameAlloc( struct frame_cmd_buffer_s* cmd, struct gpu_frame_allocator_s *alloc, size_t reqSize, struct gpu_frame_buffer_req_s* req);

#endif




