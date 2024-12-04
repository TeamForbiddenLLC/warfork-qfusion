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
#include "r_math_util.h"
#include "r_nri.h"
#include "stb_ds.h"

#define SHADOWMAP_ORTHO_NUDGE			8
#define SHADOWMAP_MIN_VIEWPORT_SIZE		16
#define SHADOWMAP_MAX_LOD				15
#define SHADOWMAP_LODBIAS			  4	

#define SHADOWGROUPS_HASH_SIZE	8
static shadowGroup_t *r_shadowGroups_hash[SHADOWGROUPS_HASH_SIZE];


void R_ShutdownShadows() {
	for(size_t frameIdx = 0; frameIdx < NUMBER_FRAMES_FLIGHT; frameIdx++) {
		for(size_t portalIdx = 0; portalIdx < MAX_PORTAL_TEXTURES; portalIdx++) {
			struct shadow_fb_s *fb = &rsh.shadowFBs[frameIdx][portalIdx];
			if(fb->depthTexture) {
				rsh.nri.coreI.DestroyTexture(fb->depthTexture);
				rsh.nri.coreI.DestroyDescriptor(fb->shaderDescriptor.descriptor);
				rsh.nri.coreI.DestroyDescriptor(fb->depthAttachment.descriptor);
			}
			for( size_t i = 0; i < fb->numAllocations; i++ )
				rsh.nri.coreI.FreeMemory(fb->memory[i]);
			fb->numAllocations = 0;
		}
	}
	memset(rsh.shadowFBs, 0, sizeof(rsh.shadowFBs));
}

/*
* R_ClearShadowGroups
*/
void R_ClearShadowGroups( void )
{
	rsc.numShadowGroups = 0;
	memset( rsc.entShadowGroups, 0, sizeof( *rsc.entShadowGroups ) * MAX_REF_ENTITIES );
	memset( rsc.entShadowBits, 0, sizeof( *rsc.entShadowBits ) * MAX_REF_ENTITIES );
	memset( r_shadowGroups_hash, 0, sizeof( r_shadowGroups_hash ) );
}

/*
* R_AddLightOccluder
*/
bool R_AddLightOccluder( const entity_t *ent )
{
	int i;
	float maxSide;
	vec3_t origin;
	unsigned int hash_key;
	shadowGroup_t *group;
	mleaf_t *leaf;
	vec3_t mins, maxs, bbox[8];
	bool bmodelRotated = false;

	if( rn.refdef.rdflags & RDF_NOWORLDMODEL )
		return false;
	if( !ent->model || ent->model->type == mod_brush )
		return false;

	VectorCopy( ent->lightingOrigin, origin );
	if( ent->model->type == mod_brush )
	{
		vec3_t t;
		VectorAdd( ent->model->mins, ent->model->maxs, t );
		VectorMA( ent->origin, 0.5, t, origin );
	}

	if( VectorCompare( origin, vec3_origin ) )
		return false;

	// find lighting group containing entities with same lightingOrigin as ours
	hash_key = (unsigned int)( origin[0] * 7 + origin[1] * 5 + origin[2] * 3 );
	hash_key &= ( SHADOWGROUPS_HASH_SIZE-1 );

	for( group = r_shadowGroups_hash[hash_key]; group; group = group->hashNext )
	{
		if( VectorCompare( group->origin, origin ) )
			goto add; // found an existing one, add
	}

	if( rsc.numShadowGroups == MAX_SHADOWGROUPS )
		return false; // no free groups

	leaf = Mod_PointInLeaf( origin, rsh.worldModel );

	// start a new group
	group = &rsc.shadowGroups[rsc.numShadowGroups];
	memset( group, 0, sizeof( *group ) );
	group->id = group - rsc.shadowGroups + 1;
	group->bit = ( 1<<rsc.numShadowGroups );
	group->vis = Mod_ClusterPVS( leaf->cluster, rsh.worldModel );
	group->useOrtho = true;
	group->alpha = r_shadows_alpha->value;

	// clear group bounds
	VectorCopy( origin, group->origin );
	ClearBounds( group->mins, group->maxs );
	ClearBounds( group->visMins, group->visMaxs );

	// add to hash table
	group->hashNext = r_shadowGroups_hash[hash_key];
	r_shadowGroups_hash[hash_key] = group;

	rsc.numShadowGroups++;
add:
	// get model bounds
	if( ent->model->type == mod_alias )
		R_AliasModelBBox( ent, mins, maxs );
	else if( ent->model->type == mod_skeletal )
		R_SkeletalModelBBox( ent, mins, maxs );
	else if( ent->model->type == mod_brush )
		R_BrushModelBBox( ent, mins, maxs, &bmodelRotated );
	else
		ClearBounds( mins, maxs );

	maxSide = 0;
	for( i = 0; i < 3; i++ ) {
		if( mins[i] >= maxs[i] )
			return false;
		maxSide = max( maxSide, maxs[i] - mins[i] );
	}

	// ignore tiny objects
	if( maxSide < 10 ) {
		return false;
	}

	
	rsc.entShadowGroups[ent - rsc.entities] = group->id;
	if( ent->flags & RF_WEAPONMODEL )
		return true;

	if( ent->model->type == mod_brush )
	{
		VectorCopy( mins, group->mins );
		VectorCopy( maxs, group->maxs );
	}
	else
	{
		// rotate local bounding box and compute the full bounding box for this group
		R_TransformBounds( ent->origin, ent->axis, mins, maxs, bbox );
		for( i = 0; i < 8; i++ ) {
			AddPointToBounds( bbox[i], group->mins, group->maxs );
		}
	}

	// increase projection distance if needed
	VectorSubtract( group->mins, origin, mins );
	VectorSubtract( group->maxs, origin, maxs );
	group->radius = RadiusFromBounds( mins, maxs );
	group->projDist = max( group->projDist, group->radius + min( r_shadows_projection_distance->value, 64.0f ) );

	return true;
}

/*
* R_ComputeShadowmapBounds
*/
void R_BuildShadowGroups( void )
{
	unsigned int i;
	vec3_t lightDir;
	vec4_t lightDiffuse;
	vec3_t mins, maxs;
	shadowGroup_t *group;

	for( i = 0; i < rsc.numShadowGroups; i++ ) {
		group = rsc.shadowGroups + i;

		if( group->projDist <= 1.0f ) {
			group->bit = 0;
			continue;
		}

		// get projection dir from lightgrid
		R_LightForOrigin( group->origin, lightDir, group->lightAmbient, lightDiffuse, group->projDist, false );

		// prevent light dir from going upwards
		VectorSet( lightDir, -lightDir[0], -lightDir[1], -fabs( lightDir[2] ) );
		VectorNormalize2( lightDir, group->lightDir );

		VectorScale( group->lightDir, group->projDist, lightDir );
		VectorScale( group->lightDir, group->projDist * 2.0f, lightDir );
		VectorAdd( group->mins, lightDir, mins );
		VectorAdd( group->maxs, lightDir, maxs );

		AddPointToBounds( group->mins, group->visMins, group->visMaxs );
		AddPointToBounds( group->maxs, group->visMins, group->visMaxs );
		AddPointToBounds( mins, group->visMins, group->visMaxs );
		AddPointToBounds( maxs, group->visMins, group->visMaxs );

		VectorAdd( group->visMins, group->visMaxs, group->visOrigin );
		VectorScale( group->visOrigin, 0.5, group->visOrigin );
		VectorSubtract( group->visMins, group->visOrigin, mins );
		VectorSubtract( group->visMaxs, group->visOrigin, maxs );
		group->visRadius = RadiusFromBounds( mins, maxs );
	}
}

/*
* R_FitOccluder
*
* returns farclip value
*/
static float R_FitOccluder( const shadowGroup_t *group, refdef_t *refdef )
{
	int i;
	float x1, x2, y1, y2, z1, z2;
	int ix1, ix2, iy1, iy2, iz1, iz2;
	int sizex = refdef->width, sizey = refdef->height;
	int diffx, diffy;
	mat4_t cameraMatrix, projectionMatrix, cameraProjectionMatrix;
	bool useOrtho = refdef->rdflags & RDF_USEORTHO ? true : false;

	Matrix4_Modelview( refdef->vieworg, refdef->viewaxis, cameraMatrix );

	// use current view settings for first approximation
	if( useOrtho ) {
		Matrix4_OrthogonalProjection( -refdef->ortho_x, refdef->ortho_x, -refdef->ortho_y, refdef->ortho_y, 
			-group->projDist, group->projDist, projectionMatrix );
	}
	else {
		Matrix4_PerspectiveProjection( refdef->fov_x, refdef->fov_y, 
			Z_NEAR, group->projDist, rf.cameraSeparation, projectionMatrix );
	}

	Matrix4_Multiply( projectionMatrix, cameraMatrix, cameraProjectionMatrix );

	// compute optimal fov to increase depth precision (so that shadow group objects are
	// as close to the nearplane as possible)
	// note that it's suboptimal to use bbox calculated in worldspace (FIXME)
	x1 = y1 = z1 = 999999;
	x2 = y2 = z2 = -999999;
	for( i = 0; i < 8; i++ )
	{
		// compute and rotate a full bounding box
		vec3_t v;
		vec4_t temp, temp2;

		temp[0] = ( ( i & 1 ) ? group->mins[0] : group->maxs[0] );
		temp[1] = ( ( i & 2 ) ? group->mins[1] : group->maxs[1] );
		temp[2] = ( ( i & 4 ) ? group->mins[2] : group->maxs[2] );
		temp[3] = 1.0f;

		// transform to screen space
		Matrix4_Multiply_Vector( cameraProjectionMatrix, temp, temp2 );

		if( temp2[3] ) {
			v[0] = ( temp2[0] / temp2[3] + 1.0f ) * 0.5f * refdef->width;
			v[1] = ( temp2[1] / temp2[3] + 1.0f ) * 0.5f * refdef->height;
			v[2] = ( temp2[2] / temp2[3] + 1.0f ) * 0.5f * group->projDist;
		} else {
			v[0] = 999999;
			v[1] = 999999;
			v[2] = 999999;
		}

		x1 = min( x1, v[0] ); y1 = min( y1, v[1] ); z1 = min( z1, v[2] );
		x2 = max( x2, v[0] ); y2 = max( y2, v[1] ); z2 = max( z2, v[2] );
	}

	// give it 1 pixel gap on both sides
	ix1 = x1 - 1.0f; ix2 = x2 + 1.0f;
	iy1 = y1 - 1.0f; iy2 = y2 + 1.0f;
	iz1 = z1 - 1.0f; iz2 = z2 + 1.0f;

	diffx = sizex - min( ix1, sizex - ix2 ) * 2;
	diffy = sizey - min( iy1, sizey - iy2 ) * 2;

	// adjust fov (for perspective projection)
	refdef->fov_x = 2 * RAD2DEG( atan( (float)diffx / (float)sizex ) );
	refdef->fov_y = 2 * RAD2DEG( atan( (float)diffy / (float)sizey ) );

	// adjust ortho clipping settings
	refdef->ortho_x = ix2 - ix1 + SHADOWMAP_ORTHO_NUDGE;
	refdef->ortho_y = iy2 - iy1 + SHADOWMAP_ORTHO_NUDGE;

	return useOrtho ? max( iz1, iz2 ) : group->projDist;
}

/*
* R_SetupShadowmapView
*/
static float R_SetupShadowmapView( shadowGroup_t *group, refdef_t *refdef, int lod )
{
	int width, height;
	float farClip;

	// clamp LOD to a sane value
	clamp( lod, 0, SHADOWMAP_MAX_LOD );

	struct shadow_fb_s* shadowmap = group->shadowmap;
	if(!shadowmap)
		return 0.0f;

	const NriTextureDesc *textureDesc = rsh.nri.coreI.GetTextureDesc( shadowmap->depthTexture );
	width = textureDesc->width >> lod;
	height = textureDesc->height >> lod;
	if( !width || !height )
		return 0.0f;

	refdef->x = 0;
	refdef->y = 0;
	refdef->width = width;
	refdef->height = height;
	// default fov to 90, R_SetupFrame will most likely alter the values to give depth more precision
	refdef->fov_x = 90;
	refdef->fov_y = CalcFov( refdef->fov_x, refdef->width, refdef->height );
	refdef->ortho_x = refdef->width;
	refdef->ortho_y = refdef->height;
	refdef->rdflags = group->useOrtho ? RDF_USEORTHO : 0;

	// set the view matrix
	// view axis are expected to be FLU (forward left up)
	NormalVectorToAxis( group->lightDir, refdef->viewaxis );
	VectorInverse( &refdef->viewaxis[AXIS_RIGHT] );

	// position the light source in the opposite direction
	VectorMA( group->origin, -group->projDist * 0.5, group->lightDir, refdef->vieworg );

	// attempt to maximize the area the occulder occupies in viewport
	farClip = R_FitOccluder( group, refdef );

	// store viewport and texture parameters for group, we'll need them later as GLSL uniforms
	group->viewportSize[0] = refdef->width;
	group->viewportSize[1] = refdef->height;
	group->textureSize[0] = textureDesc->width;
	group->textureSize[1] = textureDesc->height;

	return farClip;
}

static struct shadow_fb_s* __ResolveShadowSurface( struct frame_cmd_buffer_s* cmd,size_t i, int width, int height) {
	struct shadow_fb_s *bestFB = &rsh.shadowFBs[cmd->frameCount % NUMBER_FRAMES_FLIGHT][i];

	if( bestFB->depthTexture ) {
		const NriTextureDesc *textureDesc = rsh.nri.coreI.GetTextureDesc( bestFB->depthTexture );
		if(textureDesc->width == width  && textureDesc->height == height) {
			return bestFB;	
		}
	}

	if( bestFB->depthTexture )
		arrpush( cmd->freeTextures, bestFB->depthTexture );
	if( bestFB->depthAttachment.descriptor )
		arrpush( cmd->frameTemporaryDesc, bestFB->depthAttachment.descriptor );
	if( bestFB->shaderDescriptor.descriptor )
		arrpush( cmd->frameTemporaryDesc, bestFB->shaderDescriptor.descriptor );
	for( size_t i = 0; i < bestFB->numAllocations; i++ )
		arrpush( cmd->freeMemory, bestFB->memory[i] );
	bestFB->numAllocations = 0;

	const NriTextureDesc depthTextureDesc = { .width = width,
											  .height = height,
											  .depth = 1,
											  .usage = NriTextureUsageBits_DEPTH_STENCIL_ATTACHMENT | NriTextureUsageBits_SHADER_RESOURCE,
											  .layerNum = 1,
											  .format = ShadowDepthFormat,
											  .sampleNum = 1,
											  .type = NriTextureType_TEXTURE_2D,
											  .mipNum = 1 };
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture( rsh.nri.device, &depthTextureDesc, &bestFB->depthTexture ) );
	NriTexture *textures[] = { bestFB->depthTexture };
	const NriResourceGroupDesc resourceGroupDesc = {
		.textureNum = Q_ARRAY_COUNT( textures ),
		.textures = textures,
		.memoryLocation = NriMemoryLocation_DEVICE,
	};
	const size_t numAllocations = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
	assert( numAllocations <= Q_ARRAY_COUNT( bestFB->memory ) );
	bestFB->numAllocations = numAllocations;
	NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, bestFB->memory ) )
	NriDescriptor *descriptor = NULL;
	const NriTexture2DViewDesc textureAttachmentViewDesc = { .texture = bestFB->depthTexture, .viewType = NriTexture2DViewType_SHADER_RESOURCE_2D, .format = depthTextureDesc.format };
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &textureAttachmentViewDesc, &descriptor ) );
	bestFB->shaderDescriptor = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
	const NriTexture2DViewDesc depthAttachmentViewDesc = { .texture = bestFB->depthTexture, .viewType = NriTexture2DViewType_DEPTH_STENCIL_ATTACHMENT, .format = depthTextureDesc.format };
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &depthAttachmentViewDesc, &descriptor ) );
	bestFB->depthAttachment = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
	return bestFB;
}


/*
* R_DrawShadowmaps
*/
void R_DrawShadowmaps( struct frame_cmd_buffer_s* frame )
{
	const struct frame_cmd_save_attachment_s stash = R_CmdState_StashAttachment(frame); 
	float lodScale;
	vec3_t lodOrigin;
	vec3_t viewerOrigin;
	shadowGroup_t *group;
	int shadowBits = rn.shadowBits;
	refdef_t refdef;
	int lod;
	float farClip;
	float dist;

	if( !rsc.numShadowGroups )
		return;
	if( rn.renderFlags & RF_SHADOWMAPVIEW )
		return;
	if( rn.refdef.rdflags & RDF_NOWORLDMODEL )
		return;
	if( !shadowBits )
		return;

	if( !R_PushRefInst(frame) ) {
		return;
	}

	lodScale = rn.lod_dist_scale_for_fov;
	VectorCopy( rn.lodOrigin, lodOrigin );
	VectorCopy( rn.viewOrigin, viewerOrigin );

	refdef = rn.refdef;

	// find lighting group containing entities with same lightingOrigin as ours
	for(size_t i = 0; i < rsc.numShadowGroups; i++ )
	{
		if( !shadowBits ) {
			break;
		}

		group = rsc.shadowGroups + i;
		if( !( shadowBits & group->bit ) ) {
			continue;
		}
		shadowBits &= ~group->bit;

		// make sure we don't render the same shadowmap twice in the same scene frame
		if( rsc.renderedShadowBits & group->bit ) {
			continue;
		}

		// calculate LOD for shadowmap
		dist = DistanceFast( group->origin, lodOrigin );
		lod = (int)(dist * lodScale) / group->projDist - SHADOWMAP_LODBIAS;
		if( lod < 0 ) {
			lod = 0;
		}
		// allocate/resize the texture if needed
		struct shadow_fb_s* fb = __ResolveShadowSurface(frame, i, rsc.refdef.width, rsc.refdef.height);	
		group->shadowmap = fb;

		farClip = R_SetupShadowmapView( group, &refdef, lod );
		if( farClip <= 0.0f ) {
			continue;
		}

		// ignore shadowmaps of very low detail level
		if( refdef.width < SHADOWMAP_MIN_VIEWPORT_SIZE || refdef.height < SHADOWMAP_MIN_VIEWPORT_SIZE ) {
			continue;
		}
		const NriTextureDesc *textureDesc = rsh.nri.coreI.GetTextureDesc( fb->depthTexture );
		rn.farClip = farClip;
		rn.renderFlags = RF_SHADOWMAPVIEW|RF_FLIPFRONTFACE;
		rn.clipFlags |= 16; // clip by far plane too
		rn.meshlist = &r_shadowlist;
		rn.portalmasklist = NULL;
		rn.shadowGroup = group;
		rn.lod_dist_scale_for_fov = lodScale;
		VectorCopy( viewerOrigin, rn.pvsOrigin );
		VectorCopy( lodOrigin, rn.lodOrigin );

		// 3 pixels border on each side to prevent nasty stretching/bleeding of shadows,
		// also accounting for smoothing done in the fragment shader
		Vector4Set( rn.viewport, refdef.x + 3,refdef.y + textureDesc->height - refdef.height + 3, refdef.width - 6, refdef.height - 6 );
		Vector4Set( rn.scissor, refdef.x, refdef.y, textureDesc->width, textureDesc->height );

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
			.height = rn.scissor[3],
			
		} };
		FR_CmdSetTextureAttachment( frame, NULL, NULL, viewports, scissors, 0, ShadowDepthFormat, fb->depthAttachment.descriptor );
	
		const NriAccessLayoutStage layoutTransition = (NriAccessLayoutStage){	
			.layout = NriLayout_DEPTH_STENCIL_ATTACHMENT, 
			.access = NriAccessBits_DEPTH_STENCIL_ATTACHMENT_WRITE, 
			.stages = NriStageBits_DEPTH_STENCIL_ATTACHMENT        
		};

		{
			NriTextureBarrierDesc transitionBarriers = { 0 };
			transitionBarriers.texture = fb->depthTexture;
			transitionBarriers.after = layoutTransition;

			NriBarrierGroupDesc barrierGroupDesc = { 0 };
			barrierGroupDesc.textureNum = 1;
			barrierGroupDesc.textures = &transitionBarriers;
			rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
		}

		frame->state.pipelineLayout.flippedViewport = true;
		R_RenderView( frame, &refdef );
		frame->state.pipelineLayout.flippedViewport = false;

		{
			NriTextureBarrierDesc transitionBarriers = { 0 };
			transitionBarriers.texture = fb->depthTexture;
			transitionBarriers.before = layoutTransition;
			transitionBarriers.after = ( NriAccessLayoutStage ){ 
				.layout = NriLayout_SHADER_RESOURCE, 
				.access = NriAccessBits_SHADER_RESOURCE, 
				.stages = NriStageBits_FRAGMENT_SHADER 
			};

			NriBarrierGroupDesc barrierGroupDesc = { 0 };
			barrierGroupDesc.textureNum = 1;
			barrierGroupDesc.textures = &transitionBarriers;
			rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
		}

		Matrix4_Copy( rn.cameraProjectionMatrix, group->cameraProjectionMatrix );

		rsc.renderedShadowBits |= group->bit;
	}

	R_PopRefInst(frame);
	R_CmdState_RestoreAttachment(frame, &stash);
}
