#include "r_renderer_stub.h"
#include "r_local.h"

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

void initRendererGL() {
	R_EndFrame = R_EndFrame_GL;
}
