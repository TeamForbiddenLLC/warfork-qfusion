#include "r_scene_2.h"
#include "stb_ds.h"

void R_ClearScene_2( r_scene_2_t *scene )
{
	scene->entityIdx = 0;
	scene->dLightIdx = 0;
	scene->numPolys = 0;
	scene->numShadowGroups = 0;

	memset( scene->entShadowGroups, 0, sizeof( *scene->entShadowGroups ) * MAX_REF_ENTITIES );
	memset( scene->entShadowBits, 0, sizeof( *scene->entShadowBits ) * MAX_REF_ENTITIES );
	memset( scene->r_shadowGroups_hash, 0, sizeof( scene->r_shadowGroups_hash ) );

	scene->worldEntityIdx = scene->entityIdx++;
	scene->polyentIdx = scene->entityIdx++;
	scene->skyentIdx = scene->entityIdx++;
	scene->beginLocalIdx = scene->entityIdx;

	entity_t *worldEntity = &scene->entities[scene->worldEntityIdx];
	entity_t *polyEntity = &scene->entities[scene->polyentIdx];
	entity_t *skyEntity = &scene->entities[scene->skyentIdx];

	worldEntity->scale = 1.0f;
	worldEntity->model = rsh.worldModel;
	worldEntity->rtype = RT_MODEL;
	Matrix3_Identity( worldEntity->axis );

	polyEntity->scale = 1.0f;
	polyEntity->model = NULL;
	polyEntity->rtype = RT_MODEL;
	Matrix3_Identity( polyEntity->axis );
	( *skyEntity ) = ( *worldEntity );

	scene->numBmodelEntities = 0;
	scene->renderedShadowBits = 0;

	skmcacheentry_2_t *cache = scene->skmcache_head;
	while( cache ) {
		skmcacheentry_2_t *next = cache->next;
		cache->next = scene->skmcache_free;
		scene->skmcache_free = cache;
		cache = next;
	}
	scene->skmcache_head = NULL;
}

void R_AddLightToScene_2( r_scene_2_t *scene, const vec3_t org, float intensity, float r, float g, float b )
{
	if( intensity && ( r != 0 || g != 0 || b != 0 ) ) {
		if( scene->dLightIdx >= MAX_DLIGHTS ) {
			Com_Printf( "error: max number of lights" );
			return;
		}
		assert( scene->dLightIdx < MAX_DLIGHTS );
		dlight_t *dl = &scene->dlights[scene->dLightIdx++];
		VectorCopy( org, dl->origin );
		dl->intensity = intensity * DLIGHT_SCALE;
		dl->color[0] = r;
		dl->color[1] = g;
		dl->color[2] = b;

		if( r_lighting_grayscale->integer ) {
			vec_t grey = ColorGrayscale( dl->color );
			dl->color[0] = dl->color[1] = dl->color[2] = bound( 0, grey, 1 );
		}
	}
}

void R_AddEntityToScene_2( r_scene_2_t *scene, const entity_t *ent )
{
	if( !r_drawentities->integer )
		return;
	assert( ent );
	assert( scene->entityIdx < MAX_ENTITIES );

	if( scene->entityIdx >= MAX_POLYS ) {
		Com_Printf( "error: max number of entities" );
		return;
	}

	size_t idx = scene->entityIdx++;
	entity_t *de = &scene->entities[idx];

	*de = *ent;
	if( r_outlines_scale->value <= 0 )
		de->outlineHeight = 0;
	scene->entShadowBits[idx] = 0;
	scene->entShadowGroups[idx] = 0;

	if( de->rtype == RT_MODEL ) {
		if( de->model && de->model->type == mod_brush ) {
			rsc.bmodelEntities[rsc.numBmodelEntities++] = de;
		}
		if( !( de->renderfx & RF_NOSHADOW ) ) {
			R_AddLightOccluder( de ); // build groups and mark shadow casters
		}
	} else if( de->rtype == RT_SPRITE ) {
		// simplifies further checks
		de->model = NULL;
	}

	if( de->renderfx & RF_ALPHAHACK ) {
		if( de->shaderRGBA[3] == 255 ) {
			de->renderfx &= ~RF_ALPHAHACK;
		}
	}

	// add invisible fake entity for depth write
	if( ( de->renderfx & ( RF_WEAPONMODEL | RF_ALPHAHACK ) ) == ( RF_WEAPONMODEL | RF_ALPHAHACK ) ) {
		entity_t tent = *ent;
		tent.renderfx &= ~RF_ALPHAHACK;
		tent.renderfx |= RF_NOCOLORWRITE | RF_NOSHADOW;
		R_AddEntityToScene_2( scene, &tent );
	}
}
void R_AddPolyToScene_2( r_scene_2_t *scene, const poly_t *poly )
{
	assert( sizeof( *poly->elems ) == sizeof( elem_t ) );
	if( poly && poly->numverts ) {
		assert( scene->numPolys < MAX_POLYS );
		if( scene->numPolys >= MAX_POLYS ) {
			Com_Printf( "error: max number of poly" );
			return;
		}
		drawSurfacePoly_t *dp = &rsc.polys[rsc.numPolys++];

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
		dp->elems = (elem_t *)poly->elems;
		dp->fogNum = poly->fognum;

		// if fogNum is unset, we need to find the volume for polygon bounds
		if( !dp->fogNum ) {
			vec3_t dpmins, dpmaxs;
			ClearBounds( dpmins, dpmaxs );

			for(size_t i = 0; i < dp->numVerts; i++ ) {
				AddPointToBounds( dp->xyzArray[i], dpmins, dpmaxs );
			}

			mfog_t *fog = R_FogForBounds( dpmins, dpmaxs );
			dp->fogNum = ( fog ? fog - rsh.worldBrushModel->fogs + 1 : -1 );
		}
	}
}

void R_AddLightStyleToScene_2(r_scene_2_t* scene, int style, float r, float g, float b ) {
	if( style < 0 || style >= MAX_LIGHTSTYLES )
		ri.Com_Error( ERR_DROP, "R_AddLightStyleToScene: bad light style %i", style );

	lightstyle_t *const ls = &scene->lightStyles[style];
	ls->rgb[0] = max( 0, r );
	ls->rgb[1] = max( 0, g );
	ls->rgb[2] = max( 0, b );
}

r_scene_2_t *R_CreateScene_2()
{
	r_scene_2_t *scene = R_Malloc( sizeof( r_scene_2_t ) );
	memset( scene, 0, sizeof( r_scene_2_t ) );
	scene->skmCachePool = R_AllocPool( r_mempool, "SKM Cache" );
	return scene;
}

static void R_ComputeShadowmapBounds( void )
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


void R_RenderScene_2(r_scene_2_t* scene, const refdef_t *fd ) {
	int fbFlags = 0;
	int ppFrontBuffer = 0;
	image_t *ppSource;

	if( r_norefresh->integer )
		return;

	R_Set2DMode( false );

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

	fbFlags = 0;
	rn.fbColorAttachment = rn.fbDepthAttachment = NULL;
	
	if( !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		if( r_soft_particles->integer && ( rsh.screenTexture != NULL ) ) {
			rn.fbColorAttachment = rsh.screenTexture;
			rn.fbDepthAttachment = rsh.screenDepthTexture;
			rn.renderFlags |= RF_SOFT_PARTICLES;
			fbFlags |= 1;
		}

		if( rsh.screenPPCopies[0] && rsh.screenPPCopies[1] ) {
			int oldFlags = fbFlags;
			shader_t *cc = rn.refdef.colorCorrection;

			if( r_fxaa->integer ) {
				fbFlags |= 2;
			}

			if( cc && cc->numpasses > 0 && cc->passes[0].images[0] && cc->passes[0].images[0] != rsh.noTexture ) {
				fbFlags |= 4;
			}

			if( fbFlags != oldFlags ) {
				if( !rn.fbColorAttachment ) {
					rn.fbColorAttachment = rsh.screenPPCopies[0];
					ppFrontBuffer = 1;
				}
			}
		}
	}

	ppSource = rn.fbColorAttachment;

	// clip new scissor region to the one currently set
	Vector4Set( rn.scissor, fd->scissor_x, fd->scissor_y, fd->scissor_width, fd->scissor_height );
	Vector4Set( rn.viewport, fd->x, fd->y, fd->width, fd->height );
	VectorCopy( fd->vieworg, rn.pvsOrigin );
	VectorCopy( fd->vieworg, rn.lodOrigin );

 // R_BindFrameBufferObject( 0 );

  R_ComputeShadowmapBounds();

 // R_RenderView( fd );

 // R_RenderDebugSurface( fd );

 // R_RenderDebugBounds();

 // R_BindFrameBufferObject( 0 );

 // R_Set2DMode( true );

	if( !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		ri.Mutex_Lock( rf.speedsMsgLock );
		R_WriteSpeedsMessage( rf.speedsMsg, sizeof( rf.speedsMsg ) );
		ri.Mutex_Unlock( rf.speedsMsgLock );
	}

	// blit and blend framebuffers in proper order

	if( fbFlags == 1 ) {
		// only blit soft particles directly when we don't have any other post processing
		// otherwise use the soft particles FBO as the base texture on the next layer
		// to avoid wasting time on resolves and the fragment shader to blit to a temp texture
		//R_BlitTextureToScrFbo( fd,
		//	ppSource, 0,
		//	GLSL_PROGRAM_TYPE_NONE,
		//	colorWhite, 0,
		//	0, NULL );
	}
	fbFlags &= ~1;

	// apply FXAA
	if( fbFlags & 2 ) {
		image_t *dest;

		fbFlags &= ~2;
		dest = fbFlags ? rsh.screenPPCopies[ppFrontBuffer] : NULL;

	 // R_BlitTextureToScrFbo( fd,
	 // 	ppSource, dest ? dest->fbo : 0,
	 // 	GLSL_PROGRAM_TYPE_FXAA,
	 // 	colorWhite, 0,
	 // 	0, NULL );

		ppFrontBuffer ^= 1;
		ppSource = dest;
	}

	// apply color correction
	if( fbFlags & 4 ) {
		image_t *dest;

		fbFlags &= ~4;
		dest = fbFlags ? rsh.screenPPCopies[ppFrontBuffer] : NULL;

	 // R_BlitTextureToScrFbo( fd,
	 // 	ppSource, dest ? dest->fbo : 0,
	 // 	GLSL_PROGRAM_TYPE_COLORCORRECTION,
	 // 	colorWhite, 0,
	 // 	1, &( rn.refdef.colorCorrection->passes[0].images[0] ) );
	}

}
