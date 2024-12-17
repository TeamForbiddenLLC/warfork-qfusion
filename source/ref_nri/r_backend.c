/*
Copyright (C) 2002-2011 Victor Luchits

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

#include "r_local.h"
#include "r_backend_local.h"

// Smaller buffer for 2D polygons. Also a workaround for some instances of a hardly explainable bug on Adreno
// that caused dynamic draws to slow everything down in some cases when normals are used with dynamic VBOs.
#define COMPACT_STREAM_VATTRIBS ( VATTRIB_POSITION_BIT | VATTRIB_COLOR0_BIT | VATTRIB_TEXCOORDS_BIT )

rbackend_t rb;


void RB_Init( void )
{
	memset( &rb, 0, sizeof( rb ) );

	rb.mempool = R_AllocPool( NULL, "Rendering Backend" );
	const vattribmask_t vattribs[RB_DYN_STREAM_NUM] = {
		VATTRIBS_MASK & ~VATTRIB_INSTANCES_BITS, // RB_DYN_STREAM_DEFAULT
		COMPACT_STREAM_VATTRIBS // RB_DYN_STREAM_COMPACT
	};
	for(size_t i = 0; i < RB_DYN_STREAM_NUM; i++ ) {
		struct vbo_layout_s layout = R_CreateVBOLayout( vattribs[i], VATTRIB_TEXCOORDS_BIT | VATTRIB_NORMAL_BIT | VATTRIB_SVECTOR_BIT );
		const struct gpu_frame_ele_ring_desc_s  indexElementDesc = {
			.numElements = 1024,
			.elementStride = sizeof(uint16_t),
	  	.usageBits = NriBufferUsageBits_INDEX_BUFFER
		};
		const struct gpu_frame_ele_ring_desc_s vertexElementDesc = {
			.numElements = 1024,
			.elementStride = layout.vertexStride,
			.usageBits = NriBufferUsageBits_VERTEX_BUFFER 
		};
		rb.dynamicVertexAlloc[i].layout = layout;
		initGPUFrameEleAlloc( &rb.dynamicVertexAlloc[i].vertexAlloc, &vertexElementDesc );
		initGPUFrameEleAlloc( &rb.dynamicVertexAlloc[i].indexAlloc, &indexElementDesc);
	}

	// initialize shading
	RB_InitShading();
	
	RP_PrecachePrograms();
}

/*
* RB_Shutdown
*/
void RB_Shutdown( void )
{
	for(size_t i = 0; i < RB_DYN_STREAM_NUM; i++) {
		freeGPUFrameEleAlloc(&rb.dynamicVertexAlloc[i].vertexAlloc);
		freeGPUFrameEleAlloc(&rb.dynamicVertexAlloc[i].indexAlloc);
	}
	RP_StorePrecacheList();
	R_FreePool( &rb.mempool );
}

/*
* RB_BeginRegistration
*/
void RB_BeginRegistration( void )
{
	RB_BindVBO( 0, 0 );
}

/*
* RB_EndRegistration
*/
void RB_EndRegistration( void )
{
	RB_BindVBO( 0, 0 );
}

/*
* RB_SetTime
*/
void RB_SetTime( unsigned int time )
{
	rb.time = time;
	rb.nullEnt.shaderTime = ri.Sys_Milliseconds();
}

/*
* RB_BeginFrame
*/
void RB_BeginFrame( void )
{
	Vector4Set( rb.nullEnt.shaderRGBA, 1, 1, 1, 1 );
	rb.nullEnt.scale = 1;
	VectorClear( rb.nullEnt.origin );
	Matrix3_Identity( rb.nullEnt.axis );

	// start fresh each frame
	RB_SetShaderStateMask( ~0, 0 );
	RB_BindVBO( 0, 0 );
}

void RB_EndFrame( void )
{
}

void RB_FlushTextureCache( void )
{
}

void RB_DepthRange( float depthmin, float depthmax )
{
	clamp( depthmin, 0.0f, 1.0f );
	clamp( depthmax, 0.0f, 1.0f );
	rb.gl.depthmin = depthmin;
	rb.gl.depthmax = depthmax;
	// depthmin == depthmax is a special case when a specific depth value is going to be written
	if( ( depthmin != depthmax ) && !rb.gl.depthoffset )
		depthmin += 4.0f / 65535.0f;
	//qglDepthRange( depthmin, depthmax );
}

/*
* RB_GetDepthRange
*/
void RB_GetDepthRange( float* depthmin, float *depthmax )
{
	*depthmin = rb.gl.depthmin;
	*depthmax = rb.gl.depthmax;
}

/*
* RB_DepthOffset
*/
void RB_DepthOffset( bool enable )
{
	float depthmin = rb.gl.depthmin;
	float depthmax = rb.gl.depthmax;
	rb.gl.depthoffset = enable;
	if( depthmin != depthmax )
	{
		if( !enable )
			depthmin += 4.0f / 65535.0f;
		qglDepthRange( depthmin, depthmax );
	}
}

/*
* RB_ClearDepth
*/
void RB_ClearDepth( float depth )
{
	qglClearDepth( depth );
}

/*
* RB_LoadCameraMatrix
*/
void RB_LoadCameraMatrix( const mat4_t m )
{
	Matrix4_Copy( m, rb.cameraMatrix );
}

/*
* RB_LoadObjectMatrix
*/
void RB_LoadObjectMatrix( const mat4_t m )
{
	Matrix4_Copy( m, rb.objectMatrix );
	Matrix4_MultiplyFast( rb.cameraMatrix, m, rb.modelviewMatrix );
	Matrix4_Multiply( rb.projectionMatrix, rb.modelviewMatrix, rb.modelviewProjectionMatrix );
}

/*
* RB_LoadProjectionMatrix
*/
void RB_LoadProjectionMatrix( const mat4_t m )
{
	Matrix4_Copy( m, rb.projectionMatrix );
	Matrix4_Multiply( m, rb.modelviewMatrix, rb.modelviewProjectionMatrix );
}

// for these we assume will just do this to the first attachment
void RB_SetState_2( struct frame_cmd_buffer_s *cmd, int state )
{
	
	cmd->state.pipelineLayout.blendEnabled = ( state & GLSTATE_BLEND_MASK );
	if( state & GLSTATE_BLEND_MASK ) {
		switch( state & GLSTATE_SRCBLEND_MASK ) {
			case GLSTATE_SRCBLEND_ZERO:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_ZERO;
				break;
			case GLSTATE_SRCBLEND_DST_COLOR:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_DST_COLOR;
				break;
			case GLSTATE_SRCBLEND_ONE_MINUS_DST_COLOR:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_ONE_MINUS_DST_COLOR;
				break;
			case GLSTATE_SRCBLEND_SRC_ALPHA:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_SRC_ALPHA;
				break;
			case GLSTATE_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_ONE_MINUS_SRC_ALPHA;
				break;
			case GLSTATE_SRCBLEND_DST_ALPHA:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_DST_ALPHA;
				break;
			case GLSTATE_SRCBLEND_ONE_MINUS_DST_ALPHA:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_ONE_MINUS_DST_ALPHA;
				break;
			default:
			case GLSTATE_SRCBLEND_ONE:
				cmd->state.pipelineLayout.colorSrcFactor = NriBlendFactor_ONE;
				break;
		}

		switch( state & GLSTATE_DSTBLEND_MASK ) {
			case GLSTATE_DSTBLEND_ONE:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_ONE;
				break;
			case GLSTATE_DSTBLEND_SRC_COLOR:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_SRC_COLOR;
				break;
			case GLSTATE_DSTBLEND_ONE_MINUS_SRC_COLOR:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_ONE_MINUS_SRC_COLOR;
				break;
			case GLSTATE_DSTBLEND_SRC_ALPHA:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_SRC_ALPHA;
				break;
			case GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_ONE_MINUS_SRC_ALPHA;
				break;
			case GLSTATE_DSTBLEND_DST_ALPHA:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_DST_ALPHA;
				break;
			case GLSTATE_DSTBLEND_ONE_MINUS_DST_ALPHA:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_ONE_MINUS_DST_ALPHA;
				break;
			default:
			case GLSTATE_DSTBLEND_ZERO:
				cmd->state.pipelineLayout.colorDstFactor = NriBlendFactor_ZERO;
				break;
		}
	}

	if( state & GLSTATE_NO_COLORWRITE ) {
		cmd->state.pipelineLayout.colorWriteMask = 0;
	} else {
		cmd->state.pipelineLayout.colorWriteMask = NriColorWriteBits_RGB | ( ( state & GLSTATE_ALPHAWRITE ) ? NriColorWriteBits_A : 0 );
	}

	if( state & GLSTATE_NO_DEPTH_TEST ) {
		cmd->state.pipelineLayout.compareFunc = NriCompareFunc_ALWAYS;
	} else if( state & GLSTATE_DEPTHFUNC_EQ ) {
		cmd->state.pipelineLayout.compareFunc = NriCompareFunc_EQUAL;
	} else if( state & GLSTATE_DEPTHFUNC_GT ) {
		cmd->state.pipelineLayout.compareFunc = NriCompareFunc_GREATER;
	} else {
		cmd->state.pipelineLayout.compareFunc = NriCompareFunc_LESS_EQUAL;
	}

	cmd->state.pipelineLayout.depthWrite = ( state & GLSTATE_DEPTHWRITE );

	rb.gl.depthoffset = ( state & GLSTATE_OFFSET_FILL );
	// if( state & GLSTATE_OFFSET_FILL ) {
	// 	cmd->state.pipelineLayout.depthRangeMin = rb.gl.depthmin;
	// 	cmd->state.pipelineLayout.depthRangeMax = rb.gl.depthmax;
	// } else {
	// 	cmd->state.pipelineLayout.depthRangeMin = rb.gl.depthmin + ( 4.0f / 65535.0f );
	// 	cmd->state.pipelineLayout.depthRangeMax = rb.gl.depthmax;
	// }

//	if( glConfig.stencilBits ) {
//		// TODO: workout stencil test logic
//		//  if( state & GLSTATE_STENCIL_TEST )
//		//  	qglEnable( GL_STENCIL_TEST );
//		//  else
//		//  	qglDisable( GL_STENCIL_TEST );
//	}

	//rb.gl.state = state;
}

void RB_FlipFrontFace( struct frame_cmd_buffer_s *cmd )
{
	if( cmd->state.pipelineLayout.cullMode == NriCullMode_FRONT ) {
		cmd->state.pipelineLayout.cullMode = NriCullMode_FRONT;
	} else {
		cmd->state.pipelineLayout.cullMode = NriCullMode_BACK;
	}
}

/*
* RB_Scissor
*/
void RB_Scissor( int x, int y, int w, int h )
{

	rb.gl.scissor[0] = x;
	rb.gl.scissor[1] = y;
	rb.gl.scissor[2] = w;
	rb.gl.scissor[3] = h;
}

/*
* RB_GetScissor
*/
void RB_GetScissor( int *x, int *y, int *w, int *h )
{
	if( x ) {
		*x = rb.gl.scissor[0];
	}
	if( y ) {
		*y = rb.gl.scissor[1];
	}
	if( w ) {
		*w = rb.gl.scissor[2];
	}
	if( h ) {
		*h = rb.gl.scissor[3];
	}
}

/*
* RB_BoundFrameBufferObject
*/
int RB_BoundFrameBufferObject( void )
{
	return 0; //RFB_BoundObject();
}

/*
* RB_BindVBO
*/
void RB_BindVBO( int id, int primitive )
{
	mesh_vbo_t *vbo;

	rb.primitive = primitive;

	assert(id >= 0);
	if( id == 0 ) {
		vbo = NULL;
	} else {
		vbo = R_GetVBOByIndex( id );
	}

	rb.currentVBOId = id;
	rb.currentVBO = vbo;
}

void RB_AddDynamicMesh(struct frame_cmd_buffer_s* cmd, const entity_t *entity, const shader_t *shader,
	const struct mfog_s *fog, const struct portalSurface_s *portalSurface, unsigned int shadowBits,
	const struct mesh_s *mesh, int primitive, float x_offset, float y_offset )
{
  int numVerts = mesh->numVerts, numElems = mesh->numElems;
  bool trifan = false;
  const int scissor[4] = {
  	cmd->state.scissors[0].x,
  	cmd->state.scissors[0].y,
  	cmd->state.scissors[0].width,
  	cmd->state.scissors[0].height
  };
  rbDynamicDraw_t *prev = NULL, *draw;
  bool merge = false;
  vattribmask_t vattribs;

  // can't (and shouldn't because that would break batching) merge strip draw calls
  // (consider simply disabling merge later in this case if models with tristrips are added in the future, but that's slow)
  assert( ( primitive == GL_TRIANGLES ) || ( primitive == GL_LINES ) );

  if( !numElems ) {
  	numElems = ( max( numVerts, 2 ) - 2 ) * 3;
  	trifan = true;
  }
  if( !numVerts || !numElems || ( numVerts > MAX_STREAM_VBO_VERTS ) || ( numElems > MAX_STREAM_VBO_ELEMENTS ) ) {
  	return;
  }

  if( rb.numDynamicDraws ) {
  	prev = &rb.dynamicDraws[rb.numDynamicDraws - 1];
  }
  enum dynamic_stream_e streamId = RB_DYN_STREAM_NUM;
  if( prev ) {
  	int prevRenderFX = 0, renderFX = 0;
  	if( prev->entity ) {
  		prevRenderFX = prev->entity->renderfx;
  	}
  	if( entity ) {
  		renderFX = entity->renderfx;
  	}
  	if( ( ( shader->flags & SHADER_ENTITY_MERGABLE ) || ( prev->entity == entity ) ) && ( prevRenderFX == renderFX ) &&
  		( prev->shader == shader ) && ( prev->fog == fog ) && ( prev->portalSurface == portalSurface ) &&
  		( ( prev->shadowBits && shadowBits ) || ( !prev->shadowBits && !shadowBits ) ) ) {
  		// don't rebind the shader to get the VBO in this case
  		streamId = prev->dynamicStreamIdx;
  		if( ( prev->shadowBits == shadowBits ) && ( prev->primitive == primitive ) &&
  			( prev->offset[0] == x_offset ) && ( prev->offset[1] == y_offset ) &&
  			!memcmp( prev->scissor, scissor, sizeof( scissor ) ) ) {
  			merge = true;
  		}
  	}
  }

  if( streamId == RB_DYN_STREAM_NUM ) {
	  RB_BindShader( cmd, entity, shader, fog );
	  vattribs = rb.currentVAttribs;
	  streamId = ( ( vattribs & ~COMPACT_STREAM_VATTRIBS ) ? RB_DYN_STREAM_DEFAULT : RB_DYN_STREAM_COMPACT );
  } else {
	  vattribs = prev->vattribs;
  }
  assert(streamId != MAX_DYNAMIC_DRAWS);
	struct gpu_frame_ele_req_s vboReq = {};
	struct gpu_frame_ele_req_s eleReq = {};
	GPUFrameEleAlloc(cmd, &rb.dynamicVertexAlloc[streamId].vertexAlloc, numVerts, &vboReq);
	GPUFrameEleAlloc(cmd, &rb.dynamicVertexAlloc[streamId].indexAlloc, numElems, &eleReq);

	if( !merge && ( rb.numDynamicDraws + 1 ) > MAX_DYNAMIC_DRAWS) {
		// wrap if overflows
		RB_FlushDynamicMeshes( cmd );
		merge = false;
	}

	uint32_t vertexStartIdx = 0; 
  // we can only merge if the request from the buffer is continuous 
  if( merge && 
  	vboReq.buffer == prev->vertexBuffer && 
		eleReq.buffer == prev->indexBuffer &&
		IsElementBufferContinous( prev->bufferIndexEleOffset, prev->numElems, vboReq.elementOffset ) && 
		IsElementBufferContinous( prev->bufferVertEleOffset, prev->numVerts, eleReq.elementOffset)) {
	  // merge continuous draw calls
	  draw = prev;
	  draw->numVerts += numVerts; // if we can merge we just extend the previous buffer into this one
	  draw->numElems += numElems;
	  vertexStartIdx = vboReq.elementOffset - draw->bufferVertEleOffset;
	  assert(vertexStartIdx < draw->numElems);
  } else {
	  draw = &rb.dynamicDraws[rb.numDynamicDraws++];
	  draw->entity = entity;
	  draw->shader = shader;
	  draw->fog = fog;
	  draw->portalSurface = portalSurface;
	  draw->shadowBits = shadowBits;
	  draw->vattribs = vattribs;
	  draw->dynamicStreamIdx = streamId;
	  draw->primitive = primitive;
	  draw->offset[0] = x_offset;
	  draw->offset[1] = y_offset;
	  memcpy( draw->scissor, scissor, sizeof( scissor ) );
	  draw->vertexBuffer = vboReq.buffer;
	  draw->indexBuffer = eleReq.buffer;
	  draw->bufferIndexEleOffset = eleReq.elementOffset;
	  draw->numVerts = numVerts;
	  draw->bufferVertEleOffset = vboReq.elementOffset;
	  draw->numElems = numElems;
	  draw->layout = &rb.dynamicVertexAlloc[streamId].layout;
	  assert( draw->layout->vertexStride == vboReq.elementStride );
  }

	void *eleMemory = rsh.nri.coreI.MapBuffer( eleReq.buffer, eleReq.elementOffset * eleReq.elementStride, eleReq.numElements * eleReq.elementStride );
	void *vboMemory = rsh.nri.coreI.MapBuffer( vboReq.buffer, vboReq.elementOffset * vboReq.elementStride, vboReq.numElements * vboReq.elementStride );
	R_WriteMeshToVertexBuffer( draw->layout, vattribs, mesh, ( (uint8_t *)vboMemory )  );
	if( trifan ) {
		R_BuildTrifanElements( vertexStartIdx, numElems, (elem_t *)( eleMemory ) );
	} else {
		if( primitive == GL_TRIANGLES ) {
			R_CopyOffsetTriangles( mesh->elems, numElems, vertexStartIdx, (elem_t *)( (uint8_t *)eleMemory ) );
		} else {
			R_CopyOffsetElements( mesh->elems, numElems, vertexStartIdx, (elem_t *)( (uint8_t *)eleMemory ) );
		}
	}
	rsh.nri.coreI.UnmapBuffer( vboReq.buffer );
	rsh.nri.coreI.UnmapBuffer( eleReq.buffer );

}

/*
* RB_FlushDynamicMeshes
*/
void RB_FlushDynamicMeshes(struct frame_cmd_buffer_s* cmd)
{
	float offsetx = 0.0f;
	float offsety = 0.0f;
	float transx;
	float transy;
	mat4_t m;

	if( rb.numDynamicDraws == 0 ) {
		return;
	}
	if(cmd->stackCmdBeingRendered == 0) {
		R_FlushTransitionBarrier(cmd->cmd);
	}

	NriRect prevScissors[MAX_COLOR_ATTACHMENTS];
	size_t numColorAttachments = cmd->state.numColorAttachments;
	memcpy(prevScissors, cmd->state.scissors, sizeof(NriRect) * cmd->state.numColorAttachments); // keep a backup of the scissors

	// begin rendering pass
	Matrix4_Copy( rb.objectMatrix, m );
	transx = m[12];
	transy = m[13];
	FR_CmdBeginRendering( cmd );
	for( size_t i = 0; i < rb.numDynamicDraws; i++ ) {
		rbDynamicDraw_t const *draw = &rb.dynamicDraws[i];
		// rbDynamicStream_t *stream = &rb.dynamicStreams[draw->dynamicStreamIdx];
		//struct dynamic_stream_info_s *info = &dynamicStreamInfo[draw->dynamicStreamIdx];
		cmd->state.numStreams = 1;
		cmd->state.streams[0] = ( NriVertexStreamDesc ){ .stride = draw->layout->vertexStride, .stepRate = 0, .bindingSlot = 0 };
		cmd->state.numAttribs = 0;
		switch( draw->primitive ) {
			case GL_LINES:
				cmd->state.pipelineLayout.topology = NriTopology_LINE_LIST;
				break;
			default:
				cmd->state.pipelineLayout.topology = NriTopology_TRIANGLE_LIST;
				break;
		}
		R_FillNriVertexAttribLayout(draw->layout, cmd->state.attribs, &cmd->state.numAttribs);	
		RB_BindShader( cmd, draw->entity, draw->shader, draw->fog );
		RB_SetPortalSurface( draw->portalSurface );
		RB_SetShadowBits( draw->shadowBits );
		FR_CmdSetScissorAll( cmd, ( NriRect ){ draw->scissor[0], draw->scissor[1], draw->scissor[2], draw->scissor[3] } );
		FR_CmdSetVertexBuffer( cmd, 0, draw->vertexBuffer, draw->bufferVertEleOffset * draw->layout->vertexStride);
		FR_CmdSetIndexBuffer( cmd, draw->indexBuffer, draw->bufferIndexEleOffset * sizeof(uint16_t), NriIndexType_UINT16 );

		// translate the mesh in 2D
		if( ( offsetx != draw->offset[0] ) || ( offsety != draw->offset[1] ) ) {
			offsetx = draw->offset[0];
			offsety = draw->offset[1];
			m[12] = transx + offsetx;
			m[13] = transy + offsety;
			RB_LoadObjectMatrix( m );
		}

		RB_DrawShadedElements_2( cmd, 0, draw->numVerts, 0, draw->numElems, 
													        0, draw->numVerts, 0, draw->numElems );
		FR_CmdResetCommandState( cmd, CMD_RESET_INDEX_BUFFER | CMD_RESET_VERTEX_BUFFER );
	}
	FR_CmdEndRendering( cmd );

	rb.numDynamicDraws = 0;

	FR_CmdSetScissor(cmd, prevScissors, numColorAttachments);

	// restore the original translation in the object matrix if it has been changed
	if( offsetx || offsety ) {
		m[12] = transx;
		m[13] = transy;
		RB_LoadObjectMatrix( m );
	}
}

/*
* RB_GetVertexAttribs
*/
vattribmask_t RB_GetVertexAttribs( void )
{
	return rb.currentVAttribs;
}




void RB_DrawElements( struct frame_cmd_buffer_s *cmd, int firstVert, int numVerts, int firstElem, int numElems, int firstShadowVert, int numShadowVerts, int firstShadowElem, int numShadowElems ) {
	rb.currentVAttribs &= ~VATTRIB_INSTANCES_BITS;

	rb.drawElements.numVerts = numVerts;
	rb.drawElements.numElems = numElems;
	rb.drawElements.firstVert = firstVert;
	rb.drawElements.firstElem = firstElem;
	rb.drawElements.numInstances = 0;

	rb.drawShadowElements.numVerts = numShadowVerts;
	rb.drawShadowElements.numElems = numShadowElems;
	rb.drawShadowElements.firstVert = firstShadowVert;
	rb.drawShadowElements.firstElem = firstShadowElem;
	rb.drawShadowElements.numInstances = 0;
	
	//RB_DrawElements_();
	assert(false);
}

/*
* RB_DrawElementsInstanced
*
* Draws <numInstances> instances of elements
*/
void RB_DrawElementsInstanced( int firstVert, int numVerts, int firstElem, int numElems,
	int firstShadowVert, int numShadowVerts, int firstShadowElem, int numShadowElems,
	int numInstances, instancePoint_t *instances )
{
	if( !numInstances ) {
		return;
	}

	// currently not supporting dynamic instances
	// they will need a separate stream so they can be used with both static and dynamic geometry
	// (dynamic geometry will need changes to rbDynamicDraw_t)
	assert( rb.currentVBOId >= 0);
	if( rb.currentVBOId <= 0) {
		return;
	}

	rb.drawElements.numVerts = numVerts;
	rb.drawElements.numElems = numElems;
	rb.drawElements.firstVert = firstVert;
	rb.drawElements.firstElem = firstElem;
	rb.drawElements.numInstances = 0;

	rb.drawShadowElements.numVerts = numShadowVerts;
	rb.drawShadowElements.numElems = numShadowElems;
	rb.drawShadowElements.firstVert = firstShadowVert;
	rb.drawShadowElements.firstElem = firstShadowElem;
	rb.drawShadowElements.numInstances = 0;

	// check for vertex-attrib-divisor style instancing
	if( glConfig.ext.instanced_arrays ) {
		if( rb.currentVBO->instancesOffset ) {
			// static VBO's must come with their own set of instance data
			rb.currentVAttribs |= VATTRIB_INSTANCES_BITS;
		}
	}

	if( !( rb.currentVAttribs & VATTRIB_INSTANCES_BITS ) ) {
		// can't use instanced arrays so we'll have to manually update
		// the uniform state in between draw calls
		if( rb.maxDrawInstances < numInstances ) {
			if( rb.drawInstances ) {
				RB_Free( rb.drawInstances );
			}
			rb.drawInstances = RB_Alloc( numInstances * sizeof( *rb.drawInstances ) );
			rb.maxDrawInstances = numInstances;
		}
		memcpy( rb.drawInstances, instances, numInstances * sizeof( *instances ) );
	}

	rb.drawElements.numInstances = numInstances;
	rb.drawShadowElements.numInstances = numInstances;
	//RB_DrawElements_();
	assert(false);
}

/*
* RB_SetCamera
*/
void RB_SetCamera( const vec3_t cameraOrigin, const mat3_t cameraAxis )
{
	VectorCopy( cameraOrigin, rb.cameraOrigin );
	Matrix3_Copy( cameraAxis, rb.cameraAxis );
}

/*
* RB_SetRenderFlags
*/
void RB_SetRenderFlags( int flags )
{
	rb.renderFlags = flags;
}

/*
* RB_EnableTriangleOutlines
*
* Returns triangle outlines state before the call
*/
bool RB_EnableTriangleOutlines( bool enable )
{
	bool oldVal = rb.triangleOutlines;

	if( rb.triangleOutlines != enable ) {
		rb.triangleOutlines = enable;

		// OpenGL ES systems don't support glPolygonMode
#ifndef GL_ES_VERSION_2_0
		if( enable ) {
			RB_SetShaderStateMask( 0, GLSTATE_NO_DEPTH_TEST );
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else {
			RB_SetShaderStateMask( ~0, 0 );
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
#endif
	}

	return oldVal;
}

/*
* RB_ScissorForBounds
*/
bool RB_ScissorForBounds( vec3_t bbox[8], const struct QRectf32_s viewport, int *x, int *y, int *w, int *h )
{
	int i;
	int ix1, iy1, ix2, iy2;
	float x1, y1, x2, y2;
	vec4_t corner = { 0, 0, 0, 1 }, proj = { 0, 0, 0, 1 }, v = { 0, 0, 0, 1 };
	mat4_t cameraProjectionMatrix;

	Matrix4_Multiply( rb.projectionMatrix, rb.cameraMatrix, cameraProjectionMatrix );

	x1 = y1 = 999999;
	x2 = y2 = -999999;
	for( i = 0; i < 8; i++ )
	{
		// compute and rotate the full bounding box
		VectorCopy( bbox[i], corner );

		Matrix4_Multiply_Vector( cameraProjectionMatrix, corner, proj );

		if( proj[3] ) {
			v[0] = ( proj[0] / proj[3] + 1.0f ) * 0.5f * viewport.w;
			v[1] = ( proj[1] / proj[3] + 1.0f ) * 0.5f * viewport.h;
			v[2] = ( proj[2] / proj[3] + 1.0f ) * 0.5f; // [-1..1] -> [0..1]
		} else {
			v[0] = 999999.0f;
			v[1] = 999999.0f;
			v[2] = 999999.0f;
		}

		x1 = min( x1, v[0] ); y1 = min( y1, v[1] );
		x2 = max( x2, v[0] ); y2 = max( y2, v[1] );
	}

	ix1 = max( x1 - 1.0f, 0 ); ix2 = min( x2 + 1.0f, viewport.w );
	if( ix1 >= ix2 )
		return false; // FIXME

	iy1 = max( y1 - 1.0f, 0 ); iy2 = min( y2 + 1.0f, viewport.h );
	if( iy1 >= iy2 )
		return false; // FIXME

	*x = ix1;
	*y = viewport.h - iy2;
	*w = ix2 - ix1;
	*h = iy2 - iy1;

	return true;
}
