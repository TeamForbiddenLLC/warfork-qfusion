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

r_scene_2_t *R_CreateScene_2()
{
	r_scene_2_t *scene = R_Malloc( sizeof( r_scene_2_t ) );
	memset( scene, 0, sizeof( r_scene_2_t ) );
	scene->skmCachePool = R_AllocPool( r_mempool, "SKM Cache" );
	return scene;
}
