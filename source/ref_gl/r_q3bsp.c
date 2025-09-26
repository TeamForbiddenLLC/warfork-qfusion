/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2002-2008 Victor Luchits

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

// r_q3bsp.c -- Q3 BSP model loading

#include "r_local.h"
#include "r_model.h"

typedef struct
{
	vec3_t mins, maxs;
	int flatness[2];
} mpatchgroup_t;

static model_t *loadmodel;
static int loadmodel_numverts;
static vec3_t *loadmodel_xyz_array;                       // vertexes
static vec3_t *loadmodel_normals_array;                   // normals
static vec2_t *loadmodel_st_array;                        // texture coords
static vec2_t *loadmodel_lmst_array[MAX_LIGHTMAPS];       // lightmap texture coords
static byte_vec4_t *loadmodel_colors_array[MAX_LIGHTMAPS];     // colors used for vertex lighting

static int loadmodel_numsurfelems;
static elem_t *loadmodel_surfelems;

static int loadmodel_numlightmaps;
static mlightmapRect_t *loadmodel_lightmapRects;

static int loadmodel_numshaderrefs;
static mshaderref_t *loadmodel_shaderrefs;

static int loadmodel_numsurfaces;
static rdface_t *loadmodel_dsurfaces;

static int loadmodel_numpatchgroups;
static int loadmodel_maxpatchgroups;
static mpatchgroup_t *loadmodel_patchgroups;
static int *loadmodel_patchgrouprefs;

// current model format descriptor
static const bspFormatDesc_t *mod_bspFormat;

/*
===============================================================================

BRUSHMODEL LOADING

===============================================================================
*/

static uint8_t *mod_base;
static mbrushmodel_t *loadbmodel;

/*
* Mod_CheckDeluxemaps
*/
static void Mod_CheckDeluxemaps( const lump_t *l, uint8_t *lmData )
{
	int i, j;
	int surfaces, lightmap;

	// there are no deluxemaps in the map if the number of lightmaps is
	// less than 2 or odd
	if( loadmodel_numlightmaps < 2 || loadmodel_numlightmaps & 1 )
		return;

	if( mod_bspFormat->flags & BSP_RAVEN )
	{
		rdface_t *in = ( void * )( mod_base + l->fileofs );

		surfaces = l->filelen / sizeof( *in );
		for( i = 0; i < surfaces; i++, in++ )
		{
			for( j = 0; j < MAX_LIGHTMAPS; j++ )
			{
				lightmap = LittleLong( in->lm_texnum[j] );
				if( lightmap <= 0 )
					continue;
				if( lightmap & 1 )
					return;
			}
		}
	}
	else
	{
		dface_t	*in = ( void * )( mod_base + l->fileofs );

		surfaces = l->filelen / sizeof( *in );
		for( i = 0; i < surfaces; i++, in++ )
		{
			lightmap = LittleLong( in->lm_texnum );
			if( lightmap <= 0 )
				continue;
			if( lightmap & 1 )
				return;
		}
	}

	// check if the deluxemap is actually empty (q3map2, yay!)
	if( loadmodel_numlightmaps == 2 )
	{
		int lW = mod_bspFormat->lightmapWidth, lH = mod_bspFormat->lightmapHeight;

		lmData += lW * lH * LIGHTMAP_BYTES;
		for( i = lW * lH; i > 0; i--, lmData += LIGHTMAP_BYTES )
		{
			for( j = 0; j < LIGHTMAP_BYTES; j++ )
			{
				if( lmData[j] )
					break;
			}
			if( j != LIGHTMAP_BYTES )
				break;
		}

		// empty deluxemap
		if( !i )
		{
			loadmodel_numlightmaps = 1;
			return;
		}
	}

	mapConfig.deluxeMaps = true;
	mapConfig.deluxeMappingEnabled = r_lighting_deluxemapping->integer ? true : false;
}


/*
* Mod_FaceToRavenFace
*/
static void Mod_FaceToRavenFace( const dface_t *in, rdface_t *rdf )
{
	int j;

	rdf->facetype = in->facetype;
	rdf->lm_texnum[0] = in->lm_texnum;
	rdf->vertexStyles[0] = 0;
	if( rdf->lightmapStyles[0] == 255 || LittleLong( in->lm_texnum ) < 0 || r_lighting_vertexlight->integer ) {
		rdf->lightmapStyles[0] = 255;
	}
	else {
		rdf->lightmapStyles[0] = 0;
	}

	for( j = 1; j < MAX_LIGHTMAPS; j++ )
	{
		rdf->lm_texnum[j] = LittleLong( -1 );
		rdf->lightmapStyles[j] = rdf->vertexStyles[j] = 255;
	}

	for( j = 0; j < 3; j++ )
	{
		rdf->origin[j] = in->origin[j];
		rdf->normal[j] = in->normal[j];
		rdf->mins[j] = in->mins[j];
		rdf->maxs[j] = in->maxs[j];
	}

	rdf->shadernum = in->shadernum;
	rdf->fognum = in->fognum;
	rdf->numverts = in->numverts;
	rdf->firstvert = in->firstvert;
	rdf->patch_cp[0] = in->patch_cp[0];
	rdf->patch_cp[1] = in->patch_cp[1];
	rdf->firstelem = in->firstelem;
	rdf->numelems = in->numelems;
}

/*
* Mod_PreloadFaces
*/
static void Mod_PreloadFaces( const lump_t *l )
{
	int i, j;
	rdface_t *in;

	if( mod_bspFormat->flags & BSP_RAVEN )
	{
		in = ( void * )( mod_base + l->fileofs );
		if( l->filelen % sizeof( *in ) )
			ri.Com_Error( ERR_DROP, "Mod_LoadFaces: funny lump size in %s", loadmodel->name );

		loadmodel_numsurfaces = l->filelen / sizeof( *in );
		loadmodel_dsurfaces = in;

		// verify lighting data
		for( i = 0; i < loadmodel_numsurfaces; i++, in++ ) {
			for( j = 0; j < MAX_LIGHTMAPS; j++ ) {
				int lmNum = LittleLong( in->lm_texnum[j] );
				// disable lightstyles for fullbright mode
				if( j > 0 && r_fullbright->integer ) {
					lmNum = -1;
					in->vertexStyles[j] = 255;
				}
				if( lmNum < 0 || in->lightmapStyles[j] == 255 || r_lighting_vertexlight->integer ) {
					in->lm_texnum[j] = LittleLong( -1 );
					in->lightmapStyles[j] = 255;
				}
			}
		}
	}
	else
	{
		dface_t	*din;

		din = ( void * )( mod_base + l->fileofs );
		if( l->filelen % sizeof( *din ) )
			ri.Com_Error( ERR_DROP, "Mod_LoadFaces: funny lump size in %s", loadmodel->name );

		loadmodel_numsurfaces = l->filelen / sizeof( *din );
		loadmodel_dsurfaces = in = Q_CallocAligned(loadmodel_numsurfaces, 16, sizeof( *in ) );
		Q_LinkToPool(in, loadmodel->mempool);

		// convert from q3a format to rtcw/qfusion format
		for( i = 0; i < loadmodel_numsurfaces; i++, din++, in++ ) {
			Mod_FaceToRavenFace( din, in );
		}
	}

	// preload shaders (images will start loading in background threads while we're still busy with the map)
	in = loadmodel_dsurfaces;
	for( i = 0; i < loadmodel_numsurfaces; i++, in++ ) {
		// load shader
		int shaderNum;
		mshaderref_t *shaderRef;
		shaderType_e shaderType;

		shaderNum = LittleLong( in->shadernum );
		if( shaderNum < 0 || shaderNum >= loadmodel_numshaderrefs )
			ri.Com_Error( ERR_DROP, "MOD_LoadBmodel: bad shader number" );
		shaderRef = loadmodel_shaderrefs + shaderNum;
		if( !shaderRef->name[0] ) {
			continue;
		}

		if( in->lightmapStyles[0] == 255 ) {
			shaderType = SHADER_TYPE_VERTEX;
		} else {
			shaderType = SHADER_TYPE_DELUXEMAP;
		}

		if( !shaderRef->shaders[shaderType-SHADER_TYPE_BSP_MIN] ) {
			shaderRef->shaders[shaderType-SHADER_TYPE_BSP_MIN] = R_RegisterShader( shaderRef->name, shaderType );
		}
	}
}

/*
* Mod_LoadFaces
*/
static void Mod_LoadFaces( const lump_t *l )
{
	int i;
	int count;
	const rdface_t *in;
	msurface_t *out;

	in = loadmodel_dsurfaces;
	count = loadmodel_numsurfaces;
	
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);

	loadbmodel->surfaces = out;
	loadbmodel->numsurfaces = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		int j;
		int fogNum;
		mshaderref_t *shaderRef;
		shaderType_e shaderType;
		mlightmapRect_t *lmRects[MAX_LIGHTMAPS];
		int lightmaps[MAX_LIGHTMAPS];
		uint8_t lightmapStyles[MAX_LIGHTMAPS], vertexStyles[MAX_LIGHTMAPS];

		out->facetype = LittleLong( in->facetype );

		// lighting info
		for( j = 0; j < MAX_LIGHTMAPS; j++ )
		{
			lightmaps[j] = LittleLong( in->lm_texnum[j] );
			lightmapStyles[j] = in->lightmapStyles[j];
			vertexStyles[j] = in->vertexStyles[j];

			if( in->lightmapStyles[j] == 255 || lightmaps[j] >= loadmodel_numlightmaps )
			{
				lmRects[j] = NULL;
				lightmaps[j] = -1;
				lightmapStyles[j] = 255;
			}
			else
			{
				lmRects[j] = &loadmodel_lightmapRects[lightmaps[j]];
				lightmaps[j] = lmRects[j]->texNum;			
			}			
		}

		// add this super style
		out->superLightStyle = R_AddSuperLightStyle( loadmodel, lightmaps, lightmapStyles, vertexStyles, lmRects );

		// load shader
		shaderRef = loadmodel_shaderrefs + LittleLong( in->shadernum );
		if( lightmapStyles[0] == 255 ) {
			shaderType = SHADER_TYPE_VERTEX;
		} else {
			shaderType = SHADER_TYPE_DELUXEMAP;
		}
		
		out->shader = shaderRef->shaders[shaderType-SHADER_TYPE_BSP_MIN];
		out->flags = shaderRef->flags;

		fogNum = LittleLong( in->fognum );
		if( fogNum >= 0 && ( (unsigned)fogNum < loadbmodel->numfogs ) )
		{
			mfog_t *fog = loadbmodel->fogs + fogNum;
			if( fog->shader && fog->shader->fog_dist )
				out->fog = fog;
		}
	}
}

/*
* Mod_LoadVertexes
*/
static void Mod_LoadVertexes( const lump_t *l )
{
	int i, count, j;
	dvertex_t *in;
	float *out_xyz, *out_normals, *out_st, *out_lmst;
	uint8_t *buffer, *out_colors;
	size_t bufSize;
	vec3_t color;
	float div = (float)( 1 << mapConfig.overbrightBits ) * mapConfig.lightingIntensity / 255.0f;

	in = ( void * )( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadVertexes: funny lump size in %s", loadmodel->name );
	count = l->filelen / sizeof( *in );

	bufSize = 0;
	bufSize += count * ( sizeof( vec3_t ) + sizeof( vec3_t ) + sizeof( vec2_t )*2 + sizeof( byte_vec4_t ) );
	buffer = Q_MallocAligned(16, bufSize );
	Q_LinkToPool(buffer, loadmodel->mempool);

	loadmodel_numverts = count;
	loadmodel_xyz_array = ( vec3_t * )buffer; buffer += count*sizeof( vec3_t );
	loadmodel_normals_array = ( vec3_t * )buffer; buffer += count*sizeof( vec3_t );
	loadmodel_st_array = ( vec2_t * )buffer; buffer += count*sizeof( vec2_t );
	loadmodel_lmst_array[0] = ( vec2_t * )buffer; buffer += count*sizeof( vec2_t );
	loadmodel_colors_array[0] = ( byte_vec4_t * )buffer; buffer += count*sizeof( byte_vec4_t );
	for( i = 1; i < MAX_LIGHTMAPS; i++ )
	{
		loadmodel_lmst_array[i] = loadmodel_lmst_array[0];
		loadmodel_colors_array[i] = loadmodel_colors_array[0];
	}

	out_xyz = loadmodel_xyz_array[0];
	out_normals = loadmodel_normals_array[0];
	out_st = loadmodel_st_array[0];
	out_lmst = loadmodel_lmst_array[0][0];
	out_colors = loadmodel_colors_array[0][0];

	for( i = 0; i < count; i++, in++, out_xyz += 3, out_normals += 3, out_st += 2, out_lmst += 2, out_colors += 4 )
	{
		for( j = 0; j < 3; j++ )
		{
			out_xyz[j] = LittleFloat( in->point[j] );
			out_normals[j] = LittleFloat( in->normal[j] );
		}

		for( j = 0; j < 2; j++ )
		{
			out_st[j] = LittleFloat( in->tex_st[j] );
			out_lmst[j] = LittleFloat( in->lm_st[j] );
		}

		if( r_fullbright->integer )
		{
			out_colors[0] = 255;
			out_colors[1] = 255;
			out_colors[2] = 255;
			out_colors[3] = in->color[3];
		}
		else
		{
			color[0] = ( ( float )in->color[0] * div );
			color[1] = ( ( float )in->color[1] * div );
			color[2] = ( ( float )in->color[2] * div );
			ColorNormalize( color, color );

			// convert to grayscale if monochrome lighting is enabled
			if( r_lighting_grayscale->integer ) {
				vec_t grey = ColorGrayscale( color );
				color[0] = color[1] = color[2] = bound( 0, grey, 1 );
			}

			out_colors[0] = ( uint8_t )( color[0] * 255 );
			out_colors[1] = ( uint8_t )( color[1] * 255 );
			out_colors[2] = ( uint8_t )( color[2] * 255 );
			out_colors[3] = in->color[3];
		}
	}
}

/*
* Mod_LoadVertexes_RBSP
*/
static void Mod_LoadVertexes_RBSP( const lump_t *l )
{
	int i, count, j;
	rdvertex_t *in;
	float *out_xyz, *out_normals, *out_st, *out_lmst[MAX_LIGHTMAPS];
	uint8_t *buffer, *out_colors[MAX_LIGHTMAPS];
	size_t bufSize;
	vec3_t color;
	float div = (float)( 1 << mapConfig.overbrightBits ) * mapConfig.lightingIntensity / 255.0f;

	in = ( void * )( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadVertexes: funny lump size in %s", loadmodel->name );
	count = l->filelen / sizeof( *in );

	bufSize = 0;
	bufSize += count * ( sizeof( vec3_t ) + sizeof( vec3_t ) + sizeof( vec2_t ) + ( sizeof( vec2_t ) + sizeof( byte_vec4_t ) )*MAX_LIGHTMAPS );
	buffer = Q_MallocAligned(16, bufSize );
	Q_LinkToPool(buffer, loadmodel->mempool);
	memset(buffer, 0, bufSize);

	loadmodel_numverts = count;
	loadmodel_xyz_array = ( vec3_t * )buffer; buffer += count*sizeof( vec3_t );
	loadmodel_normals_array = ( vec3_t * )buffer; buffer += count*sizeof( vec3_t );
	loadmodel_st_array = ( vec2_t * )buffer; buffer += count*sizeof( vec2_t );
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		loadmodel_lmst_array[i] = ( vec2_t * )buffer; buffer += count*sizeof( vec2_t );
		loadmodel_colors_array[i] = ( byte_vec4_t * )buffer; buffer += count*sizeof( byte_vec4_t );
	}

	out_xyz = loadmodel_xyz_array[0];
	out_normals = loadmodel_normals_array[0];
	out_st = loadmodel_st_array[0];
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		out_lmst[i] = loadmodel_lmst_array[i][0];
		out_colors[i] = loadmodel_colors_array[i][0];
	}

	for( i = 0; i < count; i++, in++, out_xyz += 3, out_normals += 3, out_st += 2 )
	{
		for( j = 0; j < 3; j++ )
		{
			out_xyz[j] = LittleFloat( in->point[j] );
			out_normals[j] = LittleFloat( in->normal[j] );
		}

		for( j = 0; j < 2; j++ )
			out_st[j] = LittleFloat( in->tex_st[j] );

		for( j = 0; j < MAX_LIGHTMAPS; out_lmst[j] += 2, out_colors[j] += 4, j++ )
		{
			out_lmst[j][0] = LittleFloat( in->lm_st[j][0] );
			out_lmst[j][1] = LittleFloat( in->lm_st[j][1] );

			if( r_fullbright->integer )
			{
				out_colors[j][0] = 255;
				out_colors[j][1] = 255;
				out_colors[j][2] = 255;
				out_colors[j][3] = in->color[j][3];
			}
			else
			{
				color[0] = ( ( float )in->color[j][0] * div );
				color[1] = ( ( float )in->color[j][1] * div );
				color[2] = ( ( float )in->color[j][2] * div );
				ColorNormalize( color, color );

				// convert to grayscale if monochrome lighting is enabled
				if( r_lighting_grayscale->integer ) {
					vec_t grey = ColorGrayscale( color );
					color[0] = color[1] = color[2] = bound( 0, grey, 1 );
				}

				out_colors[j][0] = ( uint8_t )( color[0] * 255 );
				out_colors[j][1] = ( uint8_t )( color[1] * 255 );
				out_colors[j][2] = ( uint8_t )( color[2] * 255 );
				out_colors[j][3] = in->color[j][3];
			}
		}
	}
}

/*
* Mod_AddUpdatePatchGroup
*/
static int Mod_AddUpdatePatchGroup( const rdface_t *in )
{
	int i;
	int patch_cp[2], flatness[2];
	float subdivLevel;
	vec3_t lodMins, lodMaxs;
	int inFirstVert;
	mpatchgroup_t *group;

	patch_cp[0] = LittleLong( in->patch_cp[0] );
	patch_cp[1] = LittleLong( in->patch_cp[1] );
	if( !patch_cp[0] || !patch_cp[1] ) {
		return -1;
	}

	// load LOD group bounds
	for( i = 0; i < 3; i++ ) {
		lodMins[i] = in->mins[i];
		lodMaxs[i] = in->maxs[i];
	}

	subdivLevel = bound( SUBDIVISIONS_MIN, r_subdivisions->value, SUBDIVISIONS_MAX );
	inFirstVert = LittleLong( in->firstvert );

	// find the degree of subdivision in the u and v directions
	Patch_GetFlatness( subdivLevel, (vec_t *)loadmodel_xyz_array[inFirstVert], 3, patch_cp, flatness );

	// track LOD bounds, which hold group of all curves that must subdivide the same to avoid cracking
	for( i = 0, group = loadmodel_patchgroups; i < loadmodel_numpatchgroups; i++, group++ ) {
		if( VectorCompare( group->mins, lodMins ) && VectorCompare( group->maxs, lodMaxs ) ) {
			break;
		}
	}

	// new group
	if( i == loadmodel_numpatchgroups ) {
		if( i == loadmodel_maxpatchgroups ) {
			assert( 0 );
			Com_Printf( S_COLOR_YELLOW "Mod_AddUpdatePatchGroup: i == loadmodel_maxpatchgroups\n" );
			return -1;
		}

		VectorCopy( lodMins, group->mins );
		VectorCopy( lodMaxs, group->maxs );
		group->flatness[0] = flatness[0];
		group->flatness[1] = flatness[1];

		loadmodel_numpatchgroups++;
	}
	else {
		group->flatness[0] = max( group->flatness[0], flatness[0] );
		group->flatness[1] = max( group->flatness[1], flatness[1] );
	}

	return i;
}

/*
* Mod_CreateMeshForSurface
*/
#define MESH_T_SIZE_ALIGNED ALIGN( sizeof( mesh_t ), sizeof( vec_t ) )
static mesh_t *Mod_CreateMeshForSurface( const rdface_t *in, msurface_t *out, int patchGroupRef )
{
	mesh_t *mesh = NULL;
	uint8_t *buffer;
	size_t bufSize, bufPos = 0;

	switch( out->facetype )
	{
	case FACETYPE_PATCH:
		{
			int i, j, u, v, p;
			int patch_cp[2], step[2], size[2], flat[2];
			int numVerts, numElems;
			int inFirstVert;
			bool hasLightmap[MAX_LIGHTMAPS];
			int numattribs = 0;
			uint8_t *attribs[2 + MAX_LIGHTMAPS * 2];
			int attribsizes[2 + MAX_LIGHTMAPS * 2];
			elem_t *elems;

			if( patchGroupRef < 0 ) {
				// not a patch at all
				break;
			}

			patch_cp[0] = LittleLong( in->patch_cp[0] );
			patch_cp[1] = LittleLong( in->patch_cp[1] );

			flat[0] = loadmodel_patchgroups[patchGroupRef].flatness[0];
			flat[1] = loadmodel_patchgroups[patchGroupRef].flatness[1];

			inFirstVert = LittleLong( in->firstvert );

			// allocate space for mesh
			step[0] = ( 1 << flat[0] );
			step[1] = ( 1 << flat[1] );
			size[0] = ( patch_cp[0] >> 1 ) * step[0] + 1;
			size[1] = ( patch_cp[1] >> 1 ) * step[1] + 1;
			numVerts = size[0] * size[1];
			numElems = ( size[0] - 1 ) * ( size[1] - 1 ) * 6;

			bufSize = MESH_T_SIZE_ALIGNED;
			bufSize += numVerts * ( sizeof( vec4_t ) + sizeof( vec4_t ) + sizeof( vec4_t ) + sizeof( vec2_t ) );
			for( j = 0; j < MAX_LIGHTMAPS; j++ )
			{
				hasLightmap[j] = ( ( in->lightmapStyles[j] != 255 ) && ( LittleLong( in->lm_texnum[j] ) >= 0 ) ) ? true : false;
				if( !hasLightmap[j] )
					break;
				bufSize += numVerts * sizeof( vec2_t );
			}
			if( mapConfig.lightmapArrays )
			{
				for( j = 0; j < MAX_LIGHTMAPS && hasLightmap[j]; j++ )
				{
					if( !( j & 3 ) )
						bufSize += numVerts * sizeof( byte_vec4_t );
				}
			}
			for( j = 0; j < MAX_LIGHTMAPS && in->vertexStyles[j] != 255; j++ )
				bufSize += numVerts * sizeof( byte_vec4_t );
			bufSize = ALIGN( bufSize, sizeof( elem_t ) ) + numElems * sizeof( elem_t );
			buffer = Q_Malloc(bufSize);
			memset(buffer, 0, bufSize);
			Q_LinkToPool(buffer, loadmodel->mempool);

			bufPos = 0;

			mesh = ( mesh_t * )buffer; bufPos += MESH_T_SIZE_ALIGNED;
			mesh->numVerts = numVerts;
			mesh->numElems = numElems;

			mesh->xyzArray = ( vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec4_t );
			mesh->normalsArray = ( vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec4_t );
			mesh->sVectorsArray = ( vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec4_t );
			mesh->stArray = ( vec2_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec2_t );

			Patch_Evaluate( vec_t, 3, loadmodel_xyz_array[inFirstVert], 
				patch_cp, step, mesh->xyzArray[0], 4 );

			attribs[numattribs] = ( uint8_t * )mesh->normalsArray[0];
			attribsizes[numattribs++] = sizeof( vec4_t );
			Patch_Evaluate( vec_t, 3, loadmodel_normals_array[inFirstVert],
				patch_cp, step, mesh->normalsArray[0], 4 );

			attribs[numattribs] = ( uint8_t * )mesh->stArray[0];
			attribsizes[numattribs++] = sizeof( vec2_t );
			Patch_Evaluate( vec_t, 2, loadmodel_st_array[inFirstVert], 
				patch_cp, step, mesh->stArray[0], 0 );

			for( j = 0; j < MAX_LIGHTMAPS && hasLightmap[j]; j++ )
			{
				mesh->lmstArray[j] = ( vec2_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec2_t );
				attribs[numattribs] = ( uint8_t * )mesh->lmstArray[j];
				attribsizes[numattribs++] = sizeof( vec2_t );
				Patch_Evaluate( vec_t, 2, loadmodel_lmst_array[j][inFirstVert], 
					patch_cp, step, mesh->lmstArray[j][0], 0 );
			}

			if( mapConfig.lightmapArrays )
			{
				for( j = 0; j < MAX_LIGHTMAPS && hasLightmap[j]; j++ )
				{
					if( !( j & 3 ) )
					{
						mesh->lmlayersArray[j >> 2] = ( byte_vec4_t * )( buffer + bufPos );
						bufPos += numVerts * sizeof( byte_vec4_t );
					}
				}
			}

			for( j = 0; j < MAX_LIGHTMAPS && in->vertexStyles[j] != 255; j++ )
			{
				mesh->colorsArray[j] = ( byte_vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( byte_vec4_t );
				attribs[numattribs] = ( uint8_t * )mesh->colorsArray[j];
				attribsizes[numattribs++] = sizeof( byte_vec4_t );
				Patch_Evaluate( uint8_t, 4, loadmodel_colors_array[j][inFirstVert], 
					patch_cp, step, mesh->colorsArray[j][0], 0 );
			}

			Patch_RemoveLinearColumnsRows( mesh->xyzArray[0], 4, &size[0], &size[1], numattribs, attribs, attribsizes );
			numVerts = size[0] * size[1];
			numElems = (size[0] - 1) * (size[1] - 1) * 6;
			if( numVerts != mesh->numVerts )
			{
				size_t normalsPos, sVectorsPos, stPos;
				size_t lmstPos[MAX_LIGHTMAPS], lmlayersPos[( MAX_LIGHTMAPS + 3 ) / 4], colorsPos[MAX_LIGHTMAPS];
				uint8_t *oldBuffer = buffer;

				mesh->numVerts = numVerts;
				mesh->numElems = numElems;

				bufPos = MESH_T_SIZE_ALIGNED + numVerts * sizeof( vec4_t );

				normalsPos = bufPos;
				memmove( buffer + normalsPos, mesh->normalsArray, numVerts * sizeof( vec4_t ) );
				bufPos += numVerts * sizeof( vec4_t );

				sVectorsPos = bufPos;
				bufPos += numVerts * sizeof( vec4_t );

				stPos = bufPos;
				memmove( buffer + stPos, mesh->stArray, numVerts * sizeof( vec2_t ) );
				bufPos += numVerts * sizeof( vec2_t );

				for( j = 0; j < MAX_LIGHTMAPS; j++ )
				{
					if( mesh->lmstArray[j] )
					{
						lmstPos[j] = bufPos;
						memmove( buffer + lmstPos[j], mesh->lmstArray[j], numVerts * sizeof( vec2_t ) );
						bufPos += numVerts * sizeof( vec2_t );
					}
					else
					{
						lmstPos[j] = 0;
					}
				}

				for( j = 0; j < ( MAX_LIGHTMAPS + 3 ) / 4; j++ )
				{
					if( mesh->lmlayersArray[j] )
					{
						lmlayersPos[j] = bufPos;
						// filled later, no copying here
						bufPos += numVerts * sizeof( byte_vec4_t );
					}
					else
					{
						lmlayersPos[j] = 0;
					}
				}

				for( j = 0; j < MAX_LIGHTMAPS; j++ )
				{
					if( mesh->colorsArray[j] )
					{
						colorsPos[j] = bufPos;
						memmove( buffer + colorsPos[j], mesh->colorsArray[j], numVerts * sizeof( byte_vec4_t ) );
						bufPos += numVerts * sizeof( byte_vec4_t );
					}
					else
					{
						colorsPos[j] = 0;
					}
				}

				bufSize = ALIGN( bufPos, sizeof( elem_t ) ) + numElems * sizeof( elem_t );
				buffer = Q_Malloc( bufSize );
				Q_LinkToPool( buffer, loadmodel->mempool );
				memcpy( buffer, oldBuffer, bufSize );
				Q_Free( oldBuffer );

				mesh = ( mesh_t * )buffer;
				mesh->xyzArray = ( vec4_t * )( buffer + MESH_T_SIZE_ALIGNED );
				mesh->normalsArray = ( vec4_t * )( buffer + normalsPos );
				mesh->sVectorsArray = ( vec4_t * )( buffer + sVectorsPos );
				mesh->stArray = ( vec2_t * )( buffer + stPos );
				for( j = 0; j < MAX_LIGHTMAPS; j++ )
				{
					if( lmstPos[j] )
						mesh->lmstArray[j] = ( vec2_t * )( buffer + lmstPos[j] );
					if( !( j & 3 ) && lmlayersPos[j >> 2] )
						mesh->lmlayersArray[j >> 2] = ( byte_vec4_t * )( buffer + lmlayersPos[j >> 2] );
					if( colorsPos[j] )
						mesh->colorsArray[j] = ( byte_vec4_t * )( buffer + colorsPos[j] );
				}
			}

			// compute new elems
			bufPos = ALIGN( bufPos, sizeof( elem_t ) );
			mesh->elems = elems = ( elem_t * )( buffer + bufPos ); bufPos += numElems * sizeof( elem_t );
			for( v = 0, i = 0; v < size[1] - 1; v++ )
			{
				for( u = 0; u < size[0] - 1; u++ )
				{
					p = v * size[0] + u;
					elems[0] = p;
					elems[1] = p + size[0];
					elems[2] = p + 1;
					elems[3] = p + 1;
					elems[4] = p + size[0];
					elems[5] = p + size[0] + 1;
					elems += 6;
				}
			}

			for( i = 0; i < numVerts; i++ )
			{
				mesh->xyzArray[i][3] = 1;
				mesh->normalsArray[i][3] = 0;
				VectorNormalize( mesh->normalsArray[i] );
			}

			R_BuildTangentVectors( mesh->numVerts, mesh->xyzArray, mesh->normalsArray, mesh->stArray, mesh->numElems / 3, mesh->elems, mesh->sVectorsArray );
			break;
		}
	case FACETYPE_PLANAR:
	case FACETYPE_TRISURF:
	case FACETYPE_FOLIAGE:
		{
			int j, numVerts, firstVert, numElems, firstElem;
			int numFoliageInstances;
			bool hasLightmap[MAX_LIGHTMAPS];

			if( out->facetype == FACETYPE_FOLIAGE )
			{
				// foliage needs special care for instanced drawing
				numFoliageInstances = LittleLong( in->patch_cp[0] );
				numVerts = LittleLong( in->patch_cp[1] );
			}
			else
			{
				numFoliageInstances = 0;
				numVerts = LittleLong( in->numverts );
			}

			firstVert = LittleLong( in->firstvert );

			numElems = LittleLong( in->numelems );
			firstElem = LittleLong( in->firstelem );

			bufSize = MESH_T_SIZE_ALIGNED;
			bufSize += numVerts * ( sizeof( vec4_t ) + sizeof( vec4_t ) + sizeof( vec4_t ) + sizeof( vec2_t ) );
			for( j = 0; j < MAX_LIGHTMAPS; j++ )
			{
				hasLightmap[j] = ( ( in->lightmapStyles[j] != 255 ) && ( LittleLong( in->lm_texnum[j] ) >= 0 ) ) ? true : false;
				if( !hasLightmap[j] )
					break;
				bufSize += numVerts * sizeof( vec2_t );
			}
			if( mapConfig.lightmapArrays )
			{
				for( j = 0; j < MAX_LIGHTMAPS && hasLightmap[j]; j++ )
				{
					if( !( j & 3 ) )
						bufSize += numVerts * sizeof( byte_vec4_t );
				}
			}
			for( j = 0; j < MAX_LIGHTMAPS && in->vertexStyles[j] != 255; j++ )
				bufSize += numVerts * sizeof( byte_vec4_t );
			bufSize = ALIGN( bufSize, sizeof( elem_t ) ) + numElems * sizeof( elem_t );
			if( out->facetype == FACETYPE_PLANAR )
				bufSize = ALIGN( bufSize, 16 ) + sizeof( cplane_t );
			bufSize = ALIGN( bufSize, 16 ) + numFoliageInstances * sizeof( instancePoint_t );

			buffer = Q_Malloc(  bufSize );
			memset( buffer, 0, bufSize );
			Q_LinkToPool(buffer, loadmodel->mempool);
			bufPos = 0;

			mesh = ( mesh_t * )buffer; bufPos += MESH_T_SIZE_ALIGNED;
			mesh->numVerts = numVerts;
			mesh->numElems = numElems;

			mesh->xyzArray = ( vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec4_t );
			mesh->normalsArray = ( vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec4_t );
			mesh->sVectorsArray = ( vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec4_t );
			mesh->stArray = ( vec2_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec2_t );

			for( j = 0; j < numVerts; j++ ) {
				VectorCopy( loadmodel_xyz_array[firstVert+j], mesh->xyzArray[j] );
				mesh->xyzArray[j][3] = 1;

				VectorCopy( loadmodel_normals_array[firstVert+j], mesh->normalsArray[j] );
				mesh->normalsArray[j][3] = 0;
			}

			memcpy( mesh->stArray, loadmodel_st_array + firstVert, numVerts * sizeof( vec2_t ) );

			for( j = 0; j < MAX_LIGHTMAPS && hasLightmap[j]; j++ )
			{
				mesh->lmstArray[j] = ( vec2_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( vec2_t );
				memcpy( mesh->lmstArray[j], loadmodel_lmst_array[j] + firstVert, numVerts * sizeof( vec2_t ) );
			}
			if( mapConfig.lightmapArrays )
			{
				for( j = 0; j < MAX_LIGHTMAPS && hasLightmap[j]; j++ )
				{
					if( !( j & 3 ) )
					{
						mesh->lmlayersArray[j >> 2] = ( byte_vec4_t * )( buffer + bufPos );
						bufPos += numVerts * sizeof( byte_vec4_t );
					}
				}
			}
			for( j = 0; j < MAX_LIGHTMAPS && in->vertexStyles[j] != 255; j++ )
			{
				mesh->colorsArray[j] = ( byte_vec4_t * )( buffer + bufPos ); bufPos += numVerts * sizeof( byte_vec4_t );
				memcpy( mesh->colorsArray[j], loadmodel_colors_array[j] + firstVert, numVerts * sizeof( byte_vec4_t ) );
			}

			bufPos = ALIGN( bufPos, sizeof( elem_t ) );
			mesh->elems = ( elem_t * )( buffer + bufPos ); bufPos += numElems * sizeof( elem_t );
			memcpy( mesh->elems, loadmodel_surfelems + firstElem, numElems * sizeof( elem_t ) );

			R_BuildTangentVectors( mesh->numVerts, mesh->xyzArray, mesh->normalsArray, mesh->stArray, mesh->numElems / 3, mesh->elems, mesh->sVectorsArray );

			if( out->facetype == FACETYPE_PLANAR )
			{
				cplane_t *plane;
				vec3_t v[3];

				// don't trust q3map, recalculate surface plane from the first triangle
				bufPos = ALIGN( bufPos, 16 );
				plane = out->plane = ( cplane_t * )( buffer + bufPos ); bufPos += sizeof( cplane_t );

				// do not trust compiler on surface normal
				for( j = 0; j < 3; j++ ) {
					VectorCopy( mesh->xyzArray[mesh->elems[j]], v[j] );
				}

				PlaneFromPoints( v, plane );
				CategorizePlane( plane );
			}

			if( numFoliageInstances > 0 )
			{
				unsigned int j;
				vec3_t *origins = loadmodel_xyz_array + firstVert, *origin;
				instancePoint_t *instance;

				out->numInstances = numFoliageInstances;
				out->instances = ( instancePoint_t * )( buffer + ALIGN( bufPos, 16 ) );

				for( j = 0; j < out->numInstances; j++ ) {
					// add pseudo random YAW-angle rotation  
					vec3_t angles = { 0, 0, 0 };
					mat3_t rot;

					origin = origins + j;
					instance = out->instances + j;

					angles[YAW] = anglemod( j );
					AnglesToAxis( angles, rot );
					Quat_FromMatrix3( rot, *instance );

					VectorCopy( *origin, &((*instance)[4]) );
					(*instance)[7] = 1.0f;
				}
			}
			break;
		}
	}

	return mesh;
}

/*
* Mod_LoadPatchGroups
*/
static void Mod_LoadPatchGroups( const lump_t *l )
{
	int i, j, count;
	int *out = NULL;
	int *patches = NULL, patchcount;
	int facetype;

	count = loadbmodel->numsurfaces;
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool( out, loadmodel->mempool );
	memset( out, -1, count * sizeof( *out ) );

	patchcount = 0;
	patches = Q_CallocAligned( count, 16, sizeof( *patches ) );
	Q_LinkToPool( patches, loadmodel->mempool );

	for( i = 0; i < count; i++ ) {
		facetype = loadbmodel->surfaces[i].facetype;
		if( facetype != FACETYPE_PATCH ) {
			continue;
		}
		patches[patchcount++] = i;
	}

	loadmodel_numpatchgroups = 0;
	loadmodel_maxpatchgroups = 0;
	loadmodel_patchgroups = NULL;
	loadmodel_patchgrouprefs = out;

	if( !patchcount ) {
		Q_Free( patches );
		return;
	}

	// allocate patch groups to possibly hold all patches individually
	loadmodel_maxpatchgroups = patchcount;
	loadmodel_patchgroups = Q_CallocAligned( loadmodel_maxpatchgroups, 16, sizeof( *loadmodel_patchgroups ) );
	Q_LinkToPool( loadmodel_patchgroups, loadmodel->mempool );

	// assign patches to groups based on LOD bounds
	for( i = 0; i < patchcount; i++ ) {
		j = patches[i];
		out[j] = Mod_AddUpdatePatchGroup( loadmodel_dsurfaces + j );
	}

	Q_Free( patches );

	ri.Com_DPrintf( "Mod_LoadPatchGroups: count (%i), groups(%i)\n", patchcount, loadmodel_numpatchgroups );

#undef Mod_PreloadPatches_PROLOGUE
#undef Mod_PreloadPatches_COUNT
}


/*
* Mod_LoadElems
*/
static void Mod_LoadElems( const lump_t *l )
{
	int i, count;
	int *in;
	elem_t	*out;

	in = ( void * )( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadElems: funny lump size in %s", loadmodel->name );
	count = l->filelen / sizeof( *in );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);

	loadmodel_surfelems = out;
	loadmodel_numsurfelems = count;

	for( i = 0; i < count; i++ )
		out[i] = LittleLong( in[i] );
}


/*
* Mod_LoadLightgrid
*/
static void Mod_LoadLightgrid( const lump_t *l )
{
	int i, j, count;
	dgridlight_t *in;
	mgridlight_t *out;

	in = ( void * )( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadLightgrid: funny lump size in %s", loadmodel->name );
	count = l->filelen / sizeof( *in );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);

	loadbmodel->lightgrid = out;
	loadbmodel->numlightgridelems = count;

	// lightgrid is all 8 bit
	for( i = 0; i < count; i++, in++, out++ )
	{
		out->styles[0] = 0;
		for( j = 1; j < MAX_LIGHTMAPS; j++ )
			out->styles[j] = 255;
		out->direction[0] = in->direction[0];
		out->direction[1] = in->direction[1];
		for( j = 0; j < 3; j++ )
		{
			out->diffuse[0][j] = in->diffuse[j];
			out->ambient[0][j] = in->diffuse[j];
		}
	}
}

/*
* Mod_LoadLightgrid_RBSP
*/
static void Mod_LoadLightgrid_RBSP( const lump_t *l )
{
	int count;
	rdgridlight_t *in;
	mgridlight_t *out;

	in = ( void * )( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadLightgrid: funny lump size in %s", loadmodel->name );
	count = l->filelen / sizeof( *in );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);

	loadbmodel->lightgrid = out;
	loadbmodel->numlightgridelems = count;

	// lightgrid is all 8 bit
	memcpy( out, in, count*sizeof( *out ) );
}


/*
* Mod_ApplySuperStylesToFace
*/
static void Mod_ApplySuperStylesToFace( const rdface_t *in, msurface_t *out )
{
	int j, k;
	float *lmArray;
	uint8_t *lmlayersArray;
	mesh_t *mesh = out->mesh;
	mlightmapRect_t *lmRects[MAX_LIGHTMAPS];
	int lightmaps[MAX_LIGHTMAPS];
	uint8_t lightmapStyles[MAX_LIGHTMAPS], vertexStyles[MAX_LIGHTMAPS];

	for( j = 0; j < MAX_LIGHTMAPS; j++ )
	{
		lightmaps[j] = LittleLong( in->lm_texnum[j] );

		if( in->lightmapStyles[j] == 255 || lightmaps[j] >= loadmodel_numlightmaps || !mesh )
		{
			lmRects[j] = NULL;
			lightmaps[j] = -1;
			lightmapStyles[j] = 255;
		}
		else
		{
			lmRects[j] = &loadmodel_lightmapRects[lightmaps[j]];
			lightmaps[j] = lmRects[j]->texNum;

			// scale/shift lightmap coords
			if( mapConfig.lightmapsPacking )
			{
				lmArray = mesh->lmstArray[j][0];
				for( k = 0; k < mesh->numVerts; k++, lmArray += 2 )
				{
					lmArray[0] = (double)( lmArray[0] ) * lmRects[j]->texMatrix[0][0] + lmRects[j]->texMatrix[0][1];
					lmArray[1] = (double)( lmArray[1] ) * lmRects[j]->texMatrix[1][0] + lmRects[j]->texMatrix[1][1];
				}
			}
			if( mapConfig.lightmapArrays )
			{
				lmlayersArray = &mesh->lmlayersArray[j >> 2][0][j & 3];
				for( k = 0; k < mesh->numVerts; k++, lmlayersArray += 4 )
					*lmlayersArray = lmRects[j]->texLayer;
			}
			lightmapStyles[j] = in->lightmapStyles[j];
		}
		vertexStyles[j] = in->vertexStyles[j];
	}
	out->superLightStyle = R_AddSuperLightStyle( loadmodel, lightmaps, lightmapStyles, vertexStyles, lmRects );
}

/*
* Mod_LoadQ3BrushModel
*/
void Mod_LoadQ3BrushModel( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *format )
{
	int i;
	vec3_t gridSize, ambient, outline;

	mod->type = mod_brush;
	mod->registrationSequence = rsh.registrationSequence;
	if( rsh.worldModel != NULL )
		ri.Com_Error( ERR_DROP, "Loaded a brush model after the world" );

	loadmodel = mod;

	mod_bspFormat = format;

	dheader_t* const header = (dheader_t *)buffer;
	mod_base = (uint8_t *)buffer;

	// swap all the lumps
	for( i = 0; i < sizeof( dheader_t )/4; i++ )
		( (int *)header )[i] = LittleLong( ( (int *)header )[i] );

	lump_t* const model_lumps = &header->lumps[LUMP_MODELS];
	lump_t* const entity_lumps = &header->lumps[LUMP_ENTITIES];
	lump_t* const light_lumps = &header->lumps[LUMP_LIGHTING];
	lump_t* const face_lumps = &header->lumps[LUMP_FACES];
	lump_t* const shaderrefs_lumps = &header->lumps[LUMP_SHADERREFS];
	lump_t* const planes_lumps = &header->lumps[LUMP_PLANES];
	lump_t* const brush_lumps = &header->lumps[LUMP_BRUSHES];
	lump_t* const brushsides_faces_lumps = &header->lumps[LUMP_BRUSHSIDES];
	lump_t* const fog_lumps = &header->lumps[LUMP_FOGS];
	lump_t* const lightarray_lumps = &header->lumps[LUMP_LIGHTARRAY];
	lump_t* const nodes_lumps = &header->lumps[LUMP_NODES];
	lump_t* const leaf_lumps = &header->lumps[LUMP_LEAFS];
	lump_t* const leaffaces_lumps = &header->lumps[LUMP_LEAFFACES];
	
	const size_t lightMapWidth = mod_bspFormat->lightmapWidth;
	const size_t lightMapHeight = mod_bspFormat->lightmapHeight;
	const size_t lightMapSize = lightMapWidth * lightMapHeight * LIGHTMAP_BYTES;
	// we don't need lightmaps for vertex lighting
	const bool hasLightmaps = (!r_lighting_vertexlight->integer) && light_lumps->filelen > 0;
	const bool isBspRavenFormat = mod_bspFormat->flags & BSP_RAVEN;

	if((lightarray_lumps->filelen % sizeof( uint16_t )) ||
		 (leaf_lumps ->filelen % sizeof(dleaf_t)) ||
		 (leaffaces_lumps->filelen % sizeof(int32_t)) ||
		 (nodes_lumps->filelen % sizeof( dnode_t )) ||
		 (fog_lumps->filelen % sizeof( dfog_t )) ||
		 (brush_lumps->filelen % sizeof( dbrush_t )) ||
		 (brushsides_faces_lumps->filelen % ( isBspRavenFormat ? sizeof( rdbrushside_t ) : sizeof( dbrushside_t ) )) ||
		 (hasLightmaps && light_lumps->filelen % lightMapSize) ||
		 (shaderrefs_lumps->filelen % sizeof( dshaderref_t )) ||
		 (shaderrefs_lumps->filelen % sizeof( dshaderref_t )) ||
		 (model_lumps->filelen % sizeof( dmodel_t )) ||
		 (planes_lumps->filelen % sizeof( dplane_t )) ||
		 (face_lumps->filelen % ( isBspRavenFormat ? sizeof( rdface_t ) : sizeof( dface_t ) ))
	) {
		ri.Com_Error( ERR_DROP, "Mod_LoadBrushsides: funny lump size in %s", loadmodel->name );
	}

	dleaf_t *const q3_leafs = (void *)( mod_base + leaf_lumps->fileofs );
	int32_t *const q3_leaffaces = (void *)( mod_base + leaffaces_lumps->fileofs );
	const size_t numleafs = leaf_lumps->filelen / sizeof( dleaf_t );
	const size_t numLeafFaces = leaffaces_lumps->filelen / sizeof(int32_t);

	uint16_t *const q3_lightarrays = (void *)( mod_base + lightarray_lumps->fileofs );
	const size_t numlightArrays = lightarray_lumps->filelen / sizeof( uint16_t);

	dnode_t *const q3_nodes = (void *)( mod_base + nodes_lumps->fileofs );
	const size_t q3_nodes_len = nodes_lumps->filelen / sizeof( dnode_t);
	
	dfog_t *const q3_fogs = (void *)( mod_base + fog_lumps->fileofs );
	const size_t numFogs = fog_lumps->filelen / sizeof( dfog_t );

	dbrush_t *const q3_brushs = (void *)( mod_base + brush_lumps->fileofs );
	const size_t numBrushes = brush_lumps->filelen / sizeof( dbrush_t );

	const size_t numBrushSides = brushsides_faces_lumps->filelen / ( isBspRavenFormat ? sizeof( rdbrushside_t ) : sizeof( dbrushside_t ) );
	dbrushside_t *const q3_brushSides = (void *)( mod_base + brushsides_faces_lumps->fileofs );
	rdbrushside_t *const q3_ravenbrushSides = (void *)( mod_base + brushsides_faces_lumps->fileofs );

	const size_t modelLen = model_lumps->filelen / sizeof( dmodel_t );
	dmodel_t *const q3_model = (void *)( (uint8_t *)buffer + model_lumps->fileofs );

	const size_t numFaces = face_lumps->filelen / ( isBspRavenFormat ? sizeof( rdface_t ) : sizeof( dface_t ) );
	dface_t *const q3_faces = (void *)( mod_base + face_lumps->fileofs );
	rdface_t *const q3_ravenFaces = (void *)( mod_base + face_lumps->fileofs );

	const size_t numPlanes = planes_lumps->filelen / sizeof( dplane_t );
	dplane_t *const q3_planes = (void *)( mod_base + planes_lumps->fileofs );

	{

		uint8_t *const sub_model_mem = Q_CallocAligned( modelLen, 16, sizeof( mbrushmodel_t ) + sizeof( model_t ) );
		mmodel_t *const models = Q_CallocAligned( modelLen, 16, sizeof( mmodel_t ) );

		Q_LinkToPool( models, loadmodel->mempool );
		Q_LinkToPool( sub_model_mem, loadmodel->mempool );

		model_t *const inline_models = (model_t *)sub_model_mem;
		mbrushmodel_t *const brush_models = (mbrushmodel_t *)( sub_model_mem + ( sizeof( model_t ) * modelLen ) );

		loadmodel->extradata = brush_models;

		loadbmodel = brush_models;
		loadbmodel->submodels = models;
		loadbmodel->numsubmodels = modelLen;
		loadbmodel->inlines = inline_models;

		for( size_t i = 0; i < modelLen; i++ ) {
			inline_models[i].extradata = &brush_models[i];

			for( size_t j = 0; j < 3; j++ ) {
				// spread the mins / maxs by a pixel
				models[i].mins[j] = LittleFloat( q3_model[i].mins[j] ) - 1;
				models[i].maxs[j] = LittleFloat( q3_model[i].maxs[j] ) + 1;
			}

			models[i].radius = RadiusFromBounds( models[i].mins, models[i].maxs );
			models[i].firstface = LittleLong( q3_model[i].firstface );
			models[i].numfaces = LittleLong( q3_model[i].numfaces );
		}
	}

	{
		int n;
		bool isworld;
		float gridsizef[3] = { 0, 0, 0 }, colorf[3] = { 0, 0, 0 }, ambientf = 0;
		char key[MAX_KEY], value[MAX_VALUE], *token;
		float celcolorf[3] = { 0, 0, 0 };

		assert( gridSize );
		assert( ambient );
		assert( outline );

		VectorClear( gridSize );
		VectorClear( ambient );
		VectorClear( outline );

		char *data = (char *)mod_base + entity_lumps->fileofs;
		if( data[0] ) {
			for( ; ( token = COM_Parse( &data ) ) && token[0] == '{'; ) {
				isworld = false;

				while( 1 ) {
					token = COM_Parse( &data );
					if( !token[0] )
						break; // error
					if( token[0] == '}' )
						break; // end of entity

					Q_strncpyz( key, token, sizeof( key ) );
					Q_trim( key );

					token = COM_Parse( &data );
					if( !token[0] )
						break; // error

					Q_strncpyz( value, token, sizeof( value ) );

					// now that we have the key pair worked out...
					if( !strcmp( key, "classname" ) ) {
						if( !strcmp( value, "worldspawn" ) )
							isworld = true;
					} else if( !strcmp( key, "gridsize" ) ) {
						int gridsizei[3] = { 0, 0, 0 };
						sscanf( value, "%4i %4i %4i", &gridsizei[0], &gridsizei[1], &gridsizei[2] );
						VectorCopy( gridsizei, gridsizef );
					} else if( !strcmp( key, "_ambient" ) || ( !strcmp( key, "ambient" ) && ambientf == 0.0f ) ) {
						n = sscanf( value, "%8f", &ambientf );
						if( n != 1 ) {
							int ia = 0;
							n = sscanf( value, "%3i", &ia );
							ambientf = ia;
						}
					} else if( !strcmp( key, "_color" ) ) {
						n = sscanf( value, "%8f %8f %8f", &colorf[0], &colorf[1], &colorf[2] );
						if( n != 3 ) {
							int colori[3] = { 0, 0, 0 };
							sscanf( value, "%3i %3i %3i", &colori[0], &colori[1], &colori[2] );
							VectorCopy( colori, colorf );
						}
					} else if( !strcmp( key, "_forceclear" ) ) {
						if( atof( value ) != 0 )
							mapConfig.forceClear = true;
					} else if( !strcmp( key, "_lightingIntensity" ) ) {
						if( !r_fullbright->integer ) {
							// non power of two intensity scale for lighting
							sscanf( value, "%8f", &mapConfig.lightingIntensity );
						}
					} else if( !strcmp( key, "_outlinecolor" ) ) {
						n = sscanf( value, "%8f %8f %8f", &celcolorf[0], &celcolorf[1], &celcolorf[2] );
						if( n != 3 ) {
							int celcolori[3] = { 0, 0, 0 };
							sscanf( value, "%3i %3i %3i", &celcolori[0], &celcolori[1], &celcolori[2] );
							VectorCopy( celcolori, celcolorf );
						}
					}
				}

				if( isworld ) {
					VectorCopy( gridsizef, gridSize );

					if( VectorCompare( colorf, vec3_origin ) )
						VectorSet( colorf, 1.0, 1.0, 1.0 );
					VectorScale( colorf, ambientf, ambient );

					if( max( celcolorf[0], max( celcolorf[1], celcolorf[2] ) ) > 1.0f )
						VectorScale( celcolorf, 1.0f / 255.0f, celcolorf ); // [0..1] RGB -> [0..255] RGB
					VectorCopy( celcolorf, outline );
					break;
				}
			}
		}
	}
	{
		R_InitLightStyles( loadmodel );
		// set overbright bits for lightmaps and lightgrid
		// deluxemapped maps have zero scale because most surfaces
		// have a gloss stage that makes them look brighter anyway
		if( mapConfig.lightingIntensity )
		{
			mapConfig.overbrightBits -= atoi( r_mapoverbrightbits->dvalue );
			if( mapConfig.overbrightBits < 0 )
				mapConfig.overbrightBits = 0;
			mapConfig.pow2MapOvrbr = max( mapConfig.overbrightBits, 0 );
			mapConfig.mapLightColorScale = ( 1 << mapConfig.pow2MapOvrbr ) * mapConfig.lightingIntensity;
		}
		else
		{
			// for maps that do not specify lighting intensity, default intensity to 2
			// and reduce overbright bits according
			// this allows for more dramatic shadows while staying faithful to author's original intention
			mapConfig.pow2MapOvrbr = mapConfig.overbrightBits - 1;
			if( mapConfig.pow2MapOvrbr < 0 )
				mapConfig.pow2MapOvrbr = 0;
			mapConfig.lightingIntensity = (float)(1 << max( mapConfig.overbrightBits - mapConfig.pow2MapOvrbr, 0 ));
			mapConfig.overbrightBits = 0;
			mapConfig.mapLightColorScale = mapConfig.lightingIntensity;
		}

		if(hasLightmaps) {
			loadmodel_numlightmaps = light_lumps->filelen / lightMapSize;
			loadmodel_lightmapRects = Q_CallocAligned(  loadmodel_numlightmaps, 16, sizeof( *loadmodel_lightmapRects ) );
			Q_LinkToPool(loadmodel_lightmapRects, loadmodel->mempool);

			uint8_t *const lights = mod_base + light_lumps->fileofs;

			// there are no deluxemaps in the map if the number of lightmaps is
			// less than 2 or odd
			if( loadmodel_numlightmaps < 2 || loadmodel_numlightmaps & 1 )
				goto skip_delux_lightmaps;

			if( format->flags & BSP_RAVEN ) {
				for( size_t i = 0; i < numFaces; i++ ) {
					for( size_t j = 0; j < MAX_LIGHTMAPS; j++ ) {
						const int lightmap = LittleLong( q3_ravenFaces[i].lm_texnum[j] );
						if( lightmap <= 0 )
							continue;
						if( lightmap & 1 )
							goto skip_delux_lightmaps;
					}
				}
			} else {
				for( i = 0; i < numFaces; i++ ) {
					int lightmap = LittleLong( q3_faces[i].lm_texnum );
					if( lightmap <= 0 )
						continue;
					if( lightmap & 1 )
						goto skip_delux_lightmaps;
				}
			}
			// check if the deluxemap is actually empty (q3map2, yay!)
			if( loadmodel_numlightmaps == 2 ) {
				int lW = mod_bspFormat->lightmapWidth, lH = mod_bspFormat->lightmapHeight;
				size_t i,j;
				uint8_t* lc = lights;
				lc += lW * lH * LIGHTMAP_BYTES;
				for( i = lW * lH; i > 0; i--, lc+= LIGHTMAP_BYTES ) {
					for( j = 0; j < LIGHTMAP_BYTES; j++ ) {
						if( lc[j] )
							break;
					}
					if( j != LIGHTMAP_BYTES )
						break;
				}

				// empty deluxemap
				if( !i ) {
					loadmodel_numlightmaps = 1;
					goto skip_delux_lightmaps;
				}
			}

			mapConfig.deluxeMaps = true;
			mapConfig.deluxeMappingEnabled = r_lighting_deluxemapping->integer ? true : false;

		skip_delux_lightmaps:
			R_BuildLightmaps( loadmodel, loadmodel_numlightmaps, mod_bspFormat->lightmapWidth, mod_bspFormat->lightmapHeight, mod_base + light_lumps->fileofs, loadmodel_lightmapRects );
		}
	}

	{
		dshaderref_t *const in = (void *)( mod_base + shaderrefs_lumps->fileofs );
		const size_t count = shaderrefs_lumps->filelen / sizeof( dshaderref_t );

		mshaderref_t *out = Q_CallocAligned( count, 16, sizeof( mshaderref_t ) );
		Q_LinkToPool( out, loadmodel->mempool );

		loadmodel_shaderrefs = out;
		loadmodel_numshaderrefs = count;

		// see if the map is new and we need to free shaders from the previous one
		const bool isNewMap = r_prevworldmodel && ( r_prevworldmodel->registrationSequence != rsh.registrationSequence );

		for( i = 0; i < count; i++ ) {
			Q_strncpyz( out[i].name, in[i].name, sizeof( out[i].name ) );
			out[i].flags = LittleLong( in[i].flags );

			if( isNewMap ) {
				R_TouchShadersByName( out[i].name );
			}
		}

		// free world textures from the previous map that are not used on the new map
		if( isNewMap ) {
			const shaderType_e shaderTypes[] = { SHADER_TYPE_DELUXEMAP, SHADER_TYPE_VERTEX };
			R_FreeUnusedShadersByType( shaderTypes, sizeof( shaderTypes ) / sizeof( shaderTypes[0] ) );
			R_FreeUnusedImagesByTags( IMAGE_TAG_WORLD );
		}
	}

	// load into heap
	Mod_PreloadFaces( &header->lumps[LUMP_FACES] );
  {
  	cplane_t* planes = Q_CallocAligned( numPlanes, 16, sizeof(cplane_t) );
  	Q_LinkToPool(planes, loadmodel->mempool);

  	loadbmodel->planes = planes;
  	loadbmodel->numplanes = numPlanes;

  	for(size_t i = 0; i < numPlanes; i++)
  	{
  		planes[i].type = PLANE_NONAXIAL;
  		planes[i].signbits = 0;

  		for(size_t j = 0; j < 3; j++ )
  		{
  			planes[i].normal[j] = LittleFloat( q3_planes[i].normal[j] );
  			if( planes[i].normal[j] < 0 )
  				planes[i].signbits |= 1<<j;
  			if( planes[i].normal[j] == 1.0f )
  				planes[i].type = j;
  		}
  		planes[i].dist = LittleFloat( q3_planes[i].dist );
  	}
  }
	{
		mfog_t* fogs = Q_CallocAligned( 16, numFogs, sizeof( mfog_t) );
		Q_LinkToPool( fogs, loadmodel->mempool );

		loadbmodel->fogs = fogs;
		loadbmodel->numfogs = numFogs;

		for(size_t fogIdx = 0; fogIdx < numFogs; fogIdx++) {
			fogs->shader = R_RegisterShader( q3_fogs[fogIdx].shader, SHADER_TYPE_2D );
			const int p = LittleLong( q3_fogs[fogIdx].brushnum );
			if( p == -1 )
				continue;
			const dbrush_t *brush = &q3_brushs[p];

			const int numSides = LittleLong( brush->numsides );
			if( numSides < 6 ) {
				fogs->shader = NULL;
				ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: missing fog brush sides\n" );
				continue;
			}

			const int firstSide = LittleLong( brush->firstside );
			if( firstSide == -1 ) {
				fogs->shader = NULL;
				ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: bad fog brush side\n" );
				continue;
			}
			int brushPlanes[6] = {0};
			const int visibileSideIdx = LittleLong( q3_fogs[fogIdx].visibleside);
			if( isBspRavenFormat ) {
				if( visibileSideIdx != -1 )
					fogs[fogIdx].visibleplane = loadbmodel->planes + LittleLong( q3_ravenbrushSides[visibileSideIdx].planenum);
				for(size_t j = 0; j < 6; j++ )
					brushPlanes[j] = LittleLong( q3_ravenbrushSides[j].planenum );
			} else {
				if( visibileSideIdx != -1 )
					fogs[fogIdx].visibleplane = loadbmodel->planes + LittleLong( q3_brushSides[visibileSideIdx].planenum);
				for(size_t j = 0; j < 6; j++ )
					brushPlanes[j] = LittleLong( q3_brushSides[j].planenum );
			}

			VectorSet( 
				fogs[fogIdx].mins, 
				-loadbmodel->planes[brushPlanes[0]].dist, 
				-loadbmodel->planes[brushPlanes[2]].dist, 
				-loadbmodel->planes[brushPlanes[4]].dist );
			VectorSet( fogs[fogIdx].maxs, 
					loadbmodel->planes[brushPlanes[1]].dist, 
					loadbmodel->planes[brushPlanes[3]].dist, 
					loadbmodel->planes[brushPlanes[5]].dist );
		}
	}
	Mod_LoadFaces( &header->lumps[LUMP_FACES] );
	if( mod_bspFormat->flags & BSP_RAVEN )
		Mod_LoadVertexes_RBSP( &header->lumps[LUMP_VERTEXES] );
	else
		Mod_LoadVertexes( &header->lumps[LUMP_VERTEXES] );
	Mod_LoadElems( &header->lumps[LUMP_ELEMENTS] );
	if( mod_bspFormat->flags & BSP_RAVEN )
		Mod_LoadLightgrid_RBSP( &header->lumps[LUMP_LIGHTGRID] );
	else
		Mod_LoadLightgrid( &header->lumps[LUMP_LIGHTGRID] );
	Mod_LoadPatchGroups( &header->lumps[LUMP_FACES] );
	{
		mleaf_t *const mod_leafs = Q_CallocAligned( numleafs, 16, sizeof( mleaf_t ) );
		Q_LinkToPool( mod_leafs, loadmodel->mempool );

		loadbmodel->leafs = mod_leafs;
		loadbmodel->numleafs = numleafs;

		for(size_t i = 0; i < numleafs; i++) {
			bool badBounds = false;
			for(size_t j = 0; j < 3; j++ ) {
				mod_leafs[i].mins[j] = (float)LittleLong( q3_leafs[i].mins[j] );
				mod_leafs[i].maxs[j] = (float)LittleLong( q3_leafs[i].maxs[j] );
				if( mod_leafs[i].mins[j] > mod_leafs[i].maxs[j] )
					badBounds = true;
			}
			mod_leafs[i].cluster = LittleLong( q3_leafs[i].cluster );

			if( i && ( badBounds || VectorCompare( mod_leafs[i].mins, mod_leafs[i].maxs ) ) && mod_leafs[i].cluster >= 0 ) {
				ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: bad leaf bounds\n" );
				mod_leafs[i].cluster = -1;
			}

			if( loadbmodel->pvs && ( mod_leafs[i].cluster >= loadbmodel->pvs->numclusters ) ) {
				Com_Printf( S_COLOR_YELLOW "WARNING: leaf cluster > numclusters" );
				mod_leafs[i].cluster = -1;
			}

			mod_leafs[i].plane = NULL;
			mod_leafs[i].area = LittleLong( q3_leafs[i].area );
			if( mod_leafs[i].area >= loadbmodel->numareas )
				loadbmodel->numareas = mod_leafs[i].area + 1;

			int numVisSurfaces = 0;
			int numFragmentSurfaces = 0;
			int numMarkSurfaces = LittleLong( q3_leafs[i].numleaffaces );
			if( !numMarkSurfaces ) {
				// out->cluster = -1;
				continue;
			}

			int firstMarkSurface = LittleLong( q3_leafs[i].firstleafface );
			if( firstMarkSurface < 0 || numMarkSurfaces + firstMarkSurface > numLeafFaces)
				ri.Com_Error( ERR_DROP, "MOD_LoadBmodel: bad marksurfaces in leaf %i", i );

			numVisSurfaces = numMarkSurfaces;
			numFragmentSurfaces = numMarkSurfaces;

			uint8_t* buffer = (uint8_t *)Q_CallocAligned( ( ( numVisSurfaces + 1 ) + ( numFragmentSurfaces + 1 ) ), 16, sizeof( msurface_t * ) );
			Q_LinkToPool( buffer, loadmodel->mempool );

			mod_leafs[i].firstVisSurface = (msurface_t **)buffer;
			buffer += ( numVisSurfaces + 1 ) * sizeof( msurface_t * );

			mod_leafs[i].firstFragmentSurface = (msurface_t **)buffer;
			buffer += ( numFragmentSurfaces + 1 ) * sizeof( msurface_t * );

			numVisSurfaces = numFragmentSurfaces = 0;
			for(size_t j = 0; j < numMarkSurfaces; j++ ) {
				int k = LittleLong( q3_leaffaces[firstMarkSurface + j] );
				mod_leafs[i].firstVisSurface[numVisSurfaces++] = loadbmodel->surfaces + k;
				mod_leafs[i].firstFragmentSurface[numFragmentSurfaces++] = loadbmodel->surfaces + k;
			}
		}
	}

	{
		mnode_t *const mnodes = Q_CallocAligned( q3_nodes_len, 16, sizeof( *mnodes ) );
		Q_LinkToPool( mnodes, loadmodel->mempool );

		loadbmodel->nodes = mnodes;
		loadbmodel->numnodes = q3_nodes_len;

		for( size_t i = 0; i < q3_nodes_len; i++ ) {
			mnodes[i].plane = loadbmodel->planes + LittleLong( q3_nodes[i].planenum );

			for( size_t j = 0; j < 2; j++ ) {
				const int childOffset = LittleLong( q3_nodes[i].children[j] );
				if( childOffset >= 0 )
					mnodes[i].children[j] = loadbmodel->nodes + childOffset;
				else
					mnodes[i].children[j] = (mnode_t *)( loadbmodel->leafs + ( -1 - childOffset ) );
			}

			bool badBounds = false;
			for( size_t j = 0; j < 3; j++ ) {
				mnodes[i].mins[j] = (float)LittleLong( q3_nodes[i].mins[j] );
				mnodes[i].maxs[j] = (float)LittleLong( q3_nodes[i].maxs[j] );
				if( mnodes[i].mins[j] > mnodes[i].maxs[j] )
					badBounds = true;
			}

			if( badBounds || VectorCompare(  mnodes[i].mins, mnodes[i].maxs ) ) {
				ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: bad node %i bounds:\n", i );
				ri.Com_DPrintf( S_COLOR_YELLOW "mins: %i %i %i\n", Q_rint( mnodes[i].mins[0] ), Q_rint( mnodes[i].mins[1] ), Q_rint( mnodes[i].mins[2] ) );
				ri.Com_DPrintf( S_COLOR_YELLOW "maxs: %i %i %i\n", Q_rint( mnodes[i].maxs[0] ), Q_rint( mnodes[i].maxs[1] ), Q_rint( mnodes[i].maxs[2] ) );
			}
		}
	}

	if( isBspRavenFormat ) {
		mgridlight_t **const out = Q_CallocAligned( numlightArrays, 16, sizeof( mgridlight_t ) );
		Q_LinkToPool( out, loadmodel->mempool );

		loadbmodel->lightarray = out;
		loadbmodel->numlightarrayelems = numlightArrays;

		for( i = 0; i < numlightArrays; i++ ) {
			uint16_t index = LittleShort( q3_lightarrays[i] );
			if( index >= (unsigned)loadbmodel->numlightgridelems ) {
				ri.Com_Error( ERR_DROP, "Mod_LoadLightArray_RBSP: funny grid index(%i):%i in %s", i, index, loadmodel->name );
			}
			out[i] = loadbmodel->lightgrid + index;
		}
	} else {
		mgridlight_t **const out = Q_CallocAligned( loadbmodel->numlightgridelems, 16, sizeof( mgridlight_t ) );
		Q_LinkToPool( out, loadmodel->mempool );

		loadbmodel->lightarray = out;
		loadbmodel->numlightarrayelems = loadbmodel->numlightgridelems;

		for( size_t i = 0; i < loadbmodel->numlightarrayelems; i++ )
			out[i] = loadbmodel->lightgrid + i;
	}
	{
		// remembe the BSP format just in case
		loadbmodel->format = mod_bspFormat;

		// set up lightgrid
		if( gridSize[0] < 1 || gridSize[1] < 1 || gridSize[2] < 1 )
			VectorSet( loadbmodel->gridSize, 64, 64, 128 );
		else
			VectorCopy( gridSize, loadbmodel->gridSize );

		for(size_t j = 0; j < 3; j++ ) {
			vec3_t maxs;

			loadbmodel->gridMins[j] = loadbmodel->gridSize[j] * ceil( ( loadbmodel->submodels[0].mins[j] + 1 ) / loadbmodel->gridSize[j] );
			maxs[j] = loadbmodel->gridSize[j] * floor( ( loadbmodel->submodels[0].maxs[j] - 1 ) / loadbmodel->gridSize[j] );
			loadbmodel->gridBounds[j] = ( maxs[j] - loadbmodel->gridMins[j] ) / loadbmodel->gridSize[j];
			loadbmodel->gridBounds[j] = max( loadbmodel->gridBounds[j], 0 ) + 1;
		}
		loadbmodel->gridBounds[3] = loadbmodel->gridBounds[1] * loadbmodel->gridBounds[0];

		// ambient lighting
		for( i = 0; i < 3; i++ )
			mapConfig.ambient[i] = ambient[i];

		// outline color
		for( i = 0; i < 3; i++ )
			mapConfig.outlineColor[i] = (uint8_t)( bound( 0, outline[i] * 255.0f, 255 ) );
		mapConfig.outlineColor[3] = 255;
		
		mfog_t *testFog;
		for( i = 0, testFog = loadbmodel->fogs; i < loadbmodel->numfogs; testFog++, i++ ) {
			if( !testFog->shader )
				continue;
			if( testFog->visibleplane )
				continue;

			testFog->visibleplane = Q_Malloc( sizeof( cplane_t ) );
			memset( testFog->visibleplane, 0, sizeof( cplane_t ) );
			Q_LinkToPool( testFog->visibleplane, loadmodel->mempool );
			VectorSet( testFog->visibleplane->normal, 0, 0, 1 );
			testFog->visibleplane->type = PLANE_Z;
			testFog->visibleplane->dist = loadbmodel->submodels[0].maxs[0] + 1;
		}

		// make sure that the only fog in the map has valid shader
		bool globalFog = ( loadbmodel->numfogs == 1 ) ? true : false;
		if( globalFog ) {
			testFog = &loadbmodel->fogs[0];
			if( !testFog->shader )
				globalFog = false;
		}

		R_SortSuperLightStyles( loadmodel );

		rdface_t *in = loadmodel_dsurfaces;
		msurface_t *surf = loadbmodel->surfaces;
		for( i = 0; i < loadbmodel->numsurfaces; i++, in++, surf++ ) {
			surf->mesh = Mod_CreateMeshForSurface( in, surf, loadmodel_patchgrouprefs[i] );
			if( surf->mesh ) {
				surf->numVerts = surf->mesh->numVerts;
				surf->numElems = surf->mesh->numElems;
			}

			Mod_ApplySuperStylesToFace( in, surf );

			// force outlines hack for old maps
			if( !mapConfig.forceWorldOutlines && surf->shader && ( surf->shader->flags & SHADER_FORCE_OUTLINE_WORLD ) ) {
				mapConfig.forceWorldOutlines = true;
			}

			if( globalFog && surf->mesh && surf->fog != testFog ) {
				if( !( surf->shader->flags & SHADER_SKY ) && !surf->shader->fog_dist )
					globalFog = false;
			}
		}

		if( globalFog ) {
			loadbmodel->globalfog = testFog;
			ri.Com_DPrintf( "Global fog detected: %s\n", testFog->shader->name );
		}

		if( !( mod_bspFormat->flags & BSP_RAVEN ) ) {
			Q_Free( loadmodel_dsurfaces );
		}
		loadmodel_dsurfaces = NULL;
		loadmodel_numsurfaces = 0;

		Q_Free( loadmodel_xyz_array );
		loadmodel_xyz_array = NULL;
		loadmodel_numverts = 0;

		Q_Free( loadmodel_surfelems );
		loadmodel_surfelems = NULL;
		loadmodel_numsurfelems = 0;

		Q_Free( loadmodel_lightmapRects );
		loadmodel_lightmapRects = NULL;
		loadmodel_numlightmaps = 0;

		Q_Free( loadmodel_shaderrefs );
		loadmodel_shaderrefs = NULL;
		loadmodel_numshaderrefs = 0;

		Q_Free( loadmodel_patchgrouprefs );
		loadmodel_patchgrouprefs = NULL;

		Q_Free( loadmodel_patchgroups );
		loadmodel_patchgroups = NULL;
		loadmodel_numpatchgroups = loadmodel_maxpatchgroups = 0;
	}
}
