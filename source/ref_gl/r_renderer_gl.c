#include "r_glimp.h"
#include "r_local.h"
#include "r_renderer_api.h"

static void R_SetupGL( void );
static void R_EndGL( void );
static void R_BindRefInstFBO( void );

#define REFINST_STACK_SIZE 64
static refinst_t riStack[REFINST_STACK_SIZE];
static unsigned int riStackSize;

/*
 * Set scissor region for 2D drawing.
 * x and y represent the top left corner of the region/rectangle.
 */
void R_Scissor_GL( int x, int y, int w, int h )
{
	RB_Scissor( x, y, w, h );
}

void R_GetScissor_GL( int *x, int *y, int *w, int *h )
{
	RB_GetScissor( x, y, w, h );
}

void R_ResetScissor_GL( void )
{
	RB_Scissor( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight );
}

static void R_BindFrameBufferObject_GL( int object )
{
	int width, height;

	RFB_GetObjectSize( object, &width, &height );

	rf.frameBufferWidth = width;
	rf.frameBufferHeight = height;

	RB_BindFrameBufferObject( object );

	RB_Viewport( rn.viewport[0], rn.viewport[1], rn.viewport[2], rn.viewport[3] );
	RB_Scissor( rn.scissor[0], rn.scissor[1], rn.scissor[2], rn.scissor[3] );
}

void R_DrawStretchQuick_GL( int x, int y, int w, int h, float s1, float t1, float s2, float t2, const vec4_t color, int program_type, image_t *image, int blendMask )
{
	static char *s_name = "$builtinimage";
	static shaderpass_t p;
	static shader_t s;
	static float rgba[4];

	s.vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT;
	s.sort = SHADER_SORT_NEAREST;
	s.numpasses = 1;
	s.name = s_name;
	s.passes = &p;

	Vector4Copy( color, rgba );
	p.rgbgen.type = RGB_GEN_CONST;
	p.rgbgen.args = rgba;
	p.alphagen.type = ALPHA_GEN_CONST;
	p.alphagen.args = &rgba[3];
	p.tcgen = TC_GEN_BASE;
	p.images[0] = image;
	p.flags = blendMask;
	p.program_type = program_type;

	R_DrawRotatedStretchPic( x, y, w, h, s1, t1, s2, t2, 0, color, &s );

	RB_FlushDynamicMeshes();
}

static void R_BlitTextureToScrFbo( const refdef_t *fd, image_t *image, int dstFbo, int program_type, const vec4_t color, int blendMask, int numShaderImages, image_t **shaderImages )
{
	int x, y;
	int w, h, fw, fh;
	static char s_name[] = "$builtinpostprocessing";
	static shaderpass_t p;
	static shader_t s;
	int i;
	static tcmod_t tcmod;
	mat4_t m;

	assert( rsh.postProcessingVBO );

	// blit + flip using a static mesh to avoid redundant buffer uploads
	// (also using custom PP effects like FXAA with the stream VBO causes
	// Adreno to mark the VBO as "slow" (due to some weird bug)
	// for the rest of the frame and drop FPS to 10-20).
	RB_FlushDynamicMeshes();

	RB_BindFrameBufferObject( dstFbo );

	if( !dstFbo ) {
		// default framebuffer
		// set the viewport to full resolution
		// but keep the scissoring region
		x = fd->x;
		y = fd->y;
		w = fw = fd->width;
		h = fh = fd->height;
		RB_Viewport( 0, 0, rsh.width, rsh.height );
		RB_Scissor( rn.scissor[0], rn.scissor[1], rn.scissor[2], rn.scissor[3] );
	} else {
		// aux framebuffer
		// set the viewport to full resolution of the framebuffer (without the NPOT padding if there's one)
		// draw quad on the whole framebuffer texture
		// set scissor to default framebuffer resolution
		image_t *cb = RFB_GetObjectTextureAttachment( dstFbo, false );
		x = 0;
		y = 0;
		w = fw = rf.frameBufferWidth;
		h = fh = rf.frameBufferHeight;
		if( cb ) {
			fw = cb->upload_width;
			fh = cb->upload_height;
		}
		RB_Viewport( 0, 0, w, h );
		RB_Scissor( 0, 0, rsh.width, rsh.height );
	}

	s.vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT;
	s.sort = SHADER_SORT_NEAREST;
	s.numpasses = 1;
	s.name = s_name;
	s.passes = &p;

	p.rgbgen.type = RGB_GEN_IDENTITY;
	p.alphagen.type = ALPHA_GEN_IDENTITY;
	p.tcgen = TC_GEN_NONE;
	p.images[0] = image;
	for( i = 0; i < numShaderImages; i++ )
		p.images[i + 1] = shaderImages[i];
	p.flags = blendMask;
	p.program_type = program_type;

	if( !dstFbo ) {
		tcmod.type = TC_MOD_TRANSFORM;
		tcmod.args[0] = (float)( w ) / (float)( image->upload_width );
		tcmod.args[1] = (float)( h ) / (float)( image->upload_height );
		tcmod.args[4] = (float)( x ) / (float)( image->upload_width );
		tcmod.args[5] = (float)( image->upload_height - h - y ) / (float)( image->upload_height );
		p.numtcmods = 1;
		p.tcmods = &tcmod;
	} else {
		p.numtcmods = 0;
	}

	Matrix4_Identity( m );
	Matrix4_Scale2D( m, fw, fh );
	Matrix4_Translate2D( m, x, y );
	RB_LoadObjectMatrix( m );

	RB_BindShader( NULL, &s, NULL );
	RB_BindVBO( rsh.postProcessingVBO->index, GL_TRIANGLES );
	RB_DrawElements( 0, 4, 0, 6, 0, 0, 0, 0 );

	RB_LoadObjectMatrix( mat4x4_identity );

	// restore 2D viewport and scissor
	RB_Viewport( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight );
	RB_Scissor( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight );
}

/*
 * Note that this sets the viewport to size of the active framebuffer.
 */
static void R_Set2DMode_GL( bool enable )
{
	int width, height;

	width = rf.frameBufferWidth;
	height = rf.frameBufferHeight;

	if( rf.in2D == true && enable == true && width == rf.width2D && height == rf.height2D ) {
		return;
	} else if( rf.in2D == false && enable == false ) {
		return;
	}

	rf.in2D = enable;

	if( enable ) {
		rf.width2D = width;
		rf.height2D = height;

		Matrix4_OrthogonalProjection( 0, width, height, 0, -99999, 99999, rn.projectionMatrix );
		Matrix4_Copy( mat4x4_identity, rn.modelviewMatrix );
		Matrix4_Copy( rn.projectionMatrix, rn.cameraProjectionMatrix );

		// set 2D virtual screen size
		RB_Scissor( 0, 0, width, height );
		RB_Viewport( 0, 0, width, height );

		RB_LoadProjectionMatrix( rn.projectionMatrix );
		RB_LoadCameraMatrix( mat4x4_identity );
		RB_LoadObjectMatrix( mat4x4_identity );

		RB_SetShaderStateMask( ~0, GLSTATE_NO_DEPTH_TEST );

		RB_SetRenderFlags( 0 );
	} else {
		// render previously batched 2D geometry, if any
		RB_FlushDynamicMeshes();

		RB_SetShaderStateMask( ~0, 0 );
	}
}
static void R_DataSync_GL( void )
{
	if( rf.dataSync ) {
		if( glConfig.multithreading ) {
			// synchronize data we might have uploaded this frame between the threads
			// FIXME: only call this when absolutely necessary
			qglFinish();
		}
		rf.dataSync = false;
	}
}

static void R_SetGamma_GL( float gamma )
{
	int i, v;
	double invGamma, div;
	unsigned short gammaRamp[3 * GAMMARAMP_STRIDE];

	if( !glConfig.hwGamma )
		return;

	invGamma = 1.0 / bound( 0.5, gamma, 3.0 );
	div = (double)( 1 << 0 ) / ( glConfig.gammaRampSize - 0.5 );

	for( i = 0; i < glConfig.gammaRampSize; i++ ) {
		v = (int)( 65535.0 * pow( ( (double)i + 0.5 ) * div, invGamma ) + 0.5 );
		gammaRamp[i] = gammaRamp[i + GAMMARAMP_STRIDE] = gammaRamp[i + 2 * GAMMARAMP_STRIDE] = ( (unsigned short)bound( 0, v, 65535 ) );
	}

	GLimp_SetGammaRamp( GAMMARAMP_STRIDE, glConfig.gammaRampSize, gammaRamp );
}

static void R_SetupGL( void )
{
	RB_Scissor( rn.scissor[0], rn.scissor[1], rn.scissor[2], rn.scissor[3] );
	RB_Viewport( rn.viewport[0], rn.viewport[1], rn.viewport[2], rn.viewport[3] );

	if( rn.renderFlags & RF_CLIPPLANE ) {
		cplane_t *p = &rn.clipPlane;
		Matrix4_ObliqueNearClipping( p->normal, -p->dist, rn.cameraMatrix, rn.projectionMatrix );
	}

	RB_SetZClip( Z_NEAR, rn.farClip );

	RB_SetCamera( rn.viewOrigin, rn.viewAxis );

	RB_SetLightParams( rn.refdef.minLight, rn.refdef.rdflags & RDF_NOWORLDMODEL ? true : false );

	RB_SetRenderFlags( rn.renderFlags );

	RB_LoadProjectionMatrix( rn.projectionMatrix );

	RB_LoadCameraMatrix( rn.cameraMatrix );

	RB_LoadObjectMatrix( mat4x4_identity );

	if( rn.renderFlags & RF_FLIPFRONTFACE )
		RB_FlipFrontFace();

	if( ( rn.renderFlags & RF_SHADOWMAPVIEW ) && glConfig.ext.shadow )
		RB_SetShaderStateMask( ~0, GLSTATE_NO_COLORWRITE );
}

static void R_EndGL( void )
{
	if( ( rn.renderFlags & RF_SHADOWMAPVIEW ) && glConfig.ext.shadow )
		RB_SetShaderStateMask( ~0, 0 );

	if( rn.renderFlags & RF_FLIPFRONTFACE )
		RB_FlipFrontFace();
}

static void R_BindRefInstFBO( void )
{
	int fbo;

	if( rn.fbColorAttachment ) {
		fbo = rn.fbColorAttachment->fbo;
	} else if( rn.fbDepthAttachment ) {
		fbo = rn.fbDepthAttachment->fbo;
	} else {
		fbo = 0;
	}

	R_BindFrameBufferObject( fbo );
}

void R_DeferDataSync_GL( void )
{
	if( rsh.registrationOpen )
		return;

	rf.dataSync = true;
	qglFlush();
	RB_FlushTextureCache();
}

void R_ClearRefInstStack_GL( void )
{
	riStackSize = 0;
}

bool R_PushRefInst_GL( void )
{
	if( riStackSize == REFINST_STACK_SIZE ) {
		return false;
	}
	riStack[riStackSize++] = rn;
	R_EndGL();
	return true;
}

void R_PopRefInst_GL( void )
{
	if( !riStackSize ) {
		return;
	}

	rn = riStack[--riStackSize];
	R_BindRefInstFBO();

	R_SetupGL();
}
static void R_Finish_GL( void )
{
	qglFinish();
}

static void R_Flush_GL( void )
{
	qglFlush();
}

static void R_PolyBlend( void )
{
	if( !r_polyblend->integer )
		return;
	if( rsc.refdef.blend[3] < 0.01f )
		return;

	R_Set2DMode( true );
	R_DrawStretchPic( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight, 0, 0, 1, 1, rsc.refdef.blend, rsh.whiteShader );
	RB_FlushDynamicMeshes();
}

static void R_ApplyBrightness( void )
{
	float c;
	vec4_t color;

	c = r_brightness->value;
	if( c < 0.005 )
		return;
	else if( c > 1.0 )
		c = 1.0;

	color[0] = color[1] = color[2] = c, color[3] = 1;

	R_Set2DMode( true );
	R_DrawStretchQuick( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight, 0, 0, 1, 1, color, GLSL_PROGRAM_TYPE_NONE, rsh.whiteTexture, GLSTATE_SRCBLEND_ONE | GLSTATE_DSTBLEND_ONE );
}

static void R_EndFrame_GL( void )
{
	// render previously batched 2D geometry, if any
	RB_FlushDynamicMeshes();

	R_PolyBlend();

	R_ApplyBrightness();

	// reset the 2D state so that the mode will be
	// properly set back again in R_BeginFrame
	R_Set2DMode( false );

	RB_EndFrame();

	GLimp_EndFrame();

	assert( qglGetError() == GL_NO_ERROR );
}
/*
 * R_RenderDebugSurface
 */
static void R_RenderDebugSurface_GL( const refdef_t *fd )
{
	rtrace_t tr;
	vec3_t forward;
	vec3_t start, end;
	msurface_t *debugSurf = NULL;

	if( fd->rdflags & RDF_NOWORLDMODEL )
		return;

	if( r_speeds->integer == 4 || r_speeds->integer == 5 ) {
		msurface_t *surf = NULL;

		VectorCopy( &fd->viewaxis[AXIS_FORWARD], forward );
		VectorCopy( fd->vieworg, start );
		VectorMA( start, 4096, forward, end );

		surf = R_TraceLine( &tr, start, end, 0 );
		if( surf && surf->drawSurf && !r_showtris->integer ) {
			R_ClearDrawList( rn.meshlist );

			R_ClearDrawList( rn.portalmasklist );

			if( R_AddSurfToDrawList( rn.meshlist, R_NUM2ENT( tr.ent ), NULL, surf->shader, 0, 0, NULL, surf->drawSurf ) ) {
				if( rn.refdef.rdflags & RDF_FLIPPED )
					RB_FlipFrontFace();

				if( r_speeds->integer == 5 ) {
					// VBO debug mode
					R_AddVBOSlice( surf->drawSurf - rsh.worldBrushModel->drawSurfaces, surf->drawSurf->numVerts, surf->drawSurf->numElems, 0, 0 );
				} else {
					// classic mode (showtris for individual surface)
					R_AddVBOSlice( surf->drawSurf - rsh.worldBrushModel->drawSurfaces, surf->mesh->numVerts, surf->mesh->numElems, surf->firstDrawSurfVert, surf->firstDrawSurfElem );
				}

				R_DrawOutlinedSurfaces( rn.meshlist );

				if( rn.refdef.rdflags & RDF_FLIPPED )
					RB_FlipFrontFace();

				debugSurf = surf;
			}
		}
	}

	ri.Mutex_Lock( rf.debugSurfaceLock );
	rf.debugSurface = debugSurf;
	ri.Mutex_Unlock( rf.debugSurfaceLock );
}

void R_BeginFrame_GL( float cameraSeparation, bool forceClear, bool forceVsync )
{
	unsigned int time = ri.Sys_Milliseconds();

	GLimp_BeginFrame();

	RB_BeginFrame();

#ifndef GL_ES_VERSION_2_0
	if( cameraSeparation && ( !glConfig.stereoEnabled || !R_IsRenderingToScreen() ) )
		cameraSeparation = 0;

	if( rf.cameraSeparation != cameraSeparation ) {
		rf.cameraSeparation = cameraSeparation;
		if( cameraSeparation < 0 )
			qglDrawBuffer( GL_BACK_LEFT );
		else if( cameraSeparation > 0 )
			qglDrawBuffer( GL_BACK_RIGHT );
		else
			qglDrawBuffer( GL_BACK );
	}
#endif

	// draw buffer stuff
	if( rf.newDrawBuffer ) {
		rf.newDrawBuffer = false;

#ifndef GL_ES_VERSION_2_0
		if( cameraSeparation == 0 || !glConfig.stereoEnabled ) {
			if( Q_stricmp( rf.drawBuffer, "GL_FRONT" ) == 0 )
				qglDrawBuffer( GL_FRONT );
			else
				qglDrawBuffer( GL_BACK );
		}
#endif
	}

	if( forceClear ) {
		RB_Clear( GL_COLOR_BUFFER_BIT, 0, 0, 0, 1 );
	}

	// set swap interval (vertical synchronization)
	rf.swapInterval = R_SetSwapInterval( ( r_swapinterval->integer || forceVsync ) ? 1 : 0, rf.swapInterval );

	memset( &rf.stats, 0, sizeof( rf.stats ) );

	// update fps meter
	rf.fps.count++;
	rf.fps.time = time;
	if( rf.fps.time - rf.fps.oldTime >= 250 ) {
		rf.fps.average = time - rf.fps.oldTime;
		rf.fps.average = 1000.0f * ( rf.fps.count - rf.fps.oldCount ) / (float)rf.fps.average + 0.5f;
		rf.fps.oldTime = time;
		rf.fps.oldCount = rf.fps.count;
	}

	R_Set2DMode( true );
}
int R_SetSwapInterval_GL( int swapInterval, int oldSwapInterval )
{
	if( glConfig.stereoEnabled )
		return oldSwapInterval;

	clamp_low( swapInterval, r_swapinterval_min->integer );
	if( swapInterval != oldSwapInterval ) {
		GLimp_SetSwapInterval( swapInterval );
    }
    return swapInterval;
}

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

static bool R_AddNullSurfToDrawList( const entity_t *e )
{
	static drawSurfaceType_t nullDrawSurf = ST_NULLMODEL;
	if( !R_AddSurfToDrawList( rn.meshlist, e, R_FogForSphere( e->origin, 0.1f ), 
		rsh.whiteShader, 0, 0, NULL, &nullDrawSurf ) ) {
		return false;
	}

	return true;
}

static bool R_AddSpriteToDrawList( const entity_t *e )
{
	static drawSurfaceType_t spriteDrawSurf = ST_SPRITE;
	float dist;

	if( e->flags & RF_NOSHADOW )
	{
		if( rn.renderFlags & RF_SHADOWMAPVIEW )
			return false;
	}

	if( e->radius <= 0 || e->customShader == NULL || e->scale <= 0 ) {
		return false;
	}

	dist =
		( e->origin[0] - rn.refdef.vieworg[0] ) * rn.viewAxis[AXIS_FORWARD+0] +
		( e->origin[1] - rn.refdef.vieworg[1] ) * rn.viewAxis[AXIS_FORWARD+1] +
		( e->origin[2] - rn.refdef.vieworg[2] ) * rn.viewAxis[AXIS_FORWARD+2];
	if( dist <= 0 )
		return false; // cull it because we don't want to sort unneeded things

	if( !R_AddSurfToDrawList( rn.meshlist, e, R_FogForSphere( e->origin, e->radius ), 
		e->customShader, dist, 0, NULL, &spriteDrawSurf ) ) {
		return false;
	}

	return true;
}

static void R_DrawEntities( void )
{
	unsigned int i;
	entity_t *e;
	bool shadowmap = ( ( rn.renderFlags & RF_SHADOWMAPVIEW ) != 0 );
	bool culled = true;

	if( rn.renderFlags & RF_ENVVIEW )
	{
		for( i = 0; i < rsc.numBmodelEntities; i++ )
		{
			e = rsc.bmodelEntities[i];
			if( !r_lerpmodels->integer )
				e->backlerp = 0;
			e->outlineHeight = rsc.worldent->outlineHeight;
			Vector4Copy( rsc.worldent->outlineRGBA, e->outlineColor );
			R_AddBrushModelToDrawList( e );
		}
		return;
	}

	for( i = rsc.numLocalEntities; i < rsc.numEntities; i++ )
	{
		e = R_NUM2ENT(i);
		culled = true;

		if( !r_lerpmodels->integer )
			e->backlerp = 0;

		switch( e->rtype )
		{
		case RT_MODEL:
			if( !e->model ) {
				R_AddNullSurfToDrawList( e );
				continue;
			}

			switch( e->model->type )
			{
			case mod_alias:
				culled = ! R_AddAliasModelToDrawList( e );
				break;
			case mod_skeletal:
				culled = ! R_AddSkeletalModelToDrawList( e );
				break;
			case mod_brush:
				e->outlineHeight = rsc.worldent->outlineHeight;
				Vector4Copy( rsc.worldent->outlineRGBA, e->outlineColor );
				culled = ! R_AddBrushModelToDrawList( e );
			default:
				break;
			}
			break;
		case RT_SPRITE:
			culled = ! R_AddSpriteToDrawList( e );
			break;
		default:
			break;
		}

		if( shadowmap && !culled ) {
			if( rsc.entShadowGroups[i] != rn.shadowGroup->id ||
				r_shadows_self_shadow->integer ) {
				// not from the casting group, mark as shadowed
				rsc.entShadowBits[i] |= rn.shadowGroup->bit;
			}
		}
	}
}

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

void R_RenderView_GL( const refdef_t *fd )
{
	int msec = 0;
	bool shadowMap = rn.renderFlags & RF_SHADOWMAPVIEW ? true : false;

	rn.refdef = *fd;
	rn.numVisSurfaces = 0;

	// load view matrices with default far clip value
	R_SetupViewMatrices();

	rn.fog_eye = NULL;

	rn.shadowBits = 0;
	rn.dlightBits = 0;
	
	rn.numPortalSurfaces = 0;
	rn.numDepthPortalSurfaces = 0;
	rn.skyportalSurface = NULL;

	ClearBounds( rn.visMins, rn.visMaxs );

	R_ClearSky();

	if( r_novis->integer ) {
		rn.renderFlags |= RF_NOVIS;
	}

	if( r_lightmap->integer ) {
		rn.renderFlags |= RF_LIGHTMAP;
	}

	if( r_drawflat->integer ) {
		rn.renderFlags |= RF_DRAWFLAT;
	}

	R_ClearDrawList( rn.meshlist );

	R_ClearDrawList( rn.portalmasklist );

	if( !rsh.worldModel && !( rn.refdef.rdflags & RDF_NOWORLDMODEL ) )
		return;

	R_SetupFrame();

	R_SetupFrustum( &rn.refdef, rn.farClip, rn.frustum );

	// we know the initial farclip at this point after determining visible world leafs
	// R_DrawEntities can make adjustments as well

	if( !shadowMap ) {
		if( r_speeds->integer )
			msec = ri.Sys_Milliseconds();
		R_MarkLeaves();
		if( r_speeds->integer )
			rf.stats.t_mark_leaves += ( ri.Sys_Milliseconds() - msec );

		if( ! ( rn.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
			R_DrawWorld();

			if( !rn.numVisSurfaces ) {
				// no world surfaces visible
				return;
			}

			rn.fog_eye = R_FogForSphere( rn.viewOrigin, 0.5 );
		}

		R_DrawCoronas();

		if( r_speeds->integer )
			msec = ri.Sys_Milliseconds();
		R_DrawPolys();
		if( r_speeds->integer )
			rf.stats.t_add_polys += ( ri.Sys_Milliseconds() - msec );
	}

	if( r_speeds->integer )
		msec = ri.Sys_Milliseconds();
	R_DrawEntities();
	if( r_speeds->integer )
		rf.stats.t_add_entities += ( ri.Sys_Milliseconds() - msec );

	if( !shadowMap ) {
		// now set  the real far clip value and reload view matrices
		R_SetFarClip();

		R_SetupViewMatrices();

		// render to depth textures, mark shadowed entities and surfaces
		R_DrawShadowmaps();
	}

	R_SortDrawList( rn.meshlist );

	R_BindRefInstFBO();

	R_SetupGL();

	R_DrawPortals();

	if( r_portalonly->integer && !( rn.renderFlags & ( RF_MIRRORVIEW|RF_PORTALVIEW ) ) )
		return;

	R_Clear( ~0 );

	if( r_speeds->integer )
		msec = ri.Sys_Milliseconds();
	R_DrawSurfaces( rn.meshlist );
	if( r_speeds->integer )
		rf.stats.t_draw_meshes += ( ri.Sys_Milliseconds() - msec );

	rf.stats.c_slices_verts += rn.meshlist->numSliceVerts;
	rf.stats.c_slices_verts_real += rn.meshlist->numSliceVertsReal;

	rf.stats.c_slices_elems += rn.meshlist->numSliceElems;
	rf.stats.c_slices_elems_real += rn.meshlist->numSliceElemsReal;

	if( r_showtris->integer )
		R_DrawOutlinedSurfaces( rn.meshlist );

	R_TransformForWorld();

	R_EndGL();
}


void initRendererGL()
{
	R_SetGamma = R_SetGamma_GL;
	R_Set2DMode = R_Set2DMode_GL;
	R_Scissor = R_Scissor_GL;
	R_GetScissor = R_GetScissor_GL;
	R_ResetScissor = R_ResetScissor_GL;
	R_DrawStretchQuick = R_DrawStretchQuick_GL;
	R_BindFrameBufferObject = R_BindFrameBufferObject_GL;
	R_ClearRefInstStack = R_ClearRefInstStack_GL;
	R_PushRefInst = R_PushRefInst_GL;
	R_PopRefInst = R_PopRefInst_GL;
	R_DeferDataSync = R_DeferDataSync_GL;
	R_DataSync = R_DataSync_GL;
	R_Finish = R_Finish_GL;
	R_Flush = R_Flush_GL;
	R_EndFrame = R_EndFrame_GL;
	R_RenderDebugSurface = R_RenderDebugSurface_GL;
	R_BeginFrame = R_BeginFrame_GL;
	R_SetSwapInterval = R_SetSwapInterval_GL;
	R_RenderView = R_RenderView_GL;
}
