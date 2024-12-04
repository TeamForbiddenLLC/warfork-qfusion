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

#include "NRIDescs.h"
#include "r_image.h"
#include "r_local.h"
#include "stb_ds.h"

static void R_DrawSkyportal(struct frame_cmd_buffer_s* frame, const entity_t *e, skyportal_t *skyportal );
static const NriFormat PortalTextureFormat = NriFormat_RGBA8_UNORM; 
static const NriFormat PortalTextureDepthFormat = NriFormat_D32_SFLOAT; 
/*
* R_AddPortalSurface
*/
portalSurface_t *R_AddPortalSurface( const entity_t *ent, const mesh_t *mesh, 
	const vec3_t mins, const vec3_t maxs, const shader_t *shader, void *drawSurf )
{
	unsigned int i;
	float dist;
	cplane_t plane, untransformed_plane;
	vec3_t v[3];
	portalSurface_t *portalSurface;
	bool depthPortal = !( shader->flags & (SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2) );

	if( !mesh || !shader ) {
		return NULL;
	}

	if( R_FASTSKY() && depthPortal ) {
		return NULL;
	}

	for( i = 0; i < 3; i++ ) {
		VectorCopy( mesh->xyzArray[mesh->elems[i]], v[i] );
	}

	PlaneFromPoints( v, &untransformed_plane );
	untransformed_plane.dist += DotProduct( ent->origin, untransformed_plane.normal );
	untransformed_plane.dist += 1; // nudge along the normal a bit
	CategorizePlane( &untransformed_plane );

	if( shader->flags & SHADER_AUTOSPRITE )
	{
		vec3_t centre;

		// autosprites are quads, facing the viewer
		if( mesh->numVerts < 4 ) {
			return NULL;
		}

		// compute centre as average of 4 vertices
		VectorCopy( mesh->xyzArray[mesh->elems[3]], centre );
		for( i = 0; i < 3; i++ )
			VectorAdd( centre, v[i], centre );
		VectorMA( ent->origin, 0.25, centre, centre );

		VectorNegate( &rn.viewAxis[AXIS_FORWARD], plane.normal );
		plane.dist = DotProduct( plane.normal, centre );
		CategorizePlane( &plane );
	}
	else
	{
		vec3_t temp;
		mat3_t entity_rotation;

		// regular surfaces
		if( !Matrix3_Compare( ent->axis, axis_identity ) )
		{
			Matrix3_Transpose( ent->axis, entity_rotation );

			for( i = 0; i < 3; i++ ) {
				VectorCopy( v[i], temp );
				Matrix3_TransformVector( entity_rotation, temp, v[i] ); 
				VectorMA( ent->origin, ent->scale, v[i], v[i] );
			}

			PlaneFromPoints( v, &plane );
			CategorizePlane( &plane );
		}
		else
		{
			plane = untransformed_plane;
		}
	}

	if( ( dist = PlaneDiff( rn.viewOrigin, &plane ) ) <= BACKFACE_EPSILON )
	{
		// behind the portal plane
		if( !( shader->flags & SHADER_PORTAL_CAPTURE2 ) ) {
			return NULL;
		}

		// we need to render the backplane view
	}

	// check if portal view is opaque due to alphagen portal
	if( shader->portalDistance && dist > shader->portalDistance ) {
		return NULL;
	}

	// find the matching portal plane
	for( i = 0; i < rn.numPortalSurfaces; i++ ) {
		portalSurface = &rn.portalSurfaces[i];

		if( portalSurface->entity == ent &&
			portalSurface->shader == shader &&
			DotProduct( portalSurface->plane.normal, plane.normal ) > 0.99f &&
			fabs( portalSurface->plane.dist - plane.dist ) < 0.1f ) {
				goto addsurface;
		}
	}

	if( i == MAX_PORTAL_SURFACES ) {
		// not enough space
		return NULL;
	}

	portalSurface = &rn.portalSurfaces[rn.numPortalSurfaces++];
	portalSurface->entity = ent;
	portalSurface->plane = plane;
	portalSurface->shader = shader;
	portalSurface->untransformed_plane = untransformed_plane;
	portalSurface->skyPortal = NULL;
	ClearBounds( portalSurface->mins, portalSurface->maxs );
	memset( portalSurface->portalfbs, 0, sizeof( portalSurface->portalfbs ) );

	if( depthPortal ) {
		rn.numDepthPortalSurfaces++;
	}

addsurface:
	AddPointToBounds( mins, portalSurface->mins, portalSurface->maxs );
	AddPointToBounds( maxs, portalSurface->mins, portalSurface->maxs );
	VectorAdd( portalSurface->mins, portalSurface->maxs, portalSurface->centre );
	VectorScale( portalSurface->centre, 0.5, portalSurface->centre );

	return portalSurface;
}

void R_ShutdownPortals() {
	for(size_t i = 0; i < MAX_PORTAL_TEXTURES; i++ )
	{
		struct portal_fb_s* portalFB = &rsh.portalFBs[i];
		
		if(portalFB->colorTexture) {
			rsh.nri.coreI.DestroyTexture(portalFB->colorTexture);
			rsh.nri.coreI.DestroyDescriptor(portalFB->shaderDescriptor.descriptor);
			rsh.nri.coreI.DestroyDescriptor(portalFB->colorAttachment.descriptor);
		}
		if(portalFB->depthTexture) {
			rsh.nri.coreI.DestroyTexture(portalFB->depthTexture);
			rsh.nri.coreI.DestroyDescriptor(portalFB->depthAttachment.descriptor);
		}
		for( size_t i = 0; i < portalFB->numAllocations; i++ )
			rsh.nri.coreI.FreeMemory(portalFB->memory[i]);
		portalFB->numAllocations = 0;
	}
	memset(rsh.portalFBs, 0, sizeof(rsh.portalFBs));

}

static struct portal_fb_s* __ResolvePortalSurface(struct frame_cmd_buffer_s *cmd, int width, int height, bool filtered) {
	assert(Q_ARRAY_COUNT(rsh.portalFBs) >= MAX_PORTAL_TEXTURES);
	struct portal_fb_s* bestFB = NULL; 
	for(size_t i = 0; i < MAX_PORTAL_TEXTURES; i++ )
	{
		struct portal_fb_s* portalFB = &rsh.portalFBs[i];
		if((portalFB->frameNum + NUMBER_FRAMES_FLIGHT) >= cmd->frameCount) {
			continue;
		}
		if( portalFB->colorTexture ) {
			const NriTextureDesc *textureDesc = rsh.nri.coreI.GetTextureDesc( portalFB->colorTexture );
			if( textureDesc->width == width && textureDesc->height == height ) {
				portalFB->samplerDescriptor = R_CreateDescriptorWrapper( &rsh.nri, R_ResolveSamplerDescriptor( filtered ? 0 : IT_NOFILTERING ) );
				portalFB->frameNum = cmd->frameCount;
				return portalFB;
			}
		}
		bestFB = portalFB;
	}
	if(bestFB) {
		bestFB->samplerDescriptor = R_CreateDescriptorWrapper( &rsh.nri, R_ResolveSamplerDescriptor( filtered ? 0 : IT_NOFILTERING ) );
		bestFB->frameNum = cmd->frameCount;
		if( bestFB->depthTexture )
			arrpush( cmd->freeTextures, bestFB->depthTexture );
		if( bestFB->colorTexture )
			arrpush( cmd->freeTextures, bestFB->colorTexture );
		if(bestFB->colorAttachment.descriptor)
			arrpush( cmd->frameTemporaryDesc, bestFB->colorAttachment.descriptor);
		if(bestFB->shaderDescriptor.descriptor)
			arrpush( cmd->frameTemporaryDesc, bestFB->shaderDescriptor.descriptor);
		if(bestFB->depthAttachment.descriptor)
			arrpush( cmd->frameTemporaryDesc, bestFB->depthAttachment.descriptor);
		for( size_t i = 0; i < bestFB->numAllocations; i++ )
			arrpush( cmd->freeMemory, bestFB->memory[i] );
		bestFB->numAllocations = 0;

		const NriTextureDesc textureDesc = { 
										 .width = width,
									   .height = height,
									   .depth = 1,
									   .usage = NriTextureUsageBits_COLOR_ATTACHMENT | NriTextureUsageBits_SHADER_RESOURCE,
									   .layerNum = 1,
									   .format = PortalTextureFormat,
									   .sampleNum = 1,
									   .type = NriTextureType_TEXTURE_2D,
									   .mipNum = 1 };
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture( rsh.nri.device, &textureDesc, &bestFB->colorTexture) );
		const NriTextureDesc depthTextureDesc = { 
										  .width = width,
											.height = height,
											.depth = 1,
											.usage = NriTextureUsageBits_DEPTH_STENCIL_ATTACHMENT,
											.layerNum = 1,
											.format = PortalTextureDepthFormat ,
											.sampleNum = 1,
											.type = NriTextureType_TEXTURE_2D,
											.mipNum = 1 };
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture( rsh.nri.device, &depthTextureDesc, &bestFB->depthTexture ) );
		NriTexture* textures[] = {bestFB->colorTexture, bestFB->depthTexture};

		const NriResourceGroupDesc resourceGroupDesc = {
			.textureNum = Q_ARRAY_COUNT(textures),
			.textures = textures,
			.memoryLocation = NriMemoryLocation_DEVICE,
		};
		const size_t numAllocations = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
		assert(numAllocations <= Q_ARRAY_COUNT(bestFB->memory));
		bestFB->numAllocations = numAllocations;
		NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, bestFB->memory) )
		NriDescriptor* descriptor = NULL;
		const NriTexture2DViewDesc textureAttachmentViewDesc = {
				.texture = bestFB->colorTexture,
				.viewType = NriTexture2DViewType_COLOR_ATTACHMENT,
				.format = textureDesc.format
		};
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &textureAttachmentViewDesc, &descriptor) );
		bestFB->colorAttachment = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
		const NriTexture2DViewDesc depthAttachmentViewDesc = {
				.texture = bestFB->depthTexture,
				.viewType = NriTexture2DViewType_DEPTH_STENCIL_ATTACHMENT,
				.format = depthTextureDesc.format
		};
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &depthAttachmentViewDesc , &descriptor) );
		bestFB->depthAttachment = R_CreateDescriptorWrapper( &rsh.nri, descriptor );

		const NriTexture2DViewDesc textureShaderViewDesc = {
				.texture = bestFB->colorTexture,
				.viewType = NriTexture2DViewType_SHADER_RESOURCE_2D,
				.format = textureDesc.format
		};
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &textureShaderViewDesc, &descriptor) );
		bestFB->shaderDescriptor = R_CreateDescriptorWrapper( &rsh.nri, descriptor );

	}
	return bestFB;
}


/*
* R_DrawPortalSurface
* 
* Renders the portal view and captures the results from framebuffer if
* we need to do a $portalmap stage. Note that for $portalmaps we must
* use a different viewport.
*/
static void R_DrawPortalSurface( struct frame_cmd_buffer_s *cmd, portalSurface_t *portalSurface )
{

	unsigned int i;
	float dist, d, best_d;
	vec3_t viewerOrigin;
	vec3_t origin;
	mat3_t axis;
	entity_t *ent, *best;
	cplane_t *portal_plane = &portalSurface->plane, *untransformed_plane = &portalSurface->untransformed_plane;
	const shader_t *shader = portalSurface->shader;
	vec_t *portal_centre = portalSurface->centre;
	bool mirror, refraction = false;
	int captureTextureId = -1;
	int prevRenderFlags = 0;
	bool prevFlipped;
	bool doReflection, doRefraction;
	struct portal_fb_s* portalTexures[2] = { NULL, NULL };

	doReflection = doRefraction = true;
	if( shader->flags & SHADER_PORTAL_CAPTURE )
	{
		shaderpass_t *pass;

		captureTextureId = 0;

		for( i = 0, pass = shader->passes; i < shader->numpasses; i++, pass++ )
		{
			if( pass->program_type == GLSL_PROGRAM_TYPE_DISTORTION )
			{
				if( ( pass->alphagen.type == ALPHA_GEN_CONST && pass->alphagen.args[0] == 1 ) )
					doRefraction = false;
				else if( ( pass->alphagen.type == ALPHA_GEN_CONST && pass->alphagen.args[0] == 0 ) )
					doReflection = false;
				break;
			}
		}
	}
	else
	{
		captureTextureId = -1;
	}


	dist = PlaneDiff( rn.viewOrigin, portal_plane );
	if( dist <= BACKFACE_EPSILON || !doReflection )
	{
		if( !( shader->flags & SHADER_PORTAL_CAPTURE2 ) || !doRefraction )
			return;

		// even if we're behind the portal, we still need to capture
		// the second portal image for refraction
		refraction = true;
		captureTextureId = 1;
		if( dist < 0 )
		{
			VectorInverse( portal_plane->normal );
			portal_plane->dist = -portal_plane->dist;
		}
	}

	mirror = true; // default to mirror view
	// it is stupid IMO that mirrors require a RT_PORTALSURFACE entity

	best = NULL;
	best_d = 100000000;
	for( i = rsc.numLocalEntities; i < rsc.numEntities; i++ )
	{
		ent = &rsc.entities[i];
		if( ent->rtype != RT_PORTALSURFACE )
			continue;

		d = PlaneDiff( ent->origin, untransformed_plane );
		if( ( d >= -64 ) && ( d <= 64 ) )
		{
			d = Distance( ent->origin, portal_centre );
			if( d < best_d )
			{
				best = ent;
				best_d = d;
			}
		}
	}

	if( best == NULL )
	{
		if( captureTextureId < 0 ) {
			// still do a push&pop because to ensure the clean state
			if( R_PushRefInst(cmd) ) {
				R_PopRefInst(cmd);
			}
			return;
		}
	}
	else
	{
		if( !VectorCompare( best->origin, best->origin2 ) )	// portal
			mirror = false;
		best->rtype = NUM_RTYPES;
	}

	prevRenderFlags = rn.renderFlags;
	prevFlipped = ( rn.refdef.rdflags & RDF_FLIPPED ) != 0;
	if( !R_PushRefInst(cmd) ) {
		return;
	}

	VectorCopy( rn.viewOrigin, viewerOrigin );
	if( prevFlipped ) {
		VectorInverse( &rn.viewAxis[AXIS_RIGHT] );
	}

setup_and_render:

	if( refraction )
	{
		VectorInverse( portal_plane->normal );
		portal_plane->dist = -portal_plane->dist;
		CategorizePlane( portal_plane );
		VectorCopy( rn.viewOrigin, origin );
		Matrix3_Copy( rn.refdef.viewaxis, axis );
		VectorCopy( viewerOrigin, rn.pvsOrigin );

		rn.renderFlags |= RF_PORTALVIEW;
		if( prevFlipped )
			rn.renderFlags |= RF_FLIPFRONTFACE;
	}
	else if( mirror )
	{
		VectorReflect( rn.viewOrigin, portal_plane->normal, portal_plane->dist, origin );

		VectorReflect( &rn.viewAxis[AXIS_FORWARD], portal_plane->normal, 0, &axis[AXIS_FORWARD] );
		VectorReflect( &rn.viewAxis[AXIS_RIGHT], portal_plane->normal, 0, &axis[AXIS_RIGHT] );
		VectorReflect( &rn.viewAxis[AXIS_UP], portal_plane->normal, 0, &axis[AXIS_UP] );

		Matrix3_Normalize( axis );

		VectorCopy( viewerOrigin, rn.pvsOrigin );

		rn.renderFlags = (prevRenderFlags ^ RF_FLIPFRONTFACE) | RF_MIRRORVIEW;
	}
	else
	{
		vec3_t tvec;
		mat3_t A, B, C, rot;

		// build world-to-portal rotation matrix
		VectorNegate( portal_plane->normal, tvec );
		NormalVectorToAxis( tvec, A );

		// build portal_dest-to-world rotation matrix
		ByteToDir( best->frame, tvec );
		NormalVectorToAxis( tvec, B );
		Matrix3_Transpose( B, C );

		// multiply to get world-to-world rotation matrix
		Matrix3_Multiply( C, A, rot );

		// translate view origin
		VectorSubtract( rn.viewOrigin, best->origin, tvec );
		Matrix3_TransformVector( rot, tvec, origin );
		VectorAdd( origin, best->origin2, origin );

		Matrix3_Transpose( A, B );
		Matrix3_Multiply( rn.viewAxis, B, rot );
		Matrix3_Multiply( best->axis, rot, B );
		Matrix3_Transpose( C, A );
		Matrix3_Multiply( B, A, axis );

		// set up portal_plane
		VectorCopy( &axis[AXIS_FORWARD], portal_plane->normal );
		portal_plane->dist = DotProduct( best->origin2, portal_plane->normal );
		CategorizePlane( portal_plane );

		// for portals, vis data is taken from portal origin, not
		// view origin, because the view point moves around and
		// might fly into (or behind) a wall
		VectorCopy( best->origin2, rn.pvsOrigin );
		VectorCopy( best->origin2, rn.lodOrigin );

		rn.renderFlags |= RF_PORTALVIEW;

		// ignore entities, if asked politely
		if( best->renderfx & RF_NOPORTALENTS )
			rn.renderFlags |= RF_ENVVIEW;
		if( prevFlipped )
			rn.renderFlags |= RF_FLIPFRONTFACE;
	}

	rn.refdef.rdflags &= ~( RDF_UNDERWATER|RDF_CROSSINGWATER|RDF_FLIPPED );

	rn.shadowGroup = NULL;
	rn.meshlist = &r_portallist;
	rn.portalmasklist = NULL;

	rn.renderFlags |= RF_CLIPPLANE;
	rn.renderFlags &= ~RF_SOFT_PARTICLES;
	rn.clipPlane = *portal_plane;

	rn.farClip = R_DefaultFarClip();

	rn.clipFlags |= ( 1<<5 );
	rn.frustum[5] = *portal_plane;
	CategorizePlane( &rn.frustum[5] );

	// if we want to render to a texture, initialize texture
	// but do not try to render to it more than once
	const NriAccessLayoutStage layoutTransition = (NriAccessLayoutStage){	
		.layout = NriLayout_COLOR_ATTACHMENT, 
		.access = NriAccessBits_COLOR_ATTACHMENT, 
		.stages = NriStageBits_COLOR_ATTACHMENT 
	};
	if( captureTextureId >= 0 )
	{
		struct portal_fb_s *const fb = __ResolvePortalSurface( cmd, rsc.refdef.width, rsc.refdef.height, ( shader->flags & SHADER_NO_TEX_FILTERING ) );
		portalTexures[captureTextureId] = fb;
		if(fb == NULL) {
			// couldn't register a slot for this plane
			goto done;
		}
		rn.refdef.x = 0;
		rn.refdef.y = 0;
		rn.refdef.width = rsc.refdef.width;
		rn.refdef.height = rsc.refdef.height;
		
		Vector4Set( rn.viewport, rn.refdef.x, rn.refdef.y, rsc.refdef.width, rsc.refdef.height );
		Vector4Set( rn.scissor, rn.refdef.x, rn.refdef.y, rsc.refdef.width, rsc.refdef.height );
		const NriDescriptor *colorAttachments[] = { fb->colorAttachment.descriptor};
		const struct NriViewport viewports[] = {
			( NriViewport ){ 
				.x = rn.viewport[0], 
				.y = rn.viewport[1], 
				.width = rn.viewport[2], 
				.height = rn.viewport[3], 
				.depthMin = 0.0f, 
				.depthMax = 1.0f,
				.originBottomLeft = true
			} 
		};
		const struct NriRect scissors[] = { (NriRect){
			.x = rn.scissor[0],
			.y = rn.scissor[1],
			.width = rn.scissor[2],
			.height = rn.scissor[3]
		} };
		const NriFormat colorFormats[] = { PortalTextureFormat };
		FR_CmdSetTextureAttachment( cmd, colorFormats, colorAttachments, viewports, scissors, 1, PortalTextureDepthFormat, fb->depthAttachment.descriptor );

		{
			NriTextureBarrierDesc transitionBarriers = { 0 };
			transitionBarriers.texture = fb->colorTexture;
			transitionBarriers.after = layoutTransition; 
	
			NriBarrierGroupDesc barrierGroupDesc = { 0 };
			barrierGroupDesc.textureNum = 1;
			barrierGroupDesc.textures = &transitionBarriers;
			rsh.nri.coreI.CmdBarrier( cmd->cmd, &barrierGroupDesc );
		}

		rn.renderFlags |= RF_PORTAL_CAPTURE;
	} else {
		rn.renderFlags &= ~RF_PORTAL_CAPTURE;
	}

	VectorCopy( origin, rn.refdef.vieworg );
	Matrix3_Copy( axis, rn.refdef.viewaxis );
	assert(cmd->stackCmdBeingRendered == 0);

	cmd->state.pipelineLayout.flippedViewport = true;
	R_RenderView(cmd, &rn.refdef );
	cmd->state.pipelineLayout.flippedViewport = false;

	if(captureTextureId != -1 && portalTexures[captureTextureId]) {
			NriTextureBarrierDesc transitionBarriers = { 0 };
			transitionBarriers.texture = portalTexures[captureTextureId]->colorTexture;
			transitionBarriers.before = layoutTransition; 
			transitionBarriers.after = (NriAccessLayoutStage) {
				.layout = NriLayout_SHADER_RESOURCE,
				.access = NriAccessBits_SHADER_RESOURCE,
				.stages = NriStageBits_FRAGMENT_SHADER 
			};
	
			NriBarrierGroupDesc barrierGroupDesc = { 0 };
			barrierGroupDesc.textureNum = 1;
			barrierGroupDesc.textures = &transitionBarriers;
			rsh.nri.coreI.CmdBarrier( cmd->cmd, &barrierGroupDesc );
	}

	if( doRefraction && !refraction && ( shader->flags & SHADER_PORTAL_CAPTURE2 ) )
	{
		rn.renderFlags = prevRenderFlags;
		refraction = true;
		captureTextureId = 1;
		goto setup_and_render;
	}

done:
	portalSurface->portalfbs[0] = portalTexures[0];
	portalSurface->portalfbs[1] = portalTexures[1];
	R_PopRefInst(cmd);
}

void R_DrawPortals(struct frame_cmd_buffer_s* frame)
{
	unsigned int i;

	if( rf.viewcluster == -1 ) {
		return;
	}
	const struct frame_cmd_save_attachment_s stash = R_CmdState_StashAttachment(frame); 

	if( !( rn.renderFlags & ( RF_MIRRORVIEW|RF_PORTALVIEW|RF_SHADOWMAPVIEW ) ) ) {

		// render skyportal
		if( rn.skyportalSurface ) {
			NriAttachmentsDesc attachmentsDesc = {0};
			attachmentsDesc.depthStencil = frame->state.depthAttachment;
			attachmentsDesc.colorNum = frame->state.numColorAttachments;
			attachmentsDesc.colors = frame->state.colorAttachment;
			rsh.nri.coreI.CmdBeginRendering( frame->cmd, &attachmentsDesc );
			{
				NriClearDesc clearDesc[MAX_COLOR_ATTACHMENTS + 1] = {0};
				size_t numClearDesc = 0;
				clearDesc[numClearDesc].value.depthStencil = ( NriDepthStencil ){ .depth = 1.0f, .stencil = 0.0f };
				clearDesc[numClearDesc].planes = NriPlaneBits_DEPTH;
				numClearDesc++;
				rsh.nri.coreI.CmdClearAttachments( frame->cmd, clearDesc, numClearDesc, NULL, 0 );
			}
			rsh.nri.coreI.CmdEndRendering( frame->cmd );
			
			portalSurface_t *ps = rn.skyportalSurface;
			R_DrawSkyportal( frame, ps->entity, ps->skyPortal );
		}

		// render regular portals
		for( i = 0; i < rn.numPortalSurfaces; i++ ) {
			portalSurface_t ps = rn.portalSurfaces[i];
			if( !ps.skyPortal ) {
				R_DrawPortalSurface( frame, &ps );
				rn.portalSurfaces[i] = ps;
			}
		}
	}
	R_CmdState_RestoreAttachment(frame, &stash);
}

/*
* R_AddSkyportalSurface
*/
portalSurface_t *R_AddSkyportalSurface( const entity_t *ent, const shader_t *shader, void *drawSurf )
{
	portalSurface_t *portalSurface;

	if( rn.skyportalSurface ) {
		portalSurface = rn.skyportalSurface;
	}
	else if( rn.numPortalSurfaces == MAX_PORTAL_SURFACES ) {
		// not enough space
		return NULL;
	}
	else {
		portalSurface = &rn.portalSurfaces[rn.numPortalSurfaces++];
		memset( portalSurface, 0, sizeof( *portalSurface ) );
		rn.skyportalSurface = portalSurface;
		rn.numDepthPortalSurfaces++;
	}

	R_AddSurfToDrawList( rn.portalmasklist, ent, NULL, rsh.skyShader, 0, 0, NULL, drawSurf );
	
	portalSurface->entity = ent;
	portalSurface->shader = shader;
	portalSurface->skyPortal = &rn.refdef.skyportal;
	return rn.skyportalSurface;
}

/*
* R_DrawSkyportal
*/
static void R_DrawSkyportal(struct frame_cmd_buffer_s* frame, const entity_t *e, skyportal_t *skyportal )
{
	if( !R_PushRefInst(frame) ) {
		return;
	}

	rn.renderFlags = ( rn.renderFlags|RF_PORTALVIEW );
	//rn.renderFlags &= ~RF_SOFT_PARTICLES;
	VectorCopy( skyportal->vieworg, rn.pvsOrigin );

	rn.farClip = R_DefaultFarClip();

	rn.clipFlags = 15;
	rn.shadowGroup = NULL;
	rn.meshlist = &r_skyportallist;
	rn.portalmasklist = NULL;
	//Vector4Set( rn.scissor, rn.refdef.x + x, rn.refdef.y + y, w, h );

	if( skyportal->noEnts ) {
		rn.renderFlags |= RF_ENVVIEW;
	}

	if( skyportal->scale )
	{
		vec3_t centre, diff;

		VectorAdd( rsh.worldModel->mins, rsh.worldModel->maxs, centre );
		VectorScale( centre, 0.5f, centre );
		VectorSubtract( centre, rn.viewOrigin, diff );
		VectorMA( skyportal->vieworg, -skyportal->scale, diff, rn.refdef.vieworg );
	}
	else
	{
		VectorCopy( skyportal->vieworg, rn.refdef.vieworg );
	}

	// FIXME
	if( !VectorCompare( skyportal->viewanglesOffset, vec3_origin ) )
	{
		vec3_t angles;
		mat3_t axis;

		Matrix3_Copy( rn.refdef.viewaxis, axis );
		VectorInverse( &axis[AXIS_RIGHT] );
		Matrix3_ToAngles( axis, angles );

		VectorAdd( angles, skyportal->viewanglesOffset, angles );
		AnglesToAxis( angles, axis );
		Matrix3_Copy( axis, rn.refdef.viewaxis );
	}

	rn.refdef.rdflags &= ~( RDF_UNDERWATER|RDF_CROSSINGWATER|RDF_SKYPORTALINVIEW );
	if( skyportal->fov )
	{
		rn.refdef.fov_x = skyportal->fov;
		rn.refdef.fov_y = CalcFov( rn.refdef.fov_x, rn.refdef.width, rn.refdef.height );
		AdjustFov( &rn.refdef.fov_x, &rn.refdef.fov_y, frame->textureBuffers.screen.width, frame->textureBuffers.screen.height, false );
	}

	R_RenderView( frame, &rn.refdef );

	// restore modelview and projection matrices, scissoring, etc for the main view
	R_PopRefInst(frame);
}
