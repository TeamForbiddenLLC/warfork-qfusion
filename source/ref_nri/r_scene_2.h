

#include "r_local.h"
#include "r_shadow.h"

#ifndef R_SCENE_2_H
#define R_SCENE_2_H

#define SHADOWGROUPS_HASH_SIZE	8

typedef struct skmcacheentry_s
{
	size_t size;
	uint8_t *data;
	struct skmcacheentry_s *next;
} skmcacheentry_2_t;

typedef struct
{
  mempool_t* skmCachePool;
	
	size_t entityIdx;
	size_t beginLocalIdx;
	entity_t entities[MAX_REF_ENTITIES];
	size_t worldEntityIdx;
	size_t polyentIdx;
	size_t skyentIdx;

	unsigned int dLightIdx;
	dlight_t dlights[MAX_DLIGHTS];

	unsigned int numPolys;
	drawSurfacePoly_t polys[MAX_POLYS];

	lightstyle_t lightStyles[MAX_LIGHTSTYLES];

	unsigned int numBmodelEntities;
	entity_t *bmodelEntities[MAX_REF_ENTITIES];

	unsigned int maxShadowGroups;
	unsigned int numShadowGroups;
	shadowGroup_t shadowGroups[MAX_REF_ENTITIES];
	unsigned int entShadowGroups[MAX_REF_ENTITIES];
	unsigned int entShadowBits[MAX_REF_ENTITIES];
  shadowGroup_t *r_shadowGroups_hash[SHADOWGROUPS_HASH_SIZE];
	
	float farClipMin, farClipBias;

	unsigned int renderedShadowBits;

	skmcacheentry_2_t *skmcache_head;											// actual entries are linked to this
	skmcacheentry_2_t *skmcache_free;											// actual entries are linked to this
	skmcacheentry_2_t *skmcachekeys[MAX_REF_ENTITIES * ( MOD_MAX_LODS + 1 )]; // entities linked to cache entries

	refdef_t refdef;
} r_scene_2_t;

size_t R_NextEntityIdx(r_scene_2_t* scene);

r_scene_2_t* R_CreateScene_2();
void R_ClearScene_2(r_scene_2_t* scene);
void R_AddEntityToScene_2(r_scene_2_t* scene, const entity_t *ent );
void R_AddLightToScene_2(r_scene_2_t* scene, const vec3_t org, float intensity, float r, float g, float b );
void R_AddPolyToScene_2(r_scene_2_t* scene, const poly_t *poly );
void R_AddLightStyleToScene_2( int style, float r, float g, float b );


#endif
