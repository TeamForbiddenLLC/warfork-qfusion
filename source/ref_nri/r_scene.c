/*
Copyright (C) 2013 Victor Luchits

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
#include "r_program.h"
#include "r_frame_cmd_buffer.h"

#include "ri_types.h"
#include "stb_ds.h"
#include "ri_conversion.h"
static void R_RenderDebugBounds(  struct frame_cmd_buffer_s* frame);

/*
* R_ClearScene
*/
void R_ClearScene( void )
{
	rsc.numLocalEntities = 0;
	rsc.numDlights = 0;
	rsc.numPolys = 0;

	rsc.worldent = R_NUM2ENT( rsc.numLocalEntities );
	rsc.worldent->scale = 1.0f;
	rsc.worldent->model = rsh.worldModel;
	rsc.worldent->rtype = RT_MODEL;
	Matrix3_Identity( rsc.worldent->axis );
	rsc.numLocalEntities++;

	rsc.polyent = R_NUM2ENT( rsc.numLocalEntities );
	rsc.polyent->scale = 1.0f;
	rsc.polyent->model = NULL;
	rsc.polyent->rtype = RT_MODEL;
	Matrix3_Identity( rsc.polyent->axis );
	rsc.numLocalEntities++;

	rsc.skyent = R_NUM2ENT( rsc.numLocalEntities );
	*rsc.skyent = *rsc.worldent;
	rsc.numLocalEntities++;

	rsc.numEntities = rsc.numLocalEntities;

	rsc.numBmodelEntities = 0;

	rsc.renderedShadowBits = 0;
	rsc.frameCount++;
	
	arrsetlen(rsc.debugBounds, 0);

	R_ClearShadowGroups();

	R_ClearSkeletalCache();
}

void R_DisposeScene( r_scene_t *scene )
{
	arrfree( scene->debugBounds );
}

/*
* R_AddEntityToScene
*/
void R_AddEntityToScene( const entity_t *ent )
{
	if( !r_drawentities->integer )
		return;

	if( ( ( rsc.numEntities - rsc.numLocalEntities ) < MAX_ENTITIES ) && ent )
	{
		int eNum = rsc.numEntities;
		entity_t *de = R_NUM2ENT(eNum);

		*de = *ent;
		if( r_outlines_scale->value <= 0 )
			de->outlineHeight = 0;
		rsc.entShadowBits[eNum] = 0;
		rsc.entShadowGroups[eNum] = 0;

		if( de->rtype == RT_MODEL ) {
			if( de->model && de->model->type == mod_brush ) {
				rsc.bmodelEntities[rsc.numBmodelEntities++] = de;
			}
			if( !(de->renderfx & RF_NOSHADOW) ) {
				R_AddLightOccluder( de ); // build groups and mark shadow casters
			}
		}
		else if( de->rtype == RT_SPRITE ) {
			// simplifies further checks
			de->model = NULL;
		}

		if( de->renderfx & RF_ALPHAHACK ) {
			if( de->shaderRGBA[3] == 255 ) {
				de->renderfx &= ~RF_ALPHAHACK;
			}
		}

		rsc.numEntities++;

		// add invisible fake entity for depth write
		if( (de->renderfx & (RF_WEAPONMODEL|RF_ALPHAHACK)) == (RF_WEAPONMODEL|RF_ALPHAHACK) ) {
			entity_t tent = *ent;
			tent.renderfx &= ~RF_ALPHAHACK;
			tent.renderfx |= RF_NOCOLORWRITE|RF_NOSHADOW;
			R_AddEntityToScene( &tent );
		}
	}
}

/*
* R_AddLightToScene
*/
void R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b )
{
	if( ( rsc.numDlights < MAX_DLIGHTS ) && intensity && ( r != 0 || g != 0 || b != 0 ) )
	{
		dlight_t *dl = &rsc.dlights[rsc.numDlights];

		VectorCopy( org, dl->origin );
		dl->intensity = intensity * DLIGHT_SCALE;
		dl->color[0] = r;
		dl->color[1] = g;
		dl->color[2] = b;

		if( r_lighting_grayscale->integer ) {
			vec_t grey = ColorGrayscale( dl->color );
			dl->color[0] = dl->color[1] = dl->color[2] = bound( 0, grey, 1 );
		}

		rsc.numDlights++;
	}
}

/*
* R_AddPolyToScene
*/
void R_AddPolyToScene( const poly_t *poly )
{
	assert( sizeof( *poly->elems ) == sizeof( elem_t ) );

	if( ( rsc.numPolys < MAX_POLYS ) && poly && poly->numverts )
	{
		drawSurfacePoly_t *dp = &rsc.polys[rsc.numPolys];

		assert( poly->shader != NULL );
		if( !poly->shader ) {
			return;
		}

		dp->type = ST_POLY;
		dp->shader = poly->shader;
		dp->numVerts = min( poly->numverts, MAX_POLY_VERTS );
		dp->xyzArray = poly->verts;
		dp->normalsArray = poly->normals;
		dp->stArray = poly->stcoords;
		dp->colorsArray = poly->colors;
		dp->numElems = poly->numelems;
		dp->elems = ( elem_t * )poly->elems;
		dp->fogNum = poly->fognum;

		// if fogNum is unset, we need to find the volume for polygon bounds
		if( !dp->fogNum ) {
			int i;
			mfog_t *fog;
			vec3_t dpmins, dpmaxs;

			ClearBounds( dpmins, dpmaxs );

			for( i = 0; i < dp->numVerts; i++ ) {
				AddPointToBounds( dp->xyzArray[i], dpmins, dpmaxs );
			}

			fog = R_FogForBounds( dpmins, dpmaxs );
			dp->fogNum = (fog ? fog - rsh.worldBrushModel->fogs + 1 : -1);
		}

		rsc.numPolys++;
	}
}

/*
* R_AddLightStyleToScene
*/
void R_AddLightStyleToScene( int style, float r, float g, float b )
{
	lightstyle_t *ls;

	if( style < 0 || style >= MAX_LIGHTSTYLES )
		ri.Com_Error( ERR_DROP, "R_AddLightStyleToScene: bad light style %i", style );

	ls = &rsc.lightStyles[style];
	ls->rgb[0] = max( 0, r );
	ls->rgb[1] = max( 0, g );
	ls->rgb[2] = max( 0, b );
}

typedef void (*post_processing_t)(const refdef_t* ref, struct frame_cmd_buffer_s* cmd, struct RIDescriptor_s* src); 

void __FXAA_PostProcessing(const refdef_t* ref,struct frame_cmd_buffer_s* cmd, struct RIDescriptor_s* src) {
	r_glslfeat_t programFeatures = 0;
	size_t descriptorIndex = 0;
	struct glsl_descriptor_binding_s descriptors[8] = { 0 };
	
	descriptors[descriptorIndex++] = ( struct glsl_descriptor_binding_s ){
		.descriptor = *src, 
		.handle = Create_DescriptorHandle( "u_BaseTexture" ) 
	};
	descriptors[descriptorIndex++] = ( struct glsl_descriptor_binding_s ){
		.descriptor = *rsh.postProcessingSampler, 
		.handle = Create_DescriptorHandle( "u_BaseSampler" ) 
	};
	struct FxaaPushConstant {
		struct vec2 screenSize;
	} push;
	push.screenSize.x =  cmd->viewport.width;
	push.screenSize.y =  cmd->viewport.height;

	struct glsl_program_s *program = RP_ResolveProgram( GLSL_PROGRAM_TYPE_FXAA, NULL, "", NULL, 0, programFeatures );
	struct pipeline_hash_s *pipeline = RP_ResolvePipeline( program, &cmd->pipeline);


	//rsh.nri.coreI.CmdSetPipeline( cmd->cmd, pipeline->pipeline );
	//rsh.nri.coreI.CmdSetPipelineLayout( cmd->cmd, program->layout );
	//rsh.nri.coreI.CmdSetRootConstants( cmd->cmd, 0, &push, sizeof( struct FxaaPushConstant ) );
	RP_BindPipeline( cmd, pipeline );
	RP_BindPushConstant( &rsh.device, cmd, program, &push, sizeof( struct FxaaPushConstant ) );
	RP_BindDescriptorSets( &rsh.device, cmd, program, descriptors, descriptorIndex );
	FR_CmdDraw(cmd, 3, 0, 0,0);

}

void __ColorCorrection_PostProcessing(const refdef_t* ref,struct frame_cmd_buffer_s* cmd, struct RIDescriptor_s* src) {
	r_glslfeat_t programFeatures = 0;
	size_t descriptorIndex = 0;
	struct glsl_descriptor_binding_s descriptors[8] = { 0 };
	
	descriptors[descriptorIndex++] = ( struct glsl_descriptor_binding_s ){
		.descriptor = *src, 
		.handle = Create_DescriptorHandle( "u_BaseTexture" ) 
	};
	descriptors[descriptorIndex++] = ( struct glsl_descriptor_binding_s ){
		.descriptor = *rsh.postProcessingSampler,//R_CreateDescriptorWrapper(&rsh.nri, cmd->textureBuffers.postProcessingSampler), 
		.handle = Create_DescriptorHandle( "u_BaseSampler" ) 
	};
	
	descriptors[descriptorIndex++] = ( struct glsl_descriptor_binding_s ){
		.descriptor = ref->colorCorrection->passes[0].images[0]->binding, 
		.handle = Create_DescriptorHandle( "u_ColorLUT" ) 
	};
	
	struct glsl_program_s *program = RP_ResolveProgram( GLSL_PROGRAM_TYPE_COLORCORRECTION, NULL, "", NULL, 0, programFeatures );
	struct pipeline_hash_s *pipeline = RP_ResolvePipeline( program, &cmd->pipeline );

	RP_BindPipeline( cmd, pipeline );
	//RP_BindPushConstant( &rsh.device, cmd, program, &push, sizeof( struct FxaaPushConstant ) );
	RP_BindDescriptorSets( &rsh.device, cmd, program, descriptors, descriptorIndex );
	FR_CmdDraw(cmd, 3, 0, 0,0);

}


//TODO: remove frame only bound to primary backbuffer
void R_RenderScene(const refdef_t *fd )
{
	//R_FlushTransitionBarrier(rsh.primaryCmd.cmd);

	uint8_t pogoIndex = 0;
	size_t numPostProcessing = 0;
	post_processing_t postProcessingHandlers[16];

	if( r_norefresh->integer )
		return;

	R_Set2DMode(&rsh.frame,  false );

	RB_SetTime( fd->time );

	if( !( fd->rdflags & RDF_NOWORLDMODEL ) )
		rsc.refdef = *fd;

	rn.refdef = *fd;
	if( !rn.refdef.minLight ) {
		rn.refdef.minLight = 0.1f;
	}

	fd = &rn.refdef;

	rn.renderFlags = RF_NONE;

	rn.farClip = R_DefaultFarClip();
	rn.clipFlags = 15;
	if( rsh.worldModel && !( fd->rdflags & RDF_NOWORLDMODEL ) && rsh.worldBrushModel->globalfog )
		rn.clipFlags |= 16;
	rn.meshlist = &r_worldlist;
	rn.portalmasklist = &r_portalmasklist;
	rn.shadowBits = 0;
	rn.dlightBits = 0;
	rn.shadowGroup = NULL;

	rn.fbColorAttachment = rn.fbDepthAttachment = NULL;
	//rn.colorAttachment = rn.depthAttachment = NULL;

	if( !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		if( r_soft_particles->integer && ( rsh.screenTexture != NULL ) ) {
			//rn.colorAttachment = rsh.primaryCmd.textureBuffers.colorAttachment;

			rn.fbColorAttachment = rsh.screenTexture;
			rn.fbDepthAttachment = rsh.screenDepthTexture;
			rn.renderFlags |= RF_SOFT_PARTICLES;
		}
		shader_t *cc = rn.refdef.colorCorrection;

		if( r_fxaa->integer ) {
			postProcessingHandlers[numPostProcessing++] = __FXAA_PostProcessing;
		}

		if( cc && cc->numpasses > 0 && cc->passes[0].images[0] && cc->passes[0].images[0] != rsh.noTexture ) {
			postProcessingHandlers[numPostProcessing++] = __ColorCorrection_PostProcessing;
		}
	}

	// clip new scissor region to the one currently set
	Vector4Set( rn.scissor, fd->scissor_x, fd->scissor_y, fd->scissor_width, fd->scissor_height );
	Vector4Set( rn.viewport, fd->x, fd->y, fd->width, fd->height );

	VectorCopy( fd->vieworg, rn.pvsOrigin );
	VectorCopy( fd->vieworg, rn.lodOrigin );

	GPU_VULKAN_BLOCK( ( &rsh.renderer ), ( {
						  if( numPostProcessing > 0 ) {
							  vkCmdEndRendering( rsh.frame.handle.vk.cmd ); // end back buffer and swap to pogo attachment
							  VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
							  RI_VK_FillColorAttachment( &colorAttachment, RI_PogoBufferAttachment( rsh.pogoBuffer + rsh.vk.swapchainIndex ), true );

							  VkRenderingAttachmentInfo depthStencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
							  RI_VK_FillDepthAttachment( &depthStencil, &rsh.depthAttachment[rsh.vk.swapchainIndex], true );

							  VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
							  renderingInfo.flags = 0;
							  renderingInfo.renderArea = RIViewportToRect2D( &rsh.frame.viewport );
							  renderingInfo.layerCount = 1;
							  renderingInfo.viewMask = 0;
							  renderingInfo.colorAttachmentCount = 1;
							  renderingInfo.pColorAttachments = &colorAttachment;
							  renderingInfo.pDepthAttachment = &depthStencil;
							  renderingInfo.pStencilAttachment = NULL;
							  vkCmdBeginRendering( rsh.frame.handle.vk.cmd, &renderingInfo );
						  }
					  } ) );

	FR_CmdSetViewport(&rsh.frame, (struct RIViewport_s) {
		.x = fd->x,
		.y = fd->y,
		.width = fd->width,
		.height = fd->height,
		.depthMin = 0.0f,
		.depthMax = 1.0f
	} );
	FR_CmdSetScissor(&rsh.frame, (struct RIRect_s) {
		.x = fd->scissor_x,
		.y =  fd->scissor_y,
		.width = fd->scissor_width,
		.height = fd->scissor_height
	} );

	R_BuildShadowGroups();

	R_RenderView(&rsh.frame, fd );

	R_RenderDebugSurface( fd );

	R_RenderDebugBounds(&rsh.frame);

	if( !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		ri.Mutex_Lock( rf.speedsMsgLock );
		R_WriteSpeedsMessage( rf.speedsMsg, sizeof( rf.speedsMsg ) );
		ri.Mutex_Unlock( rf.speedsMsgLock );
	}

	if( numPostProcessing > 0 ) {
		GPU_VULKAN_BLOCK( ( &rsh.renderer ), ( {
							  vkCmdEndRendering( rsh.frame.handle.vk.cmd ); // end back buffer and swap to pogo attachment
							
								for( size_t i = 0; i < numPostProcessing - 1; i++ ) {
								  RI_PogoBufferToggle( &rsh.device, rsh.pogoBuffer + rsh.vk.swapchainIndex, &rsh.frame.handle);
								  {
									  VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
									  RI_VK_FillColorAttachment( &colorAttachment, RI_PogoBufferAttachment( rsh.pogoBuffer + rsh.vk.swapchainIndex ), true );

									  VkRenderingAttachmentInfo depthStencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
									  RI_VK_FillDepthAttachment( &depthStencil, &rsh.depthAttachment[rsh.vk.swapchainIndex], true );

									  VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
									  renderingInfo.flags = 0;
									  renderingInfo.renderArea = (VkRect2D){ { 0, 0 }, { rsh.riSwapchain.width, rsh.riSwapchain.height } };
									  renderingInfo.layerCount = 1;
									  renderingInfo.viewMask = 0;
									  renderingInfo.colorAttachmentCount = 1;
									  renderingInfo.pColorAttachments = &colorAttachment;
									  renderingInfo.pDepthAttachment = &depthStencil;
									  renderingInfo.pStencilAttachment = NULL;
									  vkCmdBeginRendering( rsh.frame.handle.vk.cmd, &renderingInfo );
								  }
								  FR_CmdResetCommandState( &rsh.frame, CMD_RESET_DEFAULT_PIPELINE_LAYOUT );
								  postProcessingHandlers[i]( fd, &rsh.frame, RI_PogoBufferShaderResource(rsh.pogoBuffer + rsh.vk.swapchainIndex)  );
								  vkCmdEndRendering( rsh.frame.handle.vk.cmd ); // end back buffer and swap to pogo attachment
							  }

							  RI_PogoBufferToggle( &rsh.device, rsh.pogoBuffer + rsh.vk.swapchainIndex, &rsh.frame.handle );
							  FR_CmdResetCommandState( &rsh.frame, CMD_RESET_DEFAULT_PIPELINE_LAYOUT | CMD_RESET_VERTEX_BUFFER );
								// reset back to back buffer
							  {
								  VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
								  RI_VK_FillColorAttachment( &colorAttachment, &rsh.colorAttachment[rsh.vk.swapchainIndex], true );

								  VkRenderingAttachmentInfo depthStencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
								  RI_VK_FillDepthAttachment( &depthStencil, &rsh.depthAttachment[rsh.vk.swapchainIndex], true );

								  VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
								  renderingInfo.flags = 0;
								  renderingInfo.renderArea = RIViewportToRect2D( &rsh.frame.viewport );
								  renderingInfo.layerCount = 1;
								  renderingInfo.viewMask = 0;
								  renderingInfo.colorAttachmentCount = 1;
								  renderingInfo.pColorAttachments = &colorAttachment;
								  renderingInfo.pDepthAttachment = &depthStencil;
								  renderingInfo.pStencilAttachment = NULL;
								  vkCmdBeginRendering( rsh.frame.handle.vk.cmd, &renderingInfo );
							  }
								postProcessingHandlers[numPostProcessing - 1]( fd, &rsh.frame, RI_PogoBufferShaderResource( rsh.pogoBuffer + rsh.vk.swapchainIndex ) );

						  } ) );
	}
	//FR_CmdResetAttachmentToBackbuffer( frame );

	R_Set2DMode(&rsh.frame, true );
}

void R_AddDebugBounds( const vec3_t mins, const vec3_t maxs, const byte_vec4_t color )
{
	r_debug_bound_t bound;
	VectorCopy( mins, bound.mins );
	VectorCopy( maxs, bound.maxs );
	Vector4Copy( color, bound.color );
	arrput(rsc.debugBounds, bound);
}

static void R_RenderDebugBounds( struct frame_cmd_buffer_s* frame)
{
	if( arrlen( rsc.debugBounds ) == 0 )
		return;

	mesh_t mesh;
	vec4_t verts[8];
	byte_vec4_t colors[8];
	elem_t elems[24] = { 0, 1, 1, 3, 3, 2, 2, 0, 
		                   0, 4, 1, 5, 2, 6, 3, 7, 
		                   4, 5, 5, 7, 7, 6, 6, 4 };

	memset( &mesh, 0, sizeof( mesh ) );
	mesh.numVerts = 8;
	mesh.xyzArray = verts;
	mesh.numElems = 24;
	mesh.elems = elems;
	mesh.colorsArray[0] = colors;

	RB_SetShaderStateMask( ~0, GLSTATE_NO_DEPTH_TEST );

	for( size_t i = 0; i < arrlen( rsc.debugBounds ); i++ ) {
		const vec_t *mins = rsc.debugBounds[i].mins;
		const vec_t *maxs = rsc.debugBounds[i].maxs;
		const uint8_t *color = rsc.debugBounds[i].color;

		for( size_t j = 0; j < 8; j++ ) {
			verts[j][0] = ( ( j & 1 ) ? mins[0] : maxs[0] );
			verts[j][1] = ( ( j & 2 ) ? mins[1] : maxs[1] );
			verts[j][2] = ( ( j & 4 ) ? mins[2] : maxs[2] );
			verts[j][3] = 1.0f;
			Vector4Copy( color, colors[j] );
		}

		RB_AddDynamicMesh( frame, rsc.worldent, rsh.whiteShader, NULL, NULL, 0, &mesh, GL_LINES, 0.0f, 0.0f );
	}

	RB_FlushDynamicMeshes( frame );

	RB_SetShaderStateMask( ~0, 0 );
}

