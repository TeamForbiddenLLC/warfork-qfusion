#ifndef R_FRAME_CMD_BUFFER_H
#define R_FRAME_CMD_BUFFER_H

#include "r_nri.h"
#include "r_resource.h"
#include "r_vattribs.h"
#include "ri_scratch_alloc.h"

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"

#include "../gameshared/q_sds.h"
#include "r_graphics.h"

#include "qhash.h"
#include "ri_format.h"

#define POGO_BUFFER_TEXTURE_FORMAT RI_FORMAT_RGBA8_UNORM

typedef struct mesh_vbo_s mesh_vbo_t;
typedef struct mfog_s mfog_t;
typedef struct entity_s entity_t;
typedef struct shader_s shader_t;

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

//struct frame_cmd_save_attachment_s {
//	uint32_t numColorAttachments;
//	NriFormat colorFormats[MAX_COLOR_ATTACHMENTS];
//	NriDescriptor const *colorAttachment[MAX_COLOR_ATTACHMENTS];
//	NriRect scissors[MAX_COLOR_ATTACHMENTS];
//	NriViewport viewports[MAX_COLOR_ATTACHMENTS];
//	NriDescriptor const *depthAttachment;
//};

struct frame_cmd_vertex_attrib_s {
	uint32_t offset;
	uint32_t format; // RI_Format_e
	uint16_t streamIndex;
	struct {
		uint16_t location;
	} vk;
};

struct frame_cmd_vertex_stream_s {
	uint16_t stride;
	uint16_t bindingSlot;
};

struct draw_element_s {
	uint32_t firstVert;
	uint32_t numVerts;
	uint32_t firstElem;
	uint32_t numElems;
	uint32_t numInstances;
};

struct pipeline_desc_s {
	// struct pipeline_state_s {
	uint8_t numColorsAttachments;
	RI_Format colorAttachments[MAX_COLOR_ATTACHMENTS];
	RI_Format depthFormat; // RI_Format_e

	// R - minimum resolvable difference
	// S - maximum slope
	// bias = constant * R + slopeFactor * S
	// if (clamp > 0)
	// 		bias = min(bias, clamp)
	// else if (clamp < 0)
	// 		bias = max(bias, clamp)
	// enabled if constant != 0 or slope != 0
	struct frame_pipeline_depth_bias_s {
		float constant;
		float clamp;
		float slope;
	} depthBias;

	uint16_t colorBlendEnabled : 1;
	uint16_t depthWrite : 1;
	uint16_t flippedViewport : 1; // bodge for portals ...
	uint16_t hasDepthAttachment : 1;
	uint16_t colorWriteMask : 4; // RIColorWriteMask_e
	uint16_t colorSrcFactor : 5; // RIBlendFactor_e
	uint16_t colorDstFactor : 5; // RIBlendFactor_e
	uint16_t topology : 4;		 // RITopology_e
	uint16_t cullMode : 3;		 // RICullMode_e
	uint16_t compareFunc : 4;	 // RICompareFunc_e
	
	size_t numStreams;
	struct frame_cmd_vertex_stream_s streams[MAX_STREAMS];
	
	size_t numAttribs;
	struct frame_cmd_vertex_attrib_s attribs[MAX_ATTRIBUTES];
};

struct frame_cmd_buffer_s {
	struct RIDevice_s* device;
	struct RICmdHandle_s handle;

 	//union {
  //  #if(DEVICE_IMPL_VULKAN)
  //  struct {
  //  	VkCommandBuffer cmd;
  //  } vk;
	//	#endif
  //};
	uint64_t frameCount; // the current frame index 
	
	// default global ubo for the scene
	struct RIDescriptor_s uboSceneFrame;
	struct RIDescriptor_s uboSceneObject;
	struct RIDescriptor_s uboPassObject;
	struct RIDescriptor_s uboBoneObject;
	struct RIDescriptor_s uboLight;

	// additional frame state
	struct draw_element_s drawElements;
	struct draw_element_s drawShadowElements;

	// cmd buffer state 
	uint32_t dirty;

	struct RIRect_s scissor;
	struct RIViewport_s viewport;

	struct RIBufferHandle_s vertexBuffers[MAX_VERTEX_BINDINGS];
	uint64_t offsets[MAX_VERTEX_BINDINGS];
	uint32_t dirtyVertexBuffers;
	
	struct RIBufferHandle_s indexBuffer;

	uint64_t indexBufferOffset;
	uint16_t indexType; // RIIndexType_e
	
	struct pipeline_desc_s pipeline;
};

// struct frame_cmd_save_attachment_s R_CmdState_StashAttachment(struct frame_cmd_buffer_s* cmd);
// void R_CmdState_RestoreAttachment(struct frame_cmd_buffer_s* cmd, const struct frame_cmd_save_attachment_s* stashed);

void FrameCmdBufferFree( struct frame_cmd_buffer_s *cmd );
void ResetFrameCmdBuffer(struct frame_cmd_buffer_s* cmd);
void UpdateFrameUBO( struct frame_cmd_buffer_s *cmd, struct RIDescriptor_s *frame, void *data, size_t size );

// try to commit the current frame ubo if its the same we will re-use this descriptor
void TryCommitFrameUBOInstance( struct RIDevice_s *device, struct frame_cmd_buffer_s *cmd, struct RIDescriptor_s* desc, void *data, size_t size );

// cmd buffer
static inline int FR_CmdNumViewports(struct frame_cmd_buffer_s* cmd) {
	return Q_MAX(cmd->pipeline.numColorsAttachments, 1);
}
//void FR_CmdResetAttachmentToBackbuffer(struct frame_cmd_buffer_s *cmd);
//void FR_CmdSetTextureAttachment( struct frame_cmd_buffer_s *cmd, 
//																const NriFormat *colorformats, 
//																const NriDescriptor **colorAttachments, 
//																const NriViewport* viewports, 
//																const NriRect* scissors, 
//																size_t numAttachments, 
//																const NriFormat depthFormat, 
//																NriDescriptor *depthAttachment );

void FR_CmdResetCmdState(struct frame_cmd_buffer_s *cmd,enum CmdStateResetBits bits);
void FR_CmdSetVertexBuffer( struct frame_cmd_buffer_s *cmd, uint32_t slot, struct RIBufferHandle_s* buffer, uint64_t offset );
void FR_CmdSetIndexBuffer( struct frame_cmd_buffer_s *cmd, struct RIBufferHandle_s* buffer, uint64_t offset, enum RIIndexType_e indexType );
void FR_CmdResetCommandState(struct frame_cmd_buffer_s *cmd, enum CmdResetBits bits);

//void FR_CmdSetScissor( struct frame_cmd_buffer_s *cmd, const struct RIRect_s *scissors, size_t numAttachments );
void FR_CmdSetScissor( struct frame_cmd_buffer_s *cmd, const struct RIRect_s scissors );
void FR_CmdSetDepthRangeAll(struct frame_cmd_buffer_s *cmd, float depthMin, float depthMax);

void FR_CmdSetViewport( struct frame_cmd_buffer_s *cmd, const struct RIViewport_s scissors );

void FR_CmdDraw( struct frame_cmd_buffer_s *cmd, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance); 
void FR_CmdDrawElements( struct frame_cmd_buffer_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance );

void FR_CmdBeginRendering(struct frame_cmd_buffer_s* cmd);
void FR_CmdEndRendering(struct frame_cmd_buffer_s* cmd);

//// frame immediate buffer
//
//struct gpu_frame_ele_allocator_s {
//  int16_t tail;
//  int16_t head;
// 	uint16_t elementStride; 
//  
//  size_t numElements;
//  size_t elementOffset;
//  struct {
//    uint64_t frameNum;
//    size_t numElements; // size of the generation
//  } segment[NUMBER_FRAMES_FLIGHT + 2]; // allocations are managed in segments
//  
//  VkBuffer buffer;
//  struct RIDescriptor_s bufferView;
//
//  NriBuffer* buffer;
//  NriMemory* memory;
//	NriBufferUsageBits usageBits;
//};
//struct gpu_frame_ele_ring_desc_s {
//	uint32_t numElements;
//	uint16_t elementStride;
//	NriBufferUsageBits usageBits;
//};
//struct gpu_frame_ele_req_s{
//	NriBuffer *buffer;
//	uint16_t elementStride;
//	uint32_t elementOffset;
//	uint32_t numElements;
//};
//void initGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *ringAlloc, const struct gpu_frame_ele_ring_desc_s *desc );
//void freeGPUFrameEleAlloc( struct gpu_frame_ele_allocator_s *alloc );
//void GPUFrameEleAlloc( struct frame_cmd_buffer_s *cmd, struct gpu_frame_ele_allocator_s* alloc, size_t numElements, struct gpu_frame_ele_req_s *req );
//static inline bool IsElementBufferContinous(size_t currentOffset, size_t currentNumElements, size_t nextOffset)
//{
//	return currentOffset + currentNumElements  == nextOffset;
//}

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




