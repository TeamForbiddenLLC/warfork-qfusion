/*
Copyright (C) 2002-2007 Victor Luchits

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
#ifndef R_BACKEND_H
#define R_BACKEND_H

#include "r_frame_cmd_buffer.h"

//===================================================================

struct shader_s;
struct mfog_s;
struct superLightStyle_s;
struct portalSurface_s;

// core
void RB_Init( void );
void RB_Shutdown( void );
void RB_SetTime( unsigned int time );
void RB_BeginFrame( void );
void RB_EndFrame( void );
void RB_BeginRegistration( void );
void RB_EndRegistration( void );

void RB_LoadCameraMatrix( const mat4_t m );
void RB_LoadObjectMatrix( const mat4_t m );
void RB_LoadProjectionMatrix( const mat4_t m );

void RB_DepthRange( float depthmin, float depthmax );
void RB_GetDepthRange( float* depthmin, float *depthmax );
void RB_DepthOffset( bool enable );
void RB_ClearDepth( float depth );
void RB_Cull( int cull );
void RB_SetState_2(struct FrameState_s *cmd, int state );
void RB_FlipFrontFace( struct FrameState_s* cmd);
void RB_Scissor( int x, int y, int w, int h );
void RB_GetScissor( int *x, int *y, int *w, int *h );
void RB_SetZClip( float zNear, float zFar );

void RB_BindVBO( int id, int primitive);

void RB_AddDynamicMesh(struct FrameState_s* cmd, const entity_t *entity, const shader_t *shader,
	const struct mfog_s *fog, const struct portalSurface_s *portalSurface, unsigned int shadowBits,
	const struct mesh_s *mesh, int primitive, float x_offset, float y_offset );
void RB_FlushDynamicMeshes(struct FrameState_s* cmd);

void RB_DrawElements(struct FrameState_s *cmd, int firstVert, int numVerts, int firstElem, int numElems, int firstShadowVert, int numShadowVerts, int firstShadowElem, int numShadowElems );
void RB_DrawElementsInstanced( int firstVert, int numVerts, int firstElem, int numElems,
	int firstShadowVert, int numShadowVerts, int firstShadowElem, int numShadowElems,
	int numInstances, instancePoint_t *instances );
void RB_DrawShadedElements_2( struct FrameState_s *cmd,
							  int firstVert,
							  int numVerts,
							  int firstElem,
							  int numElems,
							  int firstShadowVert,
							  int numShadowVerts,
							  int firstShadowElem,
							  int numShadowElems );

// shader
void RB_BindShader(struct FrameState_s* frame,  const entity_t *e, const struct shader_s *shader, const struct mfog_s *fog );
void RB_SetLightstyle( const struct superLightStyle_s *lightStyle );
void RB_SetDlightBits( unsigned int dlightBits );
void RB_SetShadowBits( unsigned int shadowBits );
void RB_SetBonesData( int numBones, dualquat_t *dualQuats, int maxWeights );
void RB_SetPortalSurface( const struct portalSurface_s *portalSurface );
void RB_SetSkyboxShader( const shader_t *shader );
void RB_SetSkyboxSide( int side );
void RB_SetRenderFlags( int flags );
void RB_SetLightParams( float minLight, bool noWorldLight );
void RB_SetShaderStateMask( int ANDmask, int ORmask );
void RB_SetCamera( const vec3_t cameraOrigin, const mat3_t cameraAxis );
bool RB_EnableTriangleOutlines( bool enable );

vattribmask_t RB_GetVertexAttribs( void );


#endif // R_BACKEND_H
