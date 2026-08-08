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

// per-axis cap on the light grid. gridBounds[3] is gridBounds[0]*gridBounds[1],
// so this has to stay well under sqrt(INT_MAX)
#define MAX_LIGHTGRID_BOUNDS	8192

static uint8_t *mod_base;
static size_t mod_bufferLen;
static mbrushmodel_t *loadbmodel;

/*
* Mod_LumpCount
*
* Validates a lump against the size of the file it came from and returns the
* number of elemSize-sized records it holds. BSP files are attacker-controlled
* (a client downloads the map from whatever server it joins), so every lump
* offset/length pair has to be range-checked before it is turned into a pointer.
*/
static int Mod_LumpCount( const lump_t *l, size_t elemSize, const char *lumpName )
{
	if( elemSize == 0 )
		ri.Com_Error( ERR_DROP, "Mod_LumpCount: zero-sized %s record in %s", lumpName, loadmodel->name );

	if( l->fileofs < 0 || l->filelen < 0 )
		ri.Com_Error( ERR_DROP, "Mod_LumpCount: negative %s lump (ofs %i, len %i) in %s",
			lumpName, l->fileofs, l->filelen, loadmodel->name );

	// written as a subtraction so that fileofs + filelen cannot itself overflow
	if( (size_t)l->fileofs > mod_bufferLen || (size_t)l->filelen > mod_bufferLen - (size_t)l->fileofs )
		ri.Com_Error( ERR_DROP, "Mod_LumpCount: %s lump out of bounds (ofs %i, len %i, file %u) in %s",
			lumpName, l->fileofs, l->filelen, (unsigned)mod_bufferLen, loadmodel->name );

	// every record in these lumps is built from 4-byte ints and floats, and the
	// loader reads them by casting mod_base+fileofs straight to a struct
	// pointer. q3map2 always aligns its lumps; a hostile file need not, and an
	// unaligned cast is undefined behaviour (and a fault on strict-alignment
	// targets).
	if( elemSize > 1 )
	{
		size_t align = elemSize >= 4 ? 4 : elemSize;
		if( (size_t)l->fileofs & ( align - 1 ) )
			ri.Com_Error( ERR_DROP, "Mod_LumpCount: %s lump offset %i is misaligned in %s",
				lumpName, l->fileofs, loadmodel->name );
	}

	if( l->filelen % elemSize )
		ri.Com_Error( ERR_DROP, "Mod_LumpCount: funny %s lump size in %s", lumpName, loadmodel->name );

	return l->filelen / (int)elemSize;
}

/*
* Mod_LumpCountMax
*
* As Mod_LumpCount, but also rejects counts beyond the format's hard limit so a
* tiny file cannot ask for a huge allocation.
*/
static int Mod_LumpCountMax( const lump_t *l, size_t elemSize, const char *lumpName, int maxCount )
{
	int count = Mod_LumpCount( l, elemSize, lumpName );
	if( count > maxCount )
		ri.Com_Error( ERR_DROP, "Mod_LumpCount: too many %s (%i > %i) in %s",
			lumpName, count, maxCount, loadmodel->name );
	return count;
}

/*
* Mod_LumpData
*
* Base pointer of a lump that has already been validated by Mod_LumpCount.
*/
static inline void *Mod_LumpData( const lump_t *l )
{
	return mod_base + l->fileofs;
}

/*
* Mod_IsFiniteFloat
*
* Rejects NaN and +-infinity. Written against the bit pattern rather than
* isfinite() because release builds are compiled with -ffast-math, which lets
* the compiler assume no NaNs exist and fold the usual predicates away.
*
* Non-finite vertex coordinates are not merely ugly: Patch_FlatnessTest
* recurses until a segment is flat enough, and every comparison against a NaN
* is false, so a single NaN control point recurses until the stack runs out.
*/
static inline bool Mod_IsFiniteFloat( float f )
{
	uint32_t u;
	memcpy( &u, &f, sizeof( u ) );
	return ( u & 0x7f800000u ) != 0x7f800000u;
}

/*
* Mod_CheckFloat
*/
static inline float Mod_CheckFloat( float f, const char *what )
{
	if( !Mod_IsFiniteFloat( f ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadQ3BrushModel: non-finite %s in %s", what, loadmodel->name );
	return f;
}

/*
* Mod_ColorToByte
*
* Converts a normalized colour component to 8 bits. The scale it comes through
* is derived from the entity string ("_lightingIntensity") and the overbright
* cvars, so the value reaching here is not guaranteed to be in [0,1] - and
* casting a float outside unsigned char range is undefined.
*/
static inline uint8_t Mod_ColorToByte( float c )
{
	// written so that NaN falls through to 0 rather than to 255
	if( !( c > 0.0f ) )
		return 0;
	if( c >= 1.0f )
		return 255;
	return (uint8_t)( c * 255.0f );
}

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
		rdface_t *in;

		surfaces = Mod_LumpCountMax( l, sizeof( *in ), "faces", MAX_MAP_FACES );
		in = Mod_LumpData( l );
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
		dface_t	*in;

		surfaces = Mod_LumpCountMax( l, sizeof( *in ), "faces", MAX_MAP_FACES );
		in = Mod_LumpData( l );
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
		int lmSize = mapConfig.lightmapImageSize ? mapConfig.lightmapImageSize : mod_bspFormat->lightmapWidth;
		int lW = lmSize, lH = lmSize;

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
* Mod_LoadLighting
*/
static void Mod_LoadLighting( const lump_t *l, const lump_t *faces )
{
	int size;

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

	// we don't need lightmaps for vertex lighting
	if( r_lighting_vertexlight->integer )
		return;

	if( !l->filelen )
		return;

	int lmSize = mapConfig.lightmapImageSize ? mapConfig.lightmapImageSize : mod_bspFormat->lightmapWidth;

	if( lmSize > r_lighting_maxlmblocksize->integer )
		ri.Com_Error( ERR_DROP, "Map lightmap size %i exceeds \"%s\" (%i). "
			"Increase the cvar to at least %i to load this map",
			lmSize, r_lighting_maxlmblocksize->name,
			r_lighting_maxlmblocksize->integer, lmSize );

	size = lmSize * lmSize * LIGHTMAP_BYTES;

	// bounds the lump against the file and guarantees filelen == numlightmaps * size,
	// which is what R_BuildLightmaps and Mod_CheckDeluxemaps then read
	loadmodel_numlightmaps = Mod_LumpCount( l, size, "lighting" );
	loadmodel_lightmapRects = Q_CallocAligned(  loadmodel_numlightmaps, 16, sizeof( *loadmodel_lightmapRects ) );
	Q_LinkToPool(loadmodel_lightmapRects, loadmodel->mempool);

	Mod_CheckDeluxemaps( faces, Mod_LumpData( l ) );
	R_BuildLightmaps( loadmodel, loadmodel_numlightmaps, lmSize, lmSize, Mod_LumpData( l ), loadmodel_lightmapRects );
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
		loadmodel_numsurfaces = Mod_LumpCountMax( l, sizeof( *in ), "faces", MAX_MAP_FACES );
		in = Mod_LumpData( l );
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

		loadmodel_numsurfaces = Mod_LumpCountMax( l, sizeof( *din ), "faces", MAX_MAP_FACES );
		din = Mod_LumpData( l );
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

			if( in->lightmapStyles[j] == 255 || lightmaps[j] < 0 || lightmaps[j] >= loadmodel_numlightmaps )
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

		// load shader. Mod_PreloadFaces has already range-checked this, but it
		// is cheap to not depend on the two staying in that order
		{
			int shaderNum = LittleLong( in->shadernum );

			if( shaderNum < 0 || shaderNum >= loadmodel_numshaderrefs )
				ri.Com_Error( ERR_DROP, "Mod_LoadFaces: face %i has bad shader %i in %s", i, shaderNum, loadmodel->name );
			shaderRef = loadmodel_shaderrefs + shaderNum;
		}
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

	count = Mod_LumpCountMax( l, sizeof( *in ), "vertexes", MAX_MAP_VERTEXES );
	in = Mod_LumpData( l );

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
			out_xyz[j] = Mod_CheckFloat( LittleFloat( in->point[j] ), "vertex position" );
			out_normals[j] = Mod_CheckFloat( LittleFloat( in->normal[j] ), "vertex normal" );
		}

		for( j = 0; j < 2; j++ )
		{
			// texcoords only ever flow into float math, so a NaN there is ugly
			// but harmless; positions are what drive Patch_FlatnessTest
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

			out_colors[0] = Mod_ColorToByte( color[0] );
			out_colors[1] = Mod_ColorToByte( color[1] );
			out_colors[2] = Mod_ColorToByte( color[2] );
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

	count = Mod_LumpCountMax( l, sizeof( *in ), "vertexes", MAX_MAP_VERTEXES );
	in = Mod_LumpData( l );

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
			out_xyz[j] = Mod_CheckFloat( LittleFloat( in->point[j] ), "vertex position" );
			out_normals[j] = Mod_CheckFloat( LittleFloat( in->normal[j] ), "vertex normal" );
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

				out_colors[j][0] = Mod_ColorToByte( color[0] );
				out_colors[j][1] = Mod_ColorToByte( color[1] );
				out_colors[j][2] = Mod_ColorToByte( color[2] );
				out_colors[j][3] = in->color[j][3];
			}
		}
	}
}

/*
* Mod_LoadSubmodels
*/
static void Mod_LoadSubmodels( const lump_t *l )
{
	int i, j, count;
	dmodel_t *in;
	mmodel_t *out;
	mbrushmodel_t *bmodel;
	model_t *mod_inline;

	count = Mod_LumpCountMax( l, sizeof( *in ), "submodels", MAX_MAP_SUBMODELS );
	if( count < 1 )
		ri.Com_Error( ERR_DROP, "Mod_LoadSubmodels: map with no models in %s", loadmodel->name );
	in = Mod_LumpData( l );
	out = Q_CallocAligned(count, 16, sizeof( *out ));
	Q_LinkToPool(out, loadmodel->mempool);

	mod_inline = Q_CallocAligned(count, 16, sizeof( *mod_inline )+sizeof( *bmodel ));
	Q_LinkToPool(mod_inline, loadmodel->mempool);

	loadmodel->extradata = bmodel = ( mbrushmodel_t * )( ( uint8_t * )mod_inline + count*sizeof( *mod_inline ) );

	loadbmodel = bmodel;
	loadbmodel->submodels = out;
	loadbmodel->numsubmodels = count;
	loadbmodel->inlines = mod_inline;

	for( i = 0; i < count; i++, in++, out++ )
	{
		mod_inline[i].extradata = bmodel + i;

		for( j = 0; j < 3; j++ )
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = Mod_CheckFloat( LittleFloat( in->mins[j] ), "submodel bounds" ) - 1;
			out->maxs[j] = Mod_CheckFloat( LittleFloat( in->maxs[j] ), "submodel bounds" ) + 1;
		}

		out->radius = RadiusFromBounds( out->mins, out->maxs );
		out->firstface = LittleLong( in->firstface );
		out->numfaces = LittleLong( in->numfaces );
	}
}

/*
* Mod_LoadShaderrefs
*/
static void Mod_LoadShaderrefs( const lump_t *l )
{
	int i, count;
	dshaderref_t *in;
	mshaderref_t *out;
	bool newMap;

	count = Mod_LumpCountMax( l, sizeof( *in ), "shaderrefs", MAX_MAP_FACES );
	in = Mod_LumpData( l );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);
	
	loadmodel_shaderrefs = out;
	loadmodel_numshaderrefs = count;

	// see if the map is new and we need to free shaders from the previous one
	newMap = r_prevworldmodel && ( r_prevworldmodel->registrationSequence != rsh.registrationSequence );

	for( i = 0; i < count; i++, in++ )
	{
		Q_strncpyz( out[i].name, in->name, sizeof( out[i].name ) );
		out[i].flags = LittleLong( in->flags );
		
		if( newMap ) {
			R_TouchShadersByName( out[i].name );
		}
	}

	// free world textures from the previous map that are not used on the new map
	if( newMap ) {
		const shaderType_e shaderTypes[] = { SHADER_TYPE_DELUXEMAP, SHADER_TYPE_VERTEX };
		R_FreeUnusedShadersByType( shaderTypes, sizeof( shaderTypes ) / sizeof( shaderTypes[0] ) );
		R_FreeUnusedImagesByTags( IMAGE_TAG_WORLD );
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

			// mesh_t stores both counts in an unsigned short; anything above that
			// would allocate from the untruncated value and index with the truncated one
			if( numVerts > USHRT_MAX || numElems > USHRT_MAX )
				ri.Com_Error( ERR_DROP, "Mod_CreateMeshForSurface: patch tesselates to %i verts / %i elems in %s",
					numVerts, numElems, loadmodel->name );

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

			// Mod_ValidateFaces has already bounded these against the vertex and
			// element lumps; this is the mesh_t unsigned short limit
			if( numVerts > USHRT_MAX || numElems > USHRT_MAX )
				ri.Com_Error( ERR_DROP, "Mod_CreateMeshForSurface: face has %i verts / %i elems in %s",
					numVerts, numElems, loadmodel->name );

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
* Mod_LoadNodes
*/
static void Mod_LoadNodes( const lump_t *l )
{
	int i, j, count, p;
	dnode_t	*in;
	mnode_t	*out;
	bool badBounds;

	count = Mod_LumpCountMax( l, sizeof( *in ), "nodes", MAX_MAP_NODES );
	in = Mod_LumpData( l );
	out = Q_CallocAligned(count,16,sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);
							
	loadbmodel->nodes = out;
	loadbmodel->numnodes = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		p = LittleLong( in->planenum );
		if( p < 0 || (unsigned)p >= loadbmodel->numplanes )
			ri.Com_Error( ERR_DROP, "Mod_LoadNodes: node %i has bad plane %i in %s", i, p, loadmodel->name );
		out->plane = loadbmodel->planes + p;

		for( j = 0; j < 2; j++ )
		{
			p = LittleLong( in->children[j] );
			if( p >= 0 )
			{
				if( (unsigned)p >= loadbmodel->numnodes )
					ri.Com_Error( ERR_DROP, "Mod_LoadNodes: node %i has bad child node %i in %s", i, p, loadmodel->name );
				out->children[j] = loadbmodel->nodes + p;
			}
			else
			{
				// negative numbers are -(leafs+1)
				if( p == INT_MIN || (unsigned)( -1 - p ) >= loadbmodel->numleafs )
					ri.Com_Error( ERR_DROP, "Mod_LoadNodes: node %i has bad child leaf %i in %s", i, -1 - p, loadmodel->name );
				out->children[j] = ( mnode_t * )( loadbmodel->leafs + ( -1 - p ) );
			}
		}

		badBounds = false;
		for( j = 0; j < 3; j++ )
		{
			out->mins[j] = (float)LittleLong( in->mins[j] );
			out->maxs[j] = (float)LittleLong( in->maxs[j] );
			if( out->mins[j] > out->maxs[j] )
				badBounds = true;
		}

		if( badBounds || VectorCompare( out->mins, out->maxs ) )
		{
			ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: bad node %i bounds:\n", i );
			ri.Com_DPrintf( S_COLOR_YELLOW "mins: %i %i %i\n", Q_rint( out->mins[0] ), Q_rint( out->mins[1] ), Q_rint( out->mins[2] ) );
			ri.Com_DPrintf( S_COLOR_YELLOW "maxs: %i %i %i\n", Q_rint( out->maxs[0] ), Q_rint( out->maxs[1] ), Q_rint( out->maxs[2] ) );
		}
	}
}

/*
* Mod_LoadFogs
*/
static void Mod_LoadFogs( const lump_t *l, const lump_t *brLump, const lump_t *brSidesLump )
{
	int i, j, count, p;
	int numbrushes, numbrushsides, numsides, firstside, visibleside;
	dfog_t *in;
	mfog_t *out;
	dbrush_t *inbrushes, *brush;
	int brushplanes[6];
	dbrushside_t *inbrushsides = NULL, *brushside = NULL;
	rdbrushside_t *inrbrushsides = NULL, *rbrushside = NULL;

	numbrushes = Mod_LumpCount( brLump, sizeof( *inbrushes ), "brushes" );
	inbrushes = Mod_LumpData( brLump );

	if( mod_bspFormat->flags & BSP_RAVEN )
	{
		numbrushsides = Mod_LumpCountMax( brSidesLump, sizeof( *inrbrushsides ), "brushsides", MAX_MAP_BRUSHSIDES );
		inrbrushsides = Mod_LumpData( brSidesLump );
	}
	else
	{
		numbrushsides = Mod_LumpCountMax( brSidesLump, sizeof( *inbrushsides ), "brushsides", MAX_MAP_BRUSHSIDES );
		inbrushsides = Mod_LumpData( brSidesLump );
	}

	count = Mod_LumpCountMax( l, sizeof( *in ), "fogs", MAX_MAP_FOGS );
	in = Mod_LumpData( l );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool( out, loadmodel->mempool );

	loadbmodel->fogs = out;
	loadbmodel->numfogs = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		out->shader = R_RegisterShader( in->shader, SHADER_TYPE_2D );
		p = LittleLong( in->brushnum );
		if( p == -1 )
			continue;
		if( p < 0 || p >= numbrushes ) {
			out->shader = NULL;
			ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: fog %i references bad brush %i\n", i, p );
			continue;
		}

		brush = inbrushes + p;

		numsides = LittleLong( brush->numsides );
		if( numsides < 6 ) {
			out->shader = NULL;
			ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: missing fog brush sides\n" );
			continue;
		}

		firstside = LittleLong( brush->firstside );
		if( firstside == -1 ) {
			out->shader = NULL;
			ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: bad fog brush side\n" );
			continue;
		}

		// the loop below reads sides [firstside, firstside+6) unconditionally,
		// and visibleside indexes the same brush's side range
		if( firstside < 0 || numsides > numbrushsides - firstside ) {
			out->shader = NULL;
			ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: fog %i brush sides out of range\n", i );
			continue;
		}

		if( mod_bspFormat->flags & BSP_RAVEN )
			rbrushside = inrbrushsides + firstside;
		else
			brushside = inbrushsides + firstside;

		visibleside = LittleLong( in->visibleside );
		if( visibleside < -1 || visibleside >= numsides ) {
			ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: fog %i has bad visibleside %i\n", i, visibleside );
			visibleside = -1;
		}

		p = -1;
		if( mod_bspFormat->flags & BSP_RAVEN )
		{
			if( visibleside != -1 )
				p = LittleLong( rbrushside[visibleside].planenum );
			for( j = 0; j < 6; j++ )
				brushplanes[j] = LittleLong( rbrushside[j].planenum );
		}
		else
		{
			if( visibleside != -1 )
				p = LittleLong( brushside[visibleside].planenum );
			for( j = 0; j < 6; j++ )
				brushplanes[j] = LittleLong( brushside[j].planenum );
		}

		if( p >= 0 && (unsigned)p < loadbmodel->numplanes )
			out->visibleplane = loadbmodel->planes + p;

		for( j = 0; j < 6; j++ )
		{
			if( brushplanes[j] < 0 || (unsigned)brushplanes[j] >= loadbmodel->numplanes ) {
				out->shader = NULL;
				out->visibleplane = NULL;
				ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: fog %i brush side %i has bad plane %i\n", i, j, brushplanes[j] );
				break;
			}
		}
		if( j != 6 )
			continue;

		// brushes are always sorted with the axial sides first

		VectorSet( out->mins,
			-loadbmodel->planes[brushplanes[0]].dist,
			-loadbmodel->planes[brushplanes[2]].dist,
			-loadbmodel->planes[brushplanes[4]].dist
			);
		VectorSet( out->maxs, 
			 loadbmodel->planes[brushplanes[1]].dist,
			 loadbmodel->planes[brushplanes[3]].dist,
			 loadbmodel->planes[brushplanes[5]].dist
			);
	}
}

/*
* Mod_LoadLeafs
*/
static void Mod_LoadLeafs( const lump_t *l, const lump_t *msLump )
{
	int i, j, k, count, countMarkSurfaces;
	dleaf_t	*in;
	mleaf_t	*out;
	size_t size;
	uint8_t *buffer;
	bool badBounds;
	int *inMarkSurfaces;
	int numMarkSurfaces, firstMarkSurface;
	int numVisSurfaces, numFragmentSurfaces;

	countMarkSurfaces = Mod_LumpCountMax( msLump, sizeof( *inMarkSurfaces ), "leaffaces", MAX_MAP_LEAFFACES );
	inMarkSurfaces = Mod_LumpData( msLump );

	count = Mod_LumpCountMax( l, sizeof( *in ), "leafs", MAX_MAP_LEAFS );
	in = Mod_LumpData( l );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool( out, loadmodel->mempool );

	loadbmodel->leafs = out;
	loadbmodel->numleafs = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		badBounds = false;
		for( j = 0; j < 3; j++ )
		{
			out->mins[j] = (float)LittleLong( in->mins[j] );
			out->maxs[j] = (float)LittleLong( in->maxs[j] );
			if( out->mins[j] > out->maxs[j] )
				badBounds = true;
		}
		out->cluster = LittleLong( in->cluster );

		if( i && ( badBounds || VectorCompare( out->mins, out->maxs ) ) && out->cluster >= 0 )
		{
			ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: bad leaf bounds\n" );
			out->cluster = -1;
		}

		// NOTE: loadbmodel->pvs is only assigned by Mod_FinalizeBrushModel, i.e. after
		// the whole file has been parsed, so this never fires at load time. The cluster
		// is re-validated against the vis data there; here we only keep it sane.
		if( loadbmodel->pvs && ( out->cluster >= loadbmodel->pvs->numclusters ) )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: leaf cluster > numclusters" );
			out->cluster = -1;
		}

		out->plane = NULL;
		out->area = LittleLong( in->area );
		// -1 means "no area"; only the upper bound matters, since it sizes
		// loadbmodel->numareas
		if( out->area >= MAX_MAP_AREAS )
			ri.Com_Error( ERR_DROP, "Mod_LoadLeafs: leaf %i has bad area %i in %s", i, out->area, loadmodel->name );
		if( out->area >= loadbmodel->numareas )
			loadbmodel->numareas = out->area + 1;

		numVisSurfaces = numFragmentSurfaces = 0;
		numMarkSurfaces = LittleLong( in->numleaffaces );
		if( !numMarkSurfaces )
		{
			//out->cluster = -1;
			continue;
		}

		firstMarkSurface = LittleLong( in->firstleafface );
		if( numMarkSurfaces < 0 || firstMarkSurface < 0 || numMarkSurfaces > countMarkSurfaces - firstMarkSurface )
			ri.Com_Error( ERR_DROP, "MOD_LoadBmodel: bad marksurfaces in leaf %i", i );

		numVisSurfaces = numMarkSurfaces;
		numFragmentSurfaces = numMarkSurfaces;

		size = ((numVisSurfaces + 1) + (numFragmentSurfaces + 1)) * sizeof( msurface_t * );
		buffer = ( uint8_t * )Q_CallocAligned(((numVisSurfaces + 1) + (numFragmentSurfaces + 1)), 16, sizeof( msurface_t *));
		Q_LinkToPool(buffer, loadmodel->mempool);

		out->firstVisSurface = ( msurface_t ** )buffer;
		buffer += ( numVisSurfaces + 1 ) * sizeof( msurface_t * );

		out->firstFragmentSurface = ( msurface_t ** )buffer;
		buffer += ( numFragmentSurfaces + 1 ) * sizeof( msurface_t * );

		numVisSurfaces = numFragmentSurfaces = 0;
		for( j = 0; j < numMarkSurfaces; j++ )
		{
			k = LittleLong( inMarkSurfaces[firstMarkSurface + j] );
			if( k < 0 || (unsigned)k >= loadbmodel->numsurfaces )
				ri.Com_Error( ERR_DROP, "MOD_LoadBmodel: bad surface number %i in leaf %i", k, i );

			out->firstVisSurface[numVisSurfaces++] = loadbmodel->surfaces + k;
			out->firstFragmentSurface[numFragmentSurfaces++] = loadbmodel->surfaces + k;
		}
	}
}

/*
* Mod_LoadElems
*/
static void Mod_LoadElems( const lump_t *l )
{
	int i, count;
	int *in;
	elem_t	*out;

	count = Mod_LumpCountMax( l, sizeof( *in ), "elements", MAX_MAP_INDICES );
	in = Mod_LumpData( l );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);

	loadmodel_surfelems = out;
	loadmodel_numsurfelems = count;

	for( i = 0; i < count; i++ )
	{
		int e = LittleLong( in[i] );

		// element values end up indexing mesh->xyzArray, and elem_t is only 16 bits
		// wide, so an out-of-range value silently truncates into a bogus vertex
		if( e < 0 || e >= loadmodel_numverts )
			ri.Com_Error( ERR_DROP, "Mod_LoadElems: element %i out of range (%i of %i) in %s",
				i, e, loadmodel_numverts, loadmodel->name );

		out[i] = e;
	}
}

/*
* Mod_ValidateFaces
*
* Faces are parsed before the vertex and element lumps, but every face indexes
* into both. This runs once the counts are known and before anything walks a
* face: Mod_LoadPatchGroups -> Mod_AddUpdatePatchGroup reads control points
* straight out of loadmodel_xyz_array, and Mod_Finish -> Mod_CreateMeshForSurface
* copies whole vertex/element ranges. Everything they can reach is bounded here,
* so those two stay free of scattered per-field checks.
*
* Also validates the submodel face ranges, which the surface count is only
* known well enough to check at this point.
*/
static void Mod_ValidateFaces( void )
{
	int i;
	rdface_t *in;

	in = loadmodel_dsurfaces;
	for( i = 0; i < loadmodel_numsurfaces; i++, in++ )
	{
		int facetype = LittleLong( in->facetype );
		int firstvert = LittleLong( in->firstvert );
		int numverts = LittleLong( in->numverts );
		int firstelem = LittleLong( in->firstelem );
		int numelems = LittleLong( in->numelems );
		int patch_cp[2];

		patch_cp[0] = LittleLong( in->patch_cp[0] );
		patch_cp[1] = LittleLong( in->patch_cp[1] );

		switch( facetype )
		{
		case FACETYPE_PATCH:
			// a patch with a zero-sized control grid is dropped by
			// Mod_AddUpdatePatchGroup and never produces a mesh
			if( !patch_cp[0] || !patch_cp[1] )
				continue;

			// Bezier control grids are 2n+1 on each axis. Patch_GetFlatness and
			// Patch_Evaluate walk them in steps of two and read p+2*cp[0]+2,
			// which lands exactly on the last control point when both
			// dimensions are odd - and a full row past the end when they are
			// not. So "odd" is a memory-safety requirement, not a nicety.
			if( patch_cp[0] < 3 || patch_cp[1] < 3 ||
				!( patch_cp[0] & 1 ) || !( patch_cp[1] & 1 ) ||
				patch_cp[0] > MAX_PATCH_CP || patch_cp[1] > MAX_PATCH_CP )
				ri.Com_Error( ERR_DROP, "Mod_ValidateFaces: face %i has bad control grid %ix%i in %s",
					i, patch_cp[0], patch_cp[1], loadmodel->name );

			// Patch_GetFlatness and Patch_Evaluate read the whole control grid
			numverts = patch_cp[0] * patch_cp[1];
			numelems = 0;
			break;

		case FACETYPE_FOLIAGE:
			// foliage reuses patch_cp for the instance and vertex counts, and
			// reads one instance origin per instance from the vertex array
			if( patch_cp[0] < 0 || patch_cp[1] < 0 )
				ri.Com_Error( ERR_DROP, "Mod_ValidateFaces: foliage face %i has negative counts in %s",
					i, loadmodel->name );
			numverts = max( patch_cp[0], patch_cp[1] );
			break;

		default:
			break;
		}

		if( numverts < 0 || firstvert < 0 || numverts > loadmodel_numverts - firstvert )
			ri.Com_Error( ERR_DROP, "Mod_ValidateFaces: face %i vertex range (first %i, count %i, of %i) in %s",
				i, firstvert, numverts, loadmodel_numverts, loadmodel->name );

		if( numelems < 0 || firstelem < 0 || numelems > loadmodel_numsurfelems - firstelem )
			ri.Com_Error( ERR_DROP, "Mod_ValidateFaces: face %i element range (first %i, count %i, of %i) in %s",
				i, firstelem, numelems, loadmodel_numsurfelems, loadmodel->name );

		// Mod_CreateMeshForSurface recomputes the surface plane from elems[0..2]
		if( facetype == FACETYPE_PLANAR && numelems < 3 )
			ri.Com_Error( ERR_DROP, "Mod_ValidateFaces: planar face %i has only %i elements in %s",
				i, numelems, loadmodel->name );

		// Element values are relative to the face's own vertex run, not to the
		// vertex lump: Mod_CreateMeshForSurface copies the face's slice of the
		// element lump verbatim and then indexes mesh->xyzArray, which only has
		// this face's numverts entries. So each value has to be bounded by the
		// face, not by loadmodel_numverts.
		{
			int e;

			for( e = 0; e < numelems; e++ )
			{
				elem_t elem = loadmodel_surfelems[firstelem + e];

				if( elem >= (elem_t)numverts )
					ri.Com_Error( ERR_DROP, "Mod_ValidateFaces: face %i element %i is %u, past its %i verts in %s",
						i, e, (unsigned)elem, numverts, loadmodel->name );
			}
		}
	}

	for( i = 0; i < (int)loadbmodel->numsubmodels; i++ )
	{
		const mmodel_t *sub = loadbmodel->submodels + i;

		// firstface/numfaces are unsigned, so a negative value in the file has
		// already wrapped to something huge and is caught by the same comparison
		if( sub->firstface > (unsigned)loadmodel_numsurfaces ||
			sub->numfaces > (unsigned)loadmodel_numsurfaces - sub->firstface )
			ri.Com_Error( ERR_DROP, "Mod_ValidateFaces: submodel %i face range (first %u, count %u, of %i) in %s",
				i, sub->firstface, sub->numfaces, loadmodel_numsurfaces, loadmodel->name );
	}
}

/*
* Mod_LoadPlanes
*/
static void Mod_LoadPlanes( const lump_t *l )
{
	int i, j;
	cplane_t *out;
	dplane_t *in;
	int count;

	count = Mod_LumpCountMax( l, sizeof( *in ), "planes", MAX_MAP_PLANES );
	in = Mod_LumpData( l );
	
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);

	loadbmodel->planes = out;
	loadbmodel->numplanes = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		out->type = PLANE_NONAXIAL;
		out->signbits = 0;

		for( j = 0; j < 3; j++ )
		{
			out->normal[j] = Mod_CheckFloat( LittleFloat( in->normal[j] ), "plane normal" );
			if( out->normal[j] < 0 )
				out->signbits |= 1<<j;
			if( out->normal[j] == 1.0f )
				out->type = j;
		}
		out->dist = Mod_CheckFloat( LittleFloat( in->dist ), "plane distance" );
	}
}

/*
* Mod_LoadLightgrid
*/
static void Mod_LoadLightgrid( const lump_t *l )
{
	int i, j, count;
	dgridlight_t *in;
	mgridlight_t *out;

	count = Mod_LumpCount( l, sizeof( *in ), "lightgrid" );
	in = Mod_LumpData( l );
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

	// the bulk copy below reads count*sizeof( *out ) from a lump sized in sizeof( *in )
	// units, so the two layouts have to stay identical
	if( sizeof( *in ) != sizeof( *out ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadLightgrid_RBSP: lightgrid layout mismatch" );

	count = Mod_LumpCount( l, sizeof( *in ), "lightgrid" );
	in = Mod_LumpData( l );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool(out, loadmodel->mempool);

	loadbmodel->lightgrid = out;
	loadbmodel->numlightgridelems = count;

	// lightgrid is all 8 bit
	memcpy( out, in, count*sizeof( *out ) );
}

/*
* Mod_LoadLightArray
*/
static void Mod_LoadLightArray( void )
{
	int i, count;
	mgridlight_t **out;

	count = loadbmodel->numlightgridelems;
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool( out, loadmodel->mempool );

	loadbmodel->lightarray = out;
	loadbmodel->numlightarrayelems = count;

	for( i = 0; i < count; i++, out++ )
		*out = loadbmodel->lightgrid + i;
}

/*
* Mod_LoadLightArray_RBSP
*/
static void Mod_LoadLightArray_RBSP( const lump_t *l )
{
	int i, count;
	unsigned index;
	unsigned short *in;
	mgridlight_t **out;

	count = Mod_LumpCount( l, sizeof( *in ), "lightarray" );
	in = Mod_LumpData( l );
	out = Q_CallocAligned( count, 16, sizeof( *out ) );
	Q_LinkToPool( out, loadmodel->mempool );

	loadbmodel->lightarray = out;
	loadbmodel->numlightarrayelems = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		index = LittleShort( *in );
		if( index >= (unsigned)loadbmodel->numlightgridelems ) {
			ri.Com_Error( ERR_DROP, "Mod_LoadLightArray_RBSP: funny grid index(%i):%i in %s", i, index, loadmodel->name );
		}
		*out = loadbmodel->lightgrid + index;
	}
}

/*
* Mod_LoadEntities
*/
static void Mod_LoadEntities( const lump_t *l, vec3_t gridSize, vec3_t ambient, vec3_t outline )
{
	int n;
	int entLen;
	char *data, *entities;
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

	// the entity lump is a NUL-terminated string in a well-formed file, but
	// nothing guarantees the terminator, so parse a bounded copy
	entLen = Mod_LumpCount( l, 1, "entities" );
	if( !entLen )
		return;

	entities = Q_Malloc( entLen + 1 );
	memcpy( entities, Mod_LumpData( l ), entLen );
	entities[entLen] = '\0';

	data = entities;
	if( !data[0] )
	{
		Q_Free( entities );
		return;
	}

	for(; ( token = COM_Parse( &data ) ) && token[0] == '{'; )
	{
		isworld = false;

		while( 1 )
		{
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
			if( !strcmp( key, "classname" ) )
			{
				if( !strcmp( value, "worldspawn" ) )
					isworld = true;
			}
			else if( !strcmp( key, "gridsize" ) )
			{
				int gridsizei[3] = { 0, 0, 0 };
				sscanf( value, "%4i %4i %4i", &gridsizei[0], &gridsizei[1], &gridsizei[2] );
				VectorCopy( gridsizei, gridsizef );
			}
			else if( !strcmp( key, "_ambient" ) || ( !strcmp( key, "ambient" ) && ambientf == 0.0f ) )
			{
				n = sscanf( value, "%8f", &ambientf );
				if( n != 1 )
				{
					int ia = 0;
					n = sscanf( value, "%3i", &ia );
					ambientf = ia;
				}
			}
			else if( !strcmp( key, "_color" ) )
			{
				n = sscanf( value, "%8f %8f %8f", &colorf[0], &colorf[1], &colorf[2] );
				if( n != 3 )
				{
					int colori[3] = { 0, 0, 0 };
					sscanf( value, "%3i %3i %3i", &colori[0], &colori[1], &colori[2] );
					VectorCopy( colori, colorf );
				}
			}
			else if( !strcmp( key, "_forceclear" ) )
			{
				if( atof( value ) != 0 )
					mapConfig.forceClear = true;
			}
			else if( !strcmp( key, "_lightingIntensity" ) )
			{
				if( !r_fullbright->integer )
				{
					// non power of two intensity scale for lighting
					float intensity = 0;

					if( sscanf( value, "%8f", &intensity ) == 1 &&
						Mod_IsFiniteFloat( intensity ) && intensity > 0.0f )
						mapConfig.lightingIntensity = intensity;
					else
						ri.Com_DPrintf( S_COLOR_YELLOW "WARNING: ignoring bad _lightingIntensity \"%s\"\n", value );
				}
			}
			else if( !strcmp( key, "_outlinecolor" ) )
			{
				n = sscanf( value, "%8f %8f %8f", &celcolorf[0], &celcolorf[1], &celcolorf[2] );
				if( n != 3 )
				{
					int celcolori[3] = { 0, 0, 0 };
					sscanf( value, "%3i %3i %3i", &celcolori[0], &celcolori[1], &celcolori[2] );
					VectorCopy( celcolori, celcolorf );
				}
			}
			else if( !Q_stricmp( key, "_lightmapimagesize" ) )
			{
				int lmSize = atoi( value );
				if( lmSize > 0 && ( lmSize & ( lmSize - 1 ) ) == 0 )
				{
					mapConfig.lightmapImageSize = lmSize;
				}
				else
					ri.Com_Printf( S_COLOR_YELLOW "WARNING: _lightmapimagesize must be a power of two, ignoring\n" );
			}
		}

		if( isworld )
		{
			VectorCopy( gridsizef, gridSize );

			if( VectorCompare( colorf, vec3_origin ) )
				VectorSet( colorf, 1.0, 1.0, 1.0 );
			VectorScale( colorf, ambientf, ambient );

			if( max( celcolorf[0], max( celcolorf[1], celcolorf[2] ) ) > 1.0f )
				VectorScale( celcolorf, 1.0f/255.0f, celcolorf );	// [0..1] RGB -> [0..255] RGB
			VectorCopy( celcolorf, outline );
			break;
		}
	}

	Q_Free( entities );
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

		if( in->lightmapStyles[j] == 255 || lightmaps[j] < 0 || lightmaps[j] >= loadmodel_numlightmaps || !mesh )
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
* Mod_Finish
*/
static void Mod_Finish( const lump_t *faces, const lump_t *light, vec3_t gridSize, vec3_t ambient, vec3_t outline )
{
	unsigned int i, j;
	msurface_t *surf;
	mfog_t *testFog;
	bool globalFog;
	rdface_t *in;

	// remembe the BSP format just in case
	loadbmodel->format = mod_bspFormat;

	// set up lightgrid
	if( gridSize[0] < 1 || gridSize[1] < 1 || gridSize[2] < 1 )
		VectorSet( loadbmodel->gridSize, 64, 64, 128 );
	else
		VectorCopy( gridSize, loadbmodel->gridSize );

	for( j = 0; j < 3; j++ )
	{
		vec3_t maxs;

		loadbmodel->gridMins[j] = loadbmodel->gridSize[j] * ceil( ( loadbmodel->submodels[0].mins[j] + 1 ) / loadbmodel->gridSize[j] );
		maxs[j] = loadbmodel->gridSize[j] *floor( ( loadbmodel->submodels[0].maxs[j] - 1 ) / loadbmodel->gridSize[j] );
		// submodel[0]'s bounds and gridSize both come from the file, so the
		// quotient is unbounded. Converting a float outside int range is
		// undefined, and gridBounds[3] below is a product of two of these, so
		// the per-axis cap has to leave room for that multiply as well.
		{
			float bounds = ( maxs[j] - loadbmodel->gridMins[j] ) / loadbmodel->gridSize[j];

			if( !( bounds >= 0.0f ) )
				bounds = 0.0f;                  // the negated test also catches NaN
			else if( bounds > (float)MAX_LIGHTGRID_BOUNDS )
				bounds = (float)MAX_LIGHTGRID_BOUNDS;

			loadbmodel->gridBounds[j] = (int)bounds + 1;
		}
	}
	loadbmodel->gridBounds[3] = loadbmodel->gridBounds[1] * loadbmodel->gridBounds[0];

	// ambient lighting
	for( i = 0; i < 3; i++ )
		mapConfig.ambient[i] = ambient[i];
	
	// outline color
	for( i = 0; i < 3; i++ )
		mapConfig.outlineColor[i] = (uint8_t)(bound( 0, outline[i]*255.0f, 255 ));
	mapConfig.outlineColor[3] = 255;

	for( i = 0, testFog = loadbmodel->fogs; i < loadbmodel->numfogs; testFog++, i++ )
	{
		if( !testFog->shader )
			continue;
		if( testFog->visibleplane )
			continue;


		testFog->visibleplane = Q_Malloc(sizeof( cplane_t ));
		memset(testFog->visibleplane, 0, sizeof( cplane_t ));
		Q_LinkToPool(testFog->visibleplane, loadmodel->mempool);
		VectorSet( testFog->visibleplane->normal, 0, 0, 1 );
		testFog->visibleplane->type = PLANE_Z;
		testFog->visibleplane->dist = loadbmodel->submodels[0].maxs[0] + 1;
	}

	// make sure that the only fog in the map has valid shader
	globalFog = ( loadbmodel->numfogs == 1 ) ? true : false;
	if( globalFog )
	{
		testFog = &loadbmodel->fogs[0];
		if( !testFog->shader )
			globalFog = false;
	}

	R_SortSuperLightStyles( loadmodel );

	in = loadmodel_dsurfaces;
	surf = loadbmodel->surfaces;
	for( i = 0; i < loadbmodel->numsurfaces; i++, in++, surf++ ) {
		surf->mesh = Mod_CreateMeshForSurface( in, surf, loadmodel_patchgrouprefs[i] );
		if( surf->mesh ) {
			surf->numVerts = surf->mesh->numVerts;
			surf->numElems = surf->mesh->numElems;
		}

		Mod_ApplySuperStylesToFace( in, surf );

		// force outlines hack for old maps
		if( !mapConfig.forceWorldOutlines 
			&& surf->shader && ( surf->shader->flags & SHADER_FORCE_OUTLINE_WORLD )  ) {
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

/*
* Mod_ResetLoaderState
*
* The loader keeps its scratch arrays in file-scope statics and only clears
* them in Mod_Finish. Any ri.Com_Error along the way longjmps past that, so a
* map that fails validation leaves every one of these pointing into the mempool
* that Mod_ForName is about to release - and the next map to be loaded reads
* through them. Mod_LoadLighting in particular returns early (vertex lighting,
* or an empty lighting lump) without reassigning loadmodel_lightmapRects or
* loadmodel_numlightmaps, so the stale pair is what Mod_LoadFaces indexes.
*
* Nothing is freed here: the previous model's pool already owns it.
*/
static void Mod_ResetLoaderState( void )
{
	int i;

	loadmodel_numverts = 0;
	loadmodel_xyz_array = NULL;
	loadmodel_normals_array = NULL;
	loadmodel_st_array = NULL;
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		loadmodel_lmst_array[i] = NULL;
		loadmodel_colors_array[i] = NULL;
	}

	loadmodel_numsurfelems = 0;
	loadmodel_surfelems = NULL;

	loadmodel_numlightmaps = 0;
	loadmodel_lightmapRects = NULL;

	loadmodel_numshaderrefs = 0;
	loadmodel_shaderrefs = NULL;

	loadmodel_numsurfaces = 0;
	loadmodel_dsurfaces = NULL;

	loadmodel_numpatchgroups = 0;
	loadmodel_maxpatchgroups = 0;
	loadmodel_patchgroups = NULL;
	loadmodel_patchgrouprefs = NULL;

	loadbmodel = NULL;
	mod_base = NULL;
	mod_bufferLen = 0;
}

/*
* Mod_LoadQ3BrushModel
*/
void Mod_LoadQ3BrushModel( model_t *mod, model_t *parent, void *buffer, size_t bufferLen, bspFormatDesc_t *format )
{
	int i;
	dheader_t *header;
	vec3_t gridSize, ambient, outline;

	mod->type = mod_brush;
	mod->registrationSequence = rsh.registrationSequence;
	if( rsh.worldModel != NULL )
		ri.Com_Error( ERR_DROP, "Loaded a brush model after the world" );

	loadmodel = mod;

	Mod_ResetLoaderState();

	mod_bspFormat = format;

	header = (dheader_t *)buffer;
	mod_base = (uint8_t *)header;
	mod_bufferLen = bufferLen;

	// the swap below rewrites the whole header in place, so the file has to be
	// at least that big before we touch it
	if( bufferLen < sizeof( dheader_t ) )
		ri.Com_Error( ERR_DROP, "Mod_LoadQ3BrushModel: %s is truncated (%u bytes, header needs %u)",
			mod->name, (unsigned)bufferLen, (unsigned)sizeof( dheader_t ) );

	// swap all the lumps
	for( i = 0; i < sizeof( dheader_t )/4; i++ )
		( (int *)header )[i] = LittleLong( ( (int *)header )[i] );

	// load into heap
	Mod_LoadSubmodels( &header->lumps[LUMP_MODELS] );
	Mod_LoadEntities( &header->lumps[LUMP_ENTITIES], gridSize, ambient, outline );
	Mod_LoadLighting( &header->lumps[LUMP_LIGHTING], &header->lumps[LUMP_FACES] );
	Mod_LoadShaderrefs( &header->lumps[LUMP_SHADERREFS] );
	Mod_PreloadFaces( &header->lumps[LUMP_FACES] );
	Mod_LoadPlanes( &header->lumps[LUMP_PLANES] );
	Mod_LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES] );
	Mod_LoadFaces( &header->lumps[LUMP_FACES] );
	if( mod_bspFormat->flags & BSP_RAVEN )
		Mod_LoadVertexes_RBSP( &header->lumps[LUMP_VERTEXES] );
	else
		Mod_LoadVertexes( &header->lumps[LUMP_VERTEXES] );
	Mod_LoadElems( &header->lumps[LUMP_ELEMENTS] );

	// faces index the vertex and element lumps, which only exist as of here
	Mod_ValidateFaces();

	if( mod_bspFormat->flags & BSP_RAVEN )
		Mod_LoadLightgrid_RBSP( &header->lumps[LUMP_LIGHTGRID] );
	else
		Mod_LoadLightgrid( &header->lumps[LUMP_LIGHTGRID] );
	Mod_LoadPatchGroups( &header->lumps[LUMP_FACES] );
	Mod_LoadLeafs( &header->lumps[LUMP_LEAFS], &header->lumps[LUMP_LEAFFACES] );
	Mod_LoadNodes( &header->lumps[LUMP_NODES] );
	if( mod_bspFormat->flags & BSP_RAVEN )
		Mod_LoadLightArray_RBSP( &header->lumps[LUMP_LIGHTARRAY] );
	else
		Mod_LoadLightArray();

	Mod_Finish( &header->lumps[LUMP_FACES], &header->lumps[LUMP_LIGHTING], gridSize, ambient, outline );
}
