/*
Copyright (C) 1997-2001 Id Software, Inc.
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

// r_main.c

#include "r_local.h"
#include "r_nri.h"
#include "r_math_util.h"

r_globals_t rf;
mapconfig_t mapConfig;
refinst_t rn;
r_scene_t rsc;

/*
* R_TransformForWorld
*/
void R_TransformForWorld( void )
{
	Matrix4_Identity( rn.objectMatrix );
	Matrix4_Copy( rn.cameraMatrix, rn.modelviewMatrix );

	RB_LoadObjectMatrix( mat4x4_identity );
}

/*
* R_TranslateForEntity
*/
void R_TranslateForEntity( const entity_t *e )
{
	Matrix4_Identity( rn.objectMatrix );

	rn.objectMatrix[0] = e->scale;
	rn.objectMatrix[5] = e->scale;
	rn.objectMatrix[10] = e->scale;
	rn.objectMatrix[12] = e->origin[0];
	rn.objectMatrix[13] = e->origin[1];
	rn.objectMatrix[14] = e->origin[2];

	RB_LoadObjectMatrix( rn.objectMatrix );
}

/*
* R_TransformForEntity
*/
void R_TransformForEntity( const entity_t *e )
{
	if( e->rtype != RT_MODEL ) {
		R_TransformForWorld();
		return;
	}
	if( e == rsc.worldent ) {
		R_TransformForWorld();
		return;
	}

	if( e->scale != 1.0f ) {
		rn.objectMatrix[0] = e->axis[0] * e->scale;
		rn.objectMatrix[1] = e->axis[1] * e->scale;
		rn.objectMatrix[2] = e->axis[2] * e->scale;
		rn.objectMatrix[4] = e->axis[3] * e->scale;
		rn.objectMatrix[5] = e->axis[4] * e->scale;
		rn.objectMatrix[6] = e->axis[5] * e->scale;
		rn.objectMatrix[8] = e->axis[6] * e->scale;
		rn.objectMatrix[9] = e->axis[7] * e->scale;
		rn.objectMatrix[10] = e->axis[8] * e->scale;
	} else {
		rn.objectMatrix[0] = e->axis[0];
		rn.objectMatrix[1] = e->axis[1];
		rn.objectMatrix[2] = e->axis[2];
		rn.objectMatrix[4] = e->axis[3];
		rn.objectMatrix[5] = e->axis[4];
		rn.objectMatrix[6] = e->axis[5];
		rn.objectMatrix[8] = e->axis[6];
		rn.objectMatrix[9] = e->axis[7];
		rn.objectMatrix[10] = e->axis[8];
	}

	rn.objectMatrix[3] = 0;
	rn.objectMatrix[7] = 0;
	rn.objectMatrix[11] = 0;
	rn.objectMatrix[12] = e->origin[0];
	rn.objectMatrix[13] = e->origin[1];
	rn.objectMatrix[14] = e->origin[2];
	rn.objectMatrix[15] = 1.0;

	Matrix4_MultiplyFast( rn.cameraMatrix, rn.objectMatrix, rn.modelviewMatrix );

	RB_LoadObjectMatrix( rn.objectMatrix );
}

/*
* R_FogForBounds
*/
mfog_t *R_FogForBounds( const vec3_t mins, const vec3_t maxs )
{
	unsigned int i, j;
	mfog_t *fog;

	if( !rsh.worldModel || ( rn.refdef.rdflags & RDF_NOWORLDMODEL ) || !rsh.worldBrushModel->numfogs )
		return NULL;
	if( rn.renderFlags & RF_SHADOWMAPVIEW )
		return NULL;
	if( rsh.worldBrushModel->globalfog )
		return rsh.worldBrushModel->globalfog;

	fog = rsh.worldBrushModel->fogs;
	for( i = 0; i < rsh.worldBrushModel->numfogs; i++, fog++ )
	{
		if( !fog->shader )
			continue;

		for( j = 0; j < 3; j++ ) {
			if( mins[j] >= fog->maxs[j] ) {
				break;
			}
			if( maxs[j] <= fog->mins[j] ) {
				break;
			}
		}

		if( j == 3 ) {
			return fog;
		}
	}

	return NULL;
}

/*
* R_FogForSphere
*/
mfog_t *R_FogForSphere( const vec3_t centre, const float radius )
{
	int i;
	vec3_t mins, maxs;

	for( i = 0; i < 3; i++ ) {
		mins[i] = centre[i] - radius;
		maxs[i] = centre[i] + radius;
	}

	return R_FogForBounds( mins, maxs );
}

/*
* R_CompletelyFogged
*/
bool R_CompletelyFogged( const mfog_t *fog, vec3_t origin, float radius )
{
	// note that fog->distanceToEye < 0 is always true if
	// globalfog is not NULL and we're inside the world boundaries
	if( fog && fog->shader && fog == rn.fog_eye )
	{
		float vpnDist = ( ( rn.viewOrigin[0] - origin[0] ) * rn.viewAxis[AXIS_FORWARD+0] + 
			( rn.viewOrigin[1] - origin[1] ) * rn.viewAxis[AXIS_FORWARD+1] + 
			( rn.viewOrigin[2] - origin[2] ) * rn.viewAxis[AXIS_FORWARD+2] );
		return ( ( vpnDist + radius ) / fog->shader->fog_dist ) < -1;
	}

	return false;
}

/*
* R_LODForSphere
*/
int R_LODForSphere( const vec3_t origin, float radius )
{
	float dist;
	int lod;

	dist = DistanceFast( origin, rn.lodOrigin );
	dist *= rn.lod_dist_scale_for_fov;

	lod = (int)( dist / radius );
	if( r_lodscale->integer )
		lod /= r_lodscale->integer;
	lod += r_lodbias->integer;

	if( lod < 1 )
		return 0;
	return lod;
}

/*
* R_InitCustomColors
*/
void R_InitCustomColors( void )
{
	memset( rsh.customColors, 255, sizeof( rsh.customColors ) );
}

/*
* R_SetCustomColor
*/
void R_SetCustomColor( int num, int r, int g, int b )
{
	if( num < 0 || num >= NUM_CUSTOMCOLORS )
		return;
	Vector4Set( rsh.customColors[num], (uint8_t)r, (uint8_t)g, (uint8_t)b, 255 );
}

int R_GetCustomColor( int num )
{
	if( num < 0 || num >= NUM_CUSTOMCOLORS )
		return COLOR_RGBA( 255, 255, 255, 255 );
	return *(int *)rsh.customColors[num];
}

void R_ShutdownCustomColors( void )
{
	memset( rsh.customColors, 255, sizeof( rsh.customColors ) );
}

/*
* R_InitNullModelVBO
*/
mesh_vbo_t *R_InitNullModelVBO( void )
{
	float scale = 15;
	vec4_t xyz[6] = { {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1} };
	vec4_t normals[6] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	byte_vec4_t colors[6];
	vec2_t texcoords[6] = { {0,0}, {0,1}, {0,0}, {0,1}, {0,0}, {0,1} };
	elem_t elems[6] = { 0, 1, 2, 3, 4, 5 };
	mesh_t mesh;
	vattribmask_t vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT | VATTRIB_COLOR0_BIT;
	mesh_vbo_t *vbo;
	
	vbo = R_CreateMeshVBO( &rf, 6, 6, 0, vattribs, VBO_TAG_NONE, vattribs );
	if( !vbo ) {
		return NULL;
	}

	VectorSet( xyz[0], 0, 0, 0 );
	VectorSet( xyz[1], scale, 0, 0 );
	Vector4Set( colors[0], 255, 0, 0, 127 );
	Vector4Set( colors[1], 255, 0, 0, 127 );

	VectorSet( xyz[2], 0, 0, 0 );
	VectorSet( xyz[3], 0, scale, 0 );
	Vector4Set( colors[2], 0, 255, 0, 127 );
	Vector4Set( colors[3], 0, 255, 0, 127 );

	VectorSet( xyz[4], 0, 0, 0 );
	VectorSet( xyz[5], 0, 0, scale );
	Vector4Set( colors[4], 0, 0, 255, 127 );
	Vector4Set( colors[5], 0, 0, 255, 127 );

	memset( &mesh, 0, sizeof( mesh ) );
	mesh.numVerts = 6;
	mesh.xyzArray = xyz;
	mesh.normalsArray = normals;
	mesh.stArray = texcoords;
	mesh.colorsArray[0] = colors;
	mesh.numElems = 6;
	mesh.elems = elems;

	R_UploadVBOVertexData( vbo, 0, vattribs, &mesh );
	R_UploadVBOElemData( vbo, 0, 0, &mesh );

	return vbo;
}

/*
* R_DrawNullSurf
*/
void R_DrawNullSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfaceType_t *drawSurf )
{
	assert( rsh.nullVBO != NULL );
	if( !rsh.nullVBO ) {
		return;
	}

	RB_BindVBO( rsh.nullVBO->index, GL_LINES );

	RB_DrawElements( 0, 6, 0, 6, 0, 0, 0, 0 );
}

static bool R_AddNullSurfToDrawList( const entity_t *e )
{
	static drawSurfaceType_t nullDrawSurf = ST_NULLMODEL;
	if( !R_AddSurfToDrawList( rn.meshlist, e, R_FogForSphere( e->origin, 0.1f ), 
		rsh.whiteShader, 0, 0, NULL, &nullDrawSurf ) ) {
		return false;
	}

	return true;
}

//==================================================================================

static vec4_t pic_xyz[4] = { {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1} };
static vec4_t pic_normals[4] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
static vec2_t pic_st[4];
static byte_vec4_t pic_colors[4];
static elem_t pic_elems[6] = { 0, 1, 2, 0, 2, 3 };
static mesh_t pic_mesh = { 4, pic_xyz, pic_normals, NULL, pic_st, { 0, 0, 0, 0 }, { 0 }, { pic_colors, pic_colors, pic_colors, pic_colors }, NULL, NULL, 6, pic_elems };

/*
* R_DrawRotatedStretchPic
*/
void R_DrawRotatedStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, float angle, 
	const vec4_t color, const shader_t *shader )
{
	int bcolor;

	if( !shader ) {
		return;
	}

	if( shader->cin ) {
		R_UploadCinematicShader( shader );
	}

	// lower-left
	Vector2Set( pic_xyz[0], x, y );
	Vector2Set( pic_st[0], s1, t1 );
	Vector4Set( pic_colors[0], 
		bound( 0, ( int )( color[0] * 255.0f ), 255 ), 
		bound( 0, ( int )( color[1] * 255.0f ), 255 ),
		bound( 0, ( int )( color[2] * 255.0f ), 255 ), 
		bound( 0, ( int )( color[3] * 255.0f ), 255 ) );
	bcolor = *(int *)pic_colors[0];

	// lower-right
	Vector2Set( pic_xyz[1], x+w, y );
	Vector2Set( pic_st[1], s2, t1 );
	*(int *)pic_colors[1] = bcolor;

	// upper-right
	Vector2Set( pic_xyz[2], x+w, y+h );
	Vector2Set( pic_st[2], s2, t2 );
	*(int *)pic_colors[2] = bcolor;

	// upper-left
	Vector2Set( pic_xyz[3], x, y+h );
	Vector2Set( pic_st[3], s1, t2 );
	*(int *)pic_colors[3] = bcolor;

	// rotated image
	angle = anglemod( angle );
	if( angle ) {
		int j;
		float sint, cost;

		angle = DEG2RAD( angle );
		sint = sin( angle );
		cost = cos( angle );

		for( j = 0; j < 4; j++ )
		{
			t1 = pic_st[j][0];
			t2 = pic_st[j][1];
			pic_st[j][0] = cost * (t1 - 0.5f) - sint * (t2 - 0.5f) + 0.5f;
			pic_st[j][1] = cost * (t2 - 0.5f) + sint * (t1 - 0.5f) + 0.5f;
		}
	}

	RB_AddDynamicMesh( NULL, shader, NULL, NULL, 0, &pic_mesh, GL_TRIANGLES, 0.0f, 0.0f );
}

/*
* R_DrawStretchPic
*/
void R_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, 
	const vec4_t color, const shader_t *shader )
{
	R_DrawRotatedStretchPic( x, y, w, h, s1, t1, s2, t2, 0, color, shader );
}

/*
* R_UploadRawPic
*/
void R_UploadRawPic( image_t *texture, int cols, int rows, uint8_t *data )
{
	if( texture->width != cols || texture->height != rows ) {
		uint8_t *nodata[1] = { NULL };
		R_ReplaceImage( texture, nodata, cols, rows, texture->flags, 1, 3 );
	}
	R_ReplaceSubImage( texture, 0, 0, 0, &data, cols, rows );
}

/*
* R_UploadRawYUVPic
*/
void R_UploadRawYUVPic( image_t **yuvTextures, ref_img_plane_t *yuv )
{
	int i;

	for( i = 0; i < 3; i++ ) {
		uint8_t *data = yuv[i].data;
		int flags = yuvTextures[i]->flags;
		int stride = yuv[i].stride;
		int height = yuv[i].height;

		if( stride < 0 ) {
			// negative stride flips the image vertically
			data = data + stride * height;
			flags = (flags & ~(IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL)) | IT_FLIPY;
			stride = -stride;
		}

		if( yuvTextures[i]->width != stride || yuvTextures[i]->height != height ) {
			uint8_t *nodata[1] = { NULL };
			R_ReplaceImage( yuvTextures[i], nodata, stride, height, flags, 1, 1 );
		}
		R_ReplaceSubImage( yuvTextures[i], 0, 0, 0, &data, stride, height );		
	}
}

/*
* R_DrawStretchRaw
*/
void R_DrawStretchRaw( int x, int y, int w, int h, float s1, float t1, float s2, float t2 )
{
	float h_scale, v_scale;

	h_scale = (float)rsh.rawTexture->width / rsh.rawTexture->upload_width;
	v_scale = (float)rsh.rawTexture->height / rsh.rawTexture->upload_height;
	s1 *= h_scale;
	s2 *= h_scale;
	t1 *= v_scale;
	t2 *= v_scale;

	R_DrawStretchQuick( x, y, w, h, s1, t1, s2, t2, colorWhite, GLSL_PROGRAM_TYPE_NONE, rsh.rawTexture, 0 );
}

/*
* Set bit 0 in 'flip' to flip the image horizontally
* Set bit 1 in 'flip' to flip the image vertically
*/
void R_DrawStretchRawYUVBuiltin( int x, int y, int w, int h, 
	float s1, float t1, float s2, float t2, image_t **yuvTextures, int flip )
{
	static char *s_name = "$builtinyuv";
	static shaderpass_t p;
	static shader_t s;
	float h_scale, v_scale;
	float s2_, t2_;
	float h_ofs, v_ofs;

	s.vattribs = VATTRIB_POSITION_BIT|VATTRIB_TEXCOORDS_BIT;
	s.sort = SHADER_SORT_NEAREST;
	s.numpasses = 1;
	s.name = s_name;
	s.passes = &p;

	p.rgbgen.type = RGB_GEN_IDENTITY;
	p.alphagen.type = ALPHA_GEN_IDENTITY;
	p.tcgen = TC_GEN_BASE;
	p.images[0] = yuvTextures[0];
	p.images[1] = yuvTextures[1];
	p.images[2] = yuvTextures[2];
	p.flags = 0;
	p.program_type = GLSL_PROGRAM_TYPE_YUV;

	h_scale = (float)yuvTextures[0]->width / yuvTextures[0]->upload_width;
	v_scale = (float)yuvTextures[0]->height / yuvTextures[0]->upload_height;
	h_ofs = 1.0f / yuvTextures[0]->upload_width;
	v_ofs = 1.0f / yuvTextures[0]->upload_height;

	s1 *= h_scale;
	s2 *= h_scale;
	t1 *= v_scale;
	t2 *= v_scale;

	s2_ = s2;
	t2_ = t2;
	if( flip & 1 ) {
		s1 = s2_ - s1, s2 = s2_ - s2;
	}
	if( flip & 2 ) {
		t1 = t2_ - t1, t2 = t2_ - t2;
	}

	// avoid lerp seams
	if( s1 > s2 ) {
		s1 -= h_ofs, s2 += h_ofs;
	} else {
		s1 += h_ofs, s2 -= h_ofs;
	}
	if( t1 > t2 ) {
		t1 -= v_ofs, t2 += v_ofs;
	} else {
		t1 += v_ofs, t2 -= v_ofs;
	}

	R_DrawRotatedStretchPic( x, y, w, h, s1, t1, s2, t2, 0, colorWhite, &s );

	RB_FlushDynamicMeshes();
}

/*
* R_DrawStretchRawYUV
*/
void R_DrawStretchRawYUV( int x, int y, int w, int h, float s1, float t1, float s2, float t2 )
{
	R_DrawStretchRawYUVBuiltin( x, y, w, h, s1, t1, s2, t2, rsh.rawYUVTextures, 0 );
}

/*
* R_DefaultFarClip
*/
float R_DefaultFarClip( void )
{
	float farclip_dist;

	if( rn.renderFlags & RF_SHADOWMAPVIEW ) {
		return rn.shadowGroup->projDist;
	} else if( rn.refdef.rdflags & RDF_NOWORLDMODEL ) {
		farclip_dist = 1024;
	} else if( rsh.worldModel && rsh.worldBrushModel->globalfog ) {
		farclip_dist = rsh.worldBrushModel->globalfog->shader->fog_dist;
	} else {
		farclip_dist = Z_NEAR;
	}

	return max( Z_NEAR, farclip_dist ) + Z_BIAS;
}

/*
* R_SetVisFarClip
*/
static float R_SetVisFarClip( void )
{
	int i;
	float dist;
	vec3_t tmp;
	float farclip_dist;

	if( !rsh.worldModel || ( rn.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
		return rn.visFarClip;
	}

	rn.visFarClip = 0;

	farclip_dist = 0;
	for( i = 0; i < 8; i++ )
	{
		tmp[0] = ( ( i & 1 ) ? rn.visMins[0] : rn.visMaxs[0] );
		tmp[1] = ( ( i & 2 ) ? rn.visMins[1] : rn.visMaxs[1] );
		tmp[2] = ( ( i & 4 ) ? rn.visMins[2] : rn.visMaxs[2] );

		dist = DistanceSquared( tmp, rn.viewOrigin );
		farclip_dist = max( farclip_dist, dist );
	}

	rn.visFarClip = sqrt( farclip_dist );
	return rn.visFarClip;
}

/*
* R_SetFarClip
*/
static void R_SetFarClip( void )
{
	float farclip;

	if( rn.refdef.rdflags & RDF_NOWORLDMODEL ) {
		rn.farClip = R_DefaultFarClip();
		return;
	}

	farclip = R_SetVisFarClip();

	if( rsh.worldBrushModel->globalfog )
	{
		float fogdist = rsh.worldBrushModel->globalfog->shader->fog_dist;
		if( farclip > fogdist )
			farclip = fogdist;
		else
			rn.clipFlags &= ~16;
	}

	rn.farClip = max( Z_NEAR, farclip ) + Z_BIAS;
}

/*
* R_SetupFrame
*/
static void R_SetupFrame( void )
{
	int viewcluster;
	int viewarea;

	// build the transformation matrix for the given view angles
	VectorCopy( rn.refdef.vieworg, rn.viewOrigin );
	Matrix3_Copy( rn.refdef.viewaxis, rn.viewAxis );

	rn.lod_dist_scale_for_fov = tan( rn.refdef.fov_x * ( M_PI/180 ) * 0.5f );

	// current viewcluster
	if( !( rn.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		mleaf_t *leaf;

		VectorCopy( rsh.worldModel->mins, rn.visMins );
		VectorCopy( rsh.worldModel->maxs, rn.visMaxs );

		leaf = Mod_PointInLeaf( rn.pvsOrigin, rsh.worldModel );
		viewcluster = leaf->cluster;
		viewarea = leaf->area;

		if( rf.worldModelSequence != rsh.worldModelSequence ) {
			rf.frameCount = 0;
			rf.viewcluster = -1; // force R_MarkLeaves
			rf.haveOldAreabits = false;
			rf.worldModelSequence = rsh.worldModelSequence;

			// load all world images if not yet
			R_FinishLoadingImages();
		}
	}
	else
	{
		viewcluster = -1;
		viewarea = -1;
	}

	rf.oldviewcluster = rf.viewcluster;
	rf.viewcluster = viewcluster;
	rf.viewarea = viewarea;

	rf.frameCount++;
}

/*
* R_SetupViewMatrices
*/
static void R_SetupViewMatrices( void )
{
	refdef_t *rd = &rn.refdef;

	Matrix4_Modelview( rd->vieworg, rd->viewaxis, rn.cameraMatrix );

	if( rd->rdflags & RDF_USEORTHO ) {
		Matrix4_OrthogonalProjection( -rd->ortho_x, rd->ortho_x, -rd->ortho_y, rd->ortho_y, 
			-rn.farClip, rn.farClip, rn.projectionMatrix );
	}
	else {
		Matrix4_PerspectiveProjection( rd->fov_x, rd->fov_y, 
			Z_NEAR, rn.farClip, rf.cameraSeparation, rn.projectionMatrix );
	}

	if( rd->rdflags & RDF_FLIPPED ) {
		rn.projectionMatrix[0] = -rn.projectionMatrix[0];
		rn.renderFlags |= RF_FLIPFRONTFACE;
	}

	Matrix4_Multiply( rn.projectionMatrix, rn.cameraMatrix, rn.cameraProjectionMatrix );
}

/*
* R_Clear
*/
static void R_Clear( int bitMask )
{
	int bits;
	vec4_t envColor;
	bool clearColor = false;
	bool rgbShadow = ( rn.renderFlags & RF_SHADOWMAPVIEW ) && rn.fbColorAttachment != NULL ? true : false;
	bool depthPortal = ( rn.renderFlags & (RF_MIRRORVIEW|RF_PORTALVIEW) ) != 0 && ( rn.renderFlags & RF_PORTAL_CAPTURE ) == 0;

	if( ( rn.refdef.rdflags & RDF_NOWORLDMODEL ) || rgbShadow ) {
		clearColor = rgbShadow;
		Vector4Set( envColor, 1, 1, 1, 1 );
	} else {
		clearColor = !rn.numDepthPortalSurfaces || R_FASTSKY();
		if( rsh.worldBrushModel && rsh.worldBrushModel->globalfog && rsh.worldBrushModel->globalfog->shader ) {
			Vector4Scale( rsh.worldBrushModel->globalfog->shader->fog_color, 1.0/255.0, envColor );
		} else {
			Vector4Scale( mapConfig.environmentColor, 1.0/255.0, envColor );
		}
	}

	bits = 0;
	if( !depthPortal )
		bits |= GL_DEPTH_BUFFER_BIT;
	if( clearColor )
		bits |= GL_COLOR_BUFFER_BIT;
	if( glConfig.stencilBits )
		bits |= GL_STENCIL_BUFFER_BIT;

	bits &= bitMask;

	RB_Clear( bits, envColor[0], envColor[1], envColor[2], 1 );
}

void R_SetWallFloorColors( const vec3_t wallColor, const vec3_t floorColor )
{
	int i;
	for( i = 0; i < 3; i++ ) {
		rsh.wallColor[i] = bound( 0, floor(wallColor[i]) / 255.0, 1.0 );
		rsh.floorColor[i] = bound( 0, floor(floorColor[i]) / 255.0, 1.0 );
	}
}

void R_SetDrawBuffer( const char *drawbuffer )
{
	Q_strncpyz( rf.drawBuffer, drawbuffer, sizeof( rf.drawBuffer ) );
	rf.newDrawBuffer = true;
}

bool R_IsRenderingToScreen( void )
{
	bool surfaceRenderable = true;
	GLimp_GetWindowSurface( &surfaceRenderable );
	return surfaceRenderable;
}

/*
* R_WriteSpeedsMessage
*/
const char *R_WriteSpeedsMessage(char *out, size_t size)
{
	char backend_msg[1024];

	if (!out || !size) {
		return out;
	}

	out[0] = '\0';
	if (r_speeds->integer && !(rn.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		switch (r_speeds->integer)
		{
			case 1:
				RB_StatsMessage(backend_msg, sizeof(backend_msg));

				Q_snprintfz(out, size,
					"%u fps\n"
					"%4u wpoly %4u leafs\n"
					"%5u\\%5u sverts %5u\\%5u stris\n"
					"%s",
					rf.fps.average,
					rf.stats.c_brush_polys, rf.stats.c_world_leafs,
					rf.stats.c_slices_verts, rf.stats.c_slices_verts_real, rf.stats.c_slices_elems / 3, rf.stats.c_slices_elems_real / 3,
					backend_msg
				);
				break;
			case 2:
			case 3:
				Q_snprintfz(out, size,
					"lvs: %5u  node: %5u\n"
					"polys\\ents: %5u\\%5i  draw: %5u\n",
					rf.stats.t_mark_leaves, rf.stats.t_world_node,
					rf.stats.t_add_polys, rf.stats.t_add_entities, rf.stats.t_draw_meshes
				);
				break;
			case 4:
			case 5:
				if (rf.debugSurface)
				{
					int numVerts = 0, numTris = 0;
					msurface_t *debugSurface = rf.debugSurface;

					Q_snprintfz(out, size,
						"%s type:%i sort:%i",
						debugSurface->shader->name, debugSurface->facetype, debugSurface->shader->sort);

					Q_strncatz(out, "\n", size);

					if (r_speeds->integer == 5 && debugSurface->drawSurf->vbo) {
						numVerts = debugSurface->drawSurf->vbo->numVerts;
						numTris = debugSurface->drawSurf->vbo->numElems / 3;
					}
					else if (debugSurface->mesh) {
						numVerts = debugSurface->mesh->numVerts;
						numTris = debugSurface->mesh->numElems;
					}

					if (numVerts) {
						Q_snprintfz(out + strlen(out), size - strlen(out),
							"verts: %5i tris: %5i", numVerts, numTris);
					}

					Q_strncatz(out, "\n", size);

					if (debugSurface->fog && debugSurface->fog->shader
						&& debugSurface->fog->shader != debugSurface->shader)
						Q_strncatz(out, debugSurface->fog->shader->name, size);
				}
				break;
			case 6:
				Q_snprintfz(out, size,
					"%.1f %.1f %.1f",
					rn.refdef.vieworg[0], rn.refdef.vieworg[1], rn.refdef.vieworg[2]
				);
				break;
			default:
				Q_snprintfz(out, size,
					"%u fps",
					rf.fps.average
				);
				break;
		}
	}

	out[size - 1] = '\0';
	return out;
}

/*
* R_GetDebugSurface
*/
const msurface_t *R_GetDebugSurface( void )
{
	msurface_t *debugSurface;

	ri.Mutex_Lock( rf.debugSurfaceLock );
	debugSurface = rf.debugSurface;
	ri.Mutex_Unlock( rf.debugSurfaceLock );

	return debugSurface;
}


/*
* R_CopyString
*/
char *R_CopyString_( const char *in, const char *filename, int fileline )
{
	char *out;

	out = ri.Mem_AllocExt( r_mempool, ( strlen( in ) + 1 ), 0, 1, filename, fileline );
	strcpy( out, in );

	return out;
}

/*
* R_LoadFile
*/
int R_LoadFile_( const char *path, int flags, void **buffer, const char *filename, int fileline )
{
	uint8_t *buf;
	unsigned int len;
	int fhandle;

	buf = NULL; // quiet compiler warning

	// look for it in the filesystem or pack files
	len = ri.FS_FOpenFile( path, &fhandle, FS_READ|flags );

	if( !fhandle )
	{
		if( buffer )
			*buffer = NULL;
		return -1;
	}

	if( !buffer )
	{
		ri.FS_FCloseFile( fhandle );
		return len;
	}

	buf = ( uint8_t *)ri.Mem_AllocExt( r_mempool, len + 1, 16, 0, filename, fileline );
	buf[len] = 0;
	*buffer = buf;

	ri.FS_Read( buf, len, fhandle );
	ri.FS_FCloseFile( fhandle );

	return len;
}

/*
* R_FreeFile
*/
void R_FreeFile_( void *buffer, const char *filename, int fileline )
{
	ri.Mem_Free( buffer, filename, fileline );
}
