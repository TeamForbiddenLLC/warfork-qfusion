/*
Copyright (C) 2011 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef R_BACKEND_LOCAL_H
#define R_BACKEND_LOCAL_H

#include "r_frame_cmd_buffer.h"
#include "r_nri.h"
#include "math/qmath.h"


#include "r_local.h"

#define MAX_STREAM_VBO_VERTS	    1000000	
#define MAX_STREAM_VBO_ELEMENTS		MAX_STREAM_VBO_VERTS*6
#define MAX_STREAM_VBO_TRIANGLES	MAX_STREAM_VBO_ELEMENTS/3

#define MAX_DYNAMIC_DRAWS			2048

enum dynamic_stream_e {
	RB_DYN_STREAM_DEFAULT,
	RB_DYN_STREAM_COMPACT, // bind RB_VBO_STREAM instead
	RB_DYN_STREAM_NUM, // bind RB_VBO_STREAM instead
};

typedef struct r_backend_stats_s
{
	unsigned int numVerts, numElems;
	unsigned int c_totalVerts, c_totalTris, c_totalStaticVerts, c_totalStaticTris, c_totalDraws, c_totalBinds;
	unsigned int c_totalPrograms;
} rbStats_t;

typedef struct
{
	unsigned int numBones;
	dualquat_t dualQuats[128];
	unsigned int maxWeights;
} rbBonesData_t;

typedef struct
{
	unsigned int firstVert;
	unsigned int numVerts;
	unsigned int firstElem;
	unsigned int numElems;
	unsigned int numInstances;
} rbDrawElements_t;

typedef struct
{
	mesh_vbo_t *vbo;
	uint8_t *vertexData;
	uint16_t *elementData;
	rbDrawElements_t drawElements;
} rbDynamicStream_t;

typedef struct
{
	const entity_t *entity;
	const shader_t *shader;
	const mfog_t *fog;
	const portalSurface_t *portalSurface;
	unsigned int shadowBits;
	vattribmask_t vattribs; // based on the fields above - cached to avoid rebinding
	enum dynamic_stream_e dynamicStreamIdx;
	int primitive;
	vec2_t offset;
	int scissor[4];

	const struct vbo_layout_s* layout;
	NriBuffer* vertexBuffer;
	NriBuffer* indexBuffer;
	uint32_t bufferVertEleOffset;
	uint32_t bufferIndexEleOffset;
	unsigned int numVerts;
	unsigned int numElems;
} rbDynamicDraw_t;

typedef struct r_backend_s
{
	mempool_t			*mempool;

	struct
	{
		int				scissor[4];

		float			depthmin, depthmax;

		bool			depthoffset;
	} gl;

	unsigned int time;

	mat4_t cameraMatrix;
	mat4_t objectMatrix;
	mat4_t modelviewMatrix;
	mat4_t projectionMatrix;
	mat4_t modelviewProjectionMatrix;
	float zNear, zFar;

	int renderFlags;

	vec3_t cameraOrigin;
	mat3_t cameraAxis;

	const entity_t *currentEntity;
	modtype_t currentModelType;
	const mesh_vbo_t *currentMeshVBO;
	rbBonesData_t bonesData;
	const portalSurface_t *currentPortalSurface;
	
	struct {
		struct vbo_layout_s layout;
		struct gpu_frame_ele_allocator_s vertexAlloc;
		struct gpu_frame_ele_allocator_s indexAlloc;
	} dynamicVertexAlloc[RB_DYN_STREAM_NUM];
	rbDynamicDraw_t dynamicDraws[MAX_DYNAMIC_DRAWS];
	unsigned int numDynamicDraws;

	instancePoint_t *drawInstances;
	int maxDrawInstances;

	rbDrawElements_t drawElements;
	rbDrawElements_t drawShadowElements;

	vattribmask_t currentVAttribs;

	int primitive;
	int currentVBOId;
	mesh_vbo_t *currentVBO;

	unsigned int currentDlightBits;
	unsigned int currentShadowBits;

	const shader_t *skyboxShader;
	int skyboxSide;

	// shader state
	const shader_t *currentShader;
	double currentShaderTime;
	int currentShaderState;
	int shaderStateORmask, shaderStateANDmask;
	bool doneDepthPass;

	bool triangleOutlines;

	const superLightStyle_t *superLightStyle;

	uint8_t entityColor[4];
	uint8_t entityOutlineColor[4];
	entity_t nullEnt;

	const mfog_t *fog, *texFog, *colorFog;

	bool greyscale;
	bool alphaHack;
	bool noDepthTest;
	bool noColorWrite;
	bool depthEqual;
	float hackedAlpha;

	float minLight;
	bool noWorldLight;
} rbackend_t;

extern rbackend_t rb;

// r_backend.c
#define RB_Alloc(size) R_MallocExt( rb.mempool, size, 16, 1 )
#define RB_Free(data) R_Free(data)

//void RB_DrawElementsReal( rbDrawElements_t *de );
#define RB_IsAlphaBlending(blendsrc,blenddst) \
	( (blendsrc) == GLSTATE_SRCBLEND_SRC_ALPHA || (blenddst) == GLSTATE_DSTBLEND_SRC_ALPHA ) || \
	( (blendsrc) == GLSTATE_SRCBLEND_ONE_MINUS_SRC_ALPHA || (blenddst) == GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA )


void RB_InitShading( void );
void RB_DrawShadedElements( void );
void RB_BindArrayBuffer( int buffer );

bool RB_ScissorForBounds(vec3_t bbox[8], const struct QRectf32_s viewport, int *x, int *y, int *w, int *h );

#endif // R_BACKEND_LOCAL_H
