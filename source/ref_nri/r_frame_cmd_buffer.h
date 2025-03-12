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
	RI_Format colorAttachments[MAX_COLOR_ATTACHMENTS]; // RI_Format_e
	RI_Format depthFormat; // RI_Format_e

	// R - minimum resolvable difference
	// S - maximum slope
	// bias = constant * R + slopeFactor * S
	// if (clamp > 0)
	// 		bias = min(bias, clamp)
	// else if (clamp < 0)
	// 		bias = max(bias, clamp)
	// enabled if constant != 0 or slope != 0
	float depthBiasConstant;
	float depthBiasClamp;
	float depthBiasSlope;
	uint16_t flippedViewport : 1; // bodge for portals ...
	uint16_t cullMode : 3;		  // RICullMode_e
	uint16_t colorBlendEnabled : 1;
	uint16_t depthWrite : 1;
	uint16_t colorWriteMask : 4; // RIColorWriteMask_e
	uint16_t colorSrcFactor : 5; // RIBlendFactor_e
	uint16_t colorDstFactor : 5; // RIBlendFactor_e
	uint16_t topology : 4;		 // RITopology_e
	uint16_t compareFunc : 4;	 // RICompareFunc_e

	size_t numStreams;
	struct frame_cmd_vertex_stream_s streams[MAX_STREAMS];
	
	size_t numAttribs;
	struct frame_cmd_vertex_attrib_s attribs[MAX_ATTRIBUTES];
};

struct FrameState_s {
	struct FrameState_s* parent; 
	struct RIDevice_s *device;
	struct RICmd_s handle;

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
	uint64_t pipelineBound;

	struct RIRect_s scissor;
	struct RIViewport_s viewport;

	struct RIBuffer_s vertexBuffers[MAX_VERTEX_BINDINGS];
	uint64_t offsets[MAX_VERTEX_BINDINGS];
	uint32_t dirtyVertexBuffers;

	struct RIBuffer_s indexBuffer;

	uint64_t indexBufferOffset;
	uint16_t indexType; // RIIndexType_e

	struct pipeline_desc_s pipeline;
};

void FrameCmdBufferFree( struct FrameState_s *cmd );
void ResetFrameCmdBuffer(struct FrameState_s* cmd);
void UpdateFrameUBO( struct FrameState_s *cmd, struct RIDescriptor_s *frame, void *data, size_t size );

// cmd buffer
static inline int FR_CmdNumViewports(struct FrameState_s* cmd) {
	return Q_MAX(cmd->pipeline.numColorsAttachments, 1);
}

void FR_ConfigureAttachment( struct pipeline_desc_s *pipelineDesc, enum RI_Format_e *formats, size_t numAttachment, enum RI_Format_e depthFormat );

void FR_CmdResetCmdState( struct FrameState_s *cmd, enum CmdStateResetBits bits );
void FR_CmdSetVertexBuffer( struct FrameState_s *cmd, uint32_t slot, struct RIBuffer_s* buffer, uint64_t offset );
void FR_CmdSetIndexBuffer( struct FrameState_s *cmd, struct RIBuffer_s* buffer, uint64_t offset, enum RIIndexType_e indexType );
void FR_CmdResetCommandState(struct FrameState_s *cmd, enum CmdResetBits bits);

void FR_CmdSetScissor( struct FrameState_s *cmd, const struct RIRect_s scissors );
void FR_CmdSetDepthRangeAll(struct FrameState_s *cmd, float depthMin, float depthMax);

void FR_CmdSetViewport( struct FrameState_s *cmd, const struct RIViewport_s scissors );
void FR_ConfigurePipelineAttachment( struct pipeline_desc_s *desc, enum RI_Format_e *formats, size_t numAttachment, enum RI_Format_e depthFormat ); 

void FR_CmdDraw( struct FrameState_s *cmd, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance); 
void FR_CmdDrawElements( struct FrameState_s *cmd, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance );

#endif




