/*
* R_SetGamma
*/
#include "r_local.h"
#include "r_renderer_api.h"
#include "r_glimp.h"

static void R_SetupGL( void );
static void R_EndGL( void );
static void R_BindRefInstFBO( void );

/*
*
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

void R_DrawStretchQuick_GL( int x, int y, int w, int h, float s1, float t1, float s2, float t2, 
	const vec4_t color, int program_type, image_t *image, int blendMask )
{
	static char *s_name = "$builtinimage";
	static shaderpass_t p;
	static shader_t s;
	static float rgba[4];

	s.vattribs = VATTRIB_POSITION_BIT|VATTRIB_TEXCOORDS_BIT;
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

static void R_BlitTextureToScrFbo( const refdef_t *fd, image_t *image, int dstFbo, 
	int program_type, const vec4_t color, int blendMask, int numShaderImages, image_t **shaderImages )
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
	}
	else {
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

	s.vattribs = VATTRIB_POSITION_BIT|VATTRIB_TEXCOORDS_BIT;
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
		tcmod.args[0] = ( float )( w ) / ( float )( image->upload_width );
		tcmod.args[1] = ( float )( h ) / ( float )( image->upload_height );
		tcmod.args[4] = ( float )( x ) / ( float )( image->upload_width );
		tcmod.args[5] = ( float )( image->upload_height - h - y ) / ( float )( image->upload_height );
		p.numtcmods = 1;
		p.tcmods = &tcmod;
	}
	else {
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

	if( enable )
	{
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
	}
	else
	{
		// render previously batched 2D geometry, if any
		RB_FlushDynamicMeshes();

		RB_SetShaderStateMask( ~0, 0 );
	}
}

static void R_SetGamma_GL( float gamma )
{
	int i, v;
	double invGamma, div;
	unsigned short gammaRamp[3*GAMMARAMP_STRIDE];

	if( !glConfig.hwGamma )
		return;

	invGamma = 1.0 / bound( 0.5, gamma, 3.0 );
	div = (double)( 1 << 0 ) / (glConfig.gammaRampSize - 0.5);

	for( i = 0; i < glConfig.gammaRampSize; i++ )
	{
		v = ( int )( 65535.0 * pow( ( (double)i + 0.5 ) * div, invGamma ) + 0.5 );
		gammaRamp[i] = gammaRamp[i + GAMMARAMP_STRIDE] = gammaRamp[i + 2*GAMMARAMP_STRIDE] = ( ( unsigned short )bound( 0, v, 65535 ) );
	}

	GLimp_SetGammaRamp( GAMMARAMP_STRIDE, glConfig.gammaRampSize, gammaRamp );
}

static void R_SetupGL( void )
{
	RB_Scissor( rn.scissor[0], rn.scissor[1], rn.scissor[2], rn.scissor[3] );
	RB_Viewport( rn.viewport[0], rn.viewport[1], rn.viewport[2], rn.viewport[3] );

	if( rn.renderFlags & RF_CLIPPLANE )
	{
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
	}
	else if( rn.fbDepthAttachment ) {
		fbo = rn.fbDepthAttachment->fbo;
	}
	else {
		fbo = 0;
	}

	R_BindFrameBufferObject( fbo );
}

void initRendererGL() {
  R_SetGamma = R_SetGamma_GL;
  R_Set2DMode = R_Set2DMode_GL;
  R_Scissor = R_Scissor_GL;
  R_GetScissor = R_GetScissor_GL;
  R_ResetScissor = R_ResetScissor_GL;
  R_DrawStretchQuick = R_DrawStretchQuick_GL;
  R_BindFrameBufferObject = R_BindFrameBufferObject_GL;
}
