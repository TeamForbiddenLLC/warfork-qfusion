/*
Copyright (C) 2007 Victor Luchits

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
#include "r_glimp.h"
#include "r_renderer_api.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

// r_register.c
#include "r_local.h"
#include "../qalgo/hash.h"
#include "r_resource_upload.h"
#include "r_nri.h"
#include "r_shared.h"

nri_t r_nri;
glconfig_t glConfig;
backend_api_t r_backend_api;
r_shared_t rsh;
mempool_t *r_mempool;

cvar_t *r_maxfps;
cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_speeds;
cvar_t *r_drawelements;
cvar_t *r_fullbright;
cvar_t *r_lightmap;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lerpmodels;
cvar_t *r_mapoverbrightbits;
cvar_t *r_brightness;

cvar_t *r_dynamiclight;
cvar_t *r_coronascale;
cvar_t *r_detailtextures;
cvar_t *r_subdivisions;
cvar_t *r_showtris;
cvar_t *r_draworder;
cvar_t *r_leafvis;

cvar_t *r_fastsky;
cvar_t *r_portalonly;
cvar_t *r_portalmaps;
cvar_t *r_portalmaps_maxtexsize;

cvar_t *r_lighting_deluxemapping;
cvar_t *r_lighting_specular;
cvar_t *r_lighting_glossintensity;
cvar_t *r_lighting_glossexponent;
cvar_t *r_lighting_ambientscale;
cvar_t *r_lighting_directedscale;
cvar_t *r_lighting_packlightmaps;
cvar_t *r_lighting_maxlmblocksize;
cvar_t *r_lighting_vertexlight;
cvar_t *r_lighting_maxglsldlights;
cvar_t *r_lighting_grayscale;

cvar_t *r_offsetmapping;
cvar_t *r_offsetmapping_scale;
cvar_t *r_offsetmapping_reliefmapping;

cvar_t *r_shadows;
cvar_t *r_shadows_alpha;
cvar_t *r_shadows_projection_distance;
cvar_t *r_shadows_maxtexsize;
cvar_t *r_shadows_pcf;
cvar_t *r_shadows_self_shadow;
cvar_t *r_shadows_dither;

cvar_t *r_outlines_world;
cvar_t *r_outlines_scale;
cvar_t *r_outlines_cutoff;

cvar_t *r_soft_particles;
cvar_t *r_soft_particles_scale;

cvar_t *r_fxaa;

cvar_t *r_lodbias;
cvar_t *r_lodscale;

cvar_t *r_stencilbits;
cvar_t *r_gamma;
cvar_t *r_texturebits;
cvar_t *r_texturemode;
cvar_t *r_texturefilter;
cvar_t *r_texturecompression;
cvar_t *r_picmip;
cvar_t *r_skymip;
cvar_t *r_nobind;
cvar_t *r_polyblend;
cvar_t *r_lockpvs;
cvar_t *r_screenshot_fmtstr;
cvar_t *r_screenshot_format;
cvar_t *r_swapinterval;
cvar_t *r_swapinterval_min;

cvar_t *r_temp1;

cvar_t *r_drawflat;
cvar_t *r_wallcolor;
cvar_t *r_floorcolor;

cvar_t *r_usenotexture;

cvar_t *r_maxglslbones;

cvar_t *gl_drawbuffer;
cvar_t *gl_driver;
cvar_t *gl_cull;
cvar_t *r_multithreading;

static bool	r_verbose;
static bool	r_postinit;

static void R_GfxInfo_f( void );

static void R_InitVolatileAssets( void );
static void R_DestroyVolatileAssets( void );

/*
* R_FillStartupBackgroundColor
*
* Fills the window with a color during the initialization.
*/
static void R_FillStartupBackgroundColor( float r, float g, float b )
{
	qglClearColor( r, g, b, 1.0 );
	GLimp_BeginFrame();
#ifndef GL_ES_VERSION_2_0
	if( glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_LEFT );
		qglClear( GL_COLOR_BUFFER_BIT );
		qglDrawBuffer( GL_BACK_RIGHT );
		qglClear( GL_COLOR_BUFFER_BIT );
		qglDrawBuffer( GL_BACK );
	}
#endif
	qglClear( GL_COLOR_BUFFER_BIT );
	qglFinish();
	GLimp_EndFrame();
}

static void R_Register( const char *screenshotsPrefix )
{
	char tmp[128];
	const qgl_driverinfo_t *driver;

    r_maxfps = ri.Cvar_Get( "r_maxfps", "250", CVAR_ARCHIVE );
	r_norefresh = ri.Cvar_Get( "r_norefresh", "0", 0 );
	r_fullbright = ri.Cvar_Get( "r_fullbright", "0", CVAR_LATCH_VIDEO );
	r_lightmap = ri.Cvar_Get( "r_lightmap", "0", 0 );
	r_drawentities = ri.Cvar_Get( "r_drawentities", "1", CVAR_CHEAT );
	r_drawworld = ri.Cvar_Get( "r_drawworld", "1", CVAR_CHEAT );
	r_novis = ri.Cvar_Get( "r_novis", "0", 0 );
	r_nocull = ri.Cvar_Get( "r_nocull", "0", 0 );
	r_lerpmodels = ri.Cvar_Get( "r_lerpmodels", "1", 0 );
	r_speeds = ri.Cvar_Get( "r_speeds", "0", 0 );
	r_drawelements = ri.Cvar_Get( "r_drawelements", "1", 0 );
	r_showtris = ri.Cvar_Get( "r_showtris", "0", CVAR_CHEAT );
	r_leafvis = ri.Cvar_Get( "r_leafvis", "0", CVAR_CHEAT );
	r_lockpvs = ri.Cvar_Get( "r_lockpvs", "0", CVAR_CHEAT );
	r_nobind = ri.Cvar_Get( "r_nobind", "0", 0 );
	r_picmip = ri.Cvar_Get( "r_picmip", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_skymip = ri.Cvar_Get( "r_skymip", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_polyblend = ri.Cvar_Get( "r_polyblend", "1", 0 );

	r_mapoverbrightbits = ri.Cvar_Get( "r_mapoverbrightbits", "2", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_brightness = ri.Cvar_Get( "r_brightness", "0", CVAR_ARCHIVE );

	r_detailtextures = ri.Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE );

	r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_coronascale = ri.Cvar_Get( "r_coronascale", "0.4", 0 );
	r_subdivisions = ri.Cvar_Get( "r_subdivisions", STR_TOSTR( SUBDIVISIONS_DEFAULT ), CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_draworder = ri.Cvar_Get( "r_draworder", "0", CVAR_CHEAT );

	r_fastsky = ri.Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_portalonly = ri.Cvar_Get( "r_portalonly", "0", 0 );
	r_portalmaps = ri.Cvar_Get( "r_portalmaps", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_portalmaps_maxtexsize = ri.Cvar_Get( "r_portalmaps_maxtexsize", "1024", CVAR_ARCHIVE );

	r_lighting_deluxemapping = ri.Cvar_Get( "r_lighting_deluxemapping", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_specular = ri.Cvar_Get( "r_lighting_specular", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_glossintensity = ri.Cvar_Get( "r_lighting_glossintensity", "1.5", CVAR_ARCHIVE );
	r_lighting_glossexponent = ri.Cvar_Get( "r_lighting_glossexponent", "24", CVAR_ARCHIVE );
	r_lighting_ambientscale = ri.Cvar_Get( "r_lighting_ambientscale", "1", 0 );
	r_lighting_directedscale = ri.Cvar_Get( "r_lighting_directedscale", "1", 0 );

	r_lighting_packlightmaps = ri.Cvar_Get( "r_lighting_packlightmaps", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_maxlmblocksize = ri.Cvar_Get( "r_lighting_maxlmblocksize", "2048", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_vertexlight = ri.Cvar_Get( "r_lighting_vertexlight", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_maxglsldlights = ri.Cvar_Get( "r_lighting_maxglsldlights", "16", CVAR_ARCHIVE );
	r_lighting_grayscale = ri.Cvar_Get( "r_lighting_grayscale", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_offsetmapping = ri.Cvar_Get( "r_offsetmapping", "2", CVAR_ARCHIVE );
	r_offsetmapping_scale = ri.Cvar_Get( "r_offsetmapping_scale", "0.02", CVAR_ARCHIVE );
	r_offsetmapping_reliefmapping = ri.Cvar_Get( "r_offsetmapping_reliefmapping", "0", CVAR_ARCHIVE );

#ifdef CGAMEGETLIGHTORIGIN
	r_shadows = ri.Cvar_Get( "cg_shadows", "1", CVAR_ARCHIVE );
#else
	r_shadows = ri.Cvar_Get( "r_shadows", "0", CVAR_ARCHIVE );
#endif
	r_shadows_alpha = ri.Cvar_Get( "r_shadows_alpha", "0.7", CVAR_ARCHIVE );
	r_shadows_projection_distance = ri.Cvar_Get( "r_shadows_projection_distance", "64", CVAR_CHEAT );
	r_shadows_maxtexsize = ri.Cvar_Get( "r_shadows_maxtexsize", "64", CVAR_ARCHIVE );
	r_shadows_pcf = ri.Cvar_Get( "r_shadows_pcf", "1", CVAR_ARCHIVE );
	r_shadows_self_shadow = ri.Cvar_Get( "r_shadows_self_shadow", "0", CVAR_ARCHIVE );
	r_shadows_dither = ri.Cvar_Get( "r_shadows_dither", "0", CVAR_ARCHIVE );

	r_outlines_world = ri.Cvar_Get( "r_outlines_world", "1.8", CVAR_ARCHIVE );
	r_outlines_scale = ri.Cvar_Get( "r_outlines_scale", "1", CVAR_ARCHIVE );
	r_outlines_cutoff = ri.Cvar_Get( "r_outlines_cutoff", "712", CVAR_ARCHIVE );

	r_soft_particles = ri.Cvar_Get( "r_soft_particles", "1", CVAR_ARCHIVE );
	r_soft_particles_scale = ri.Cvar_Get( "r_soft_particles_scale", "0.02", CVAR_ARCHIVE );

	r_fxaa = ri.Cvar_Get( "r_fxaa", "1", CVAR_ARCHIVE );

	r_lodbias = ri.Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_lodscale = ri.Cvar_Get( "r_lodscale", "5.0", CVAR_ARCHIVE );

	r_gamma = ri.Cvar_Get( "r_gamma", "1.0", CVAR_ARCHIVE );
	r_texturebits = ri.Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH_VIDEO );
	r_texturemode = ri.Cvar_Get( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	r_texturefilter = ri.Cvar_Get( "r_texturefilter", "4", CVAR_ARCHIVE );
	r_texturecompression = ri.Cvar_Get( "r_texturecompression", "0", CVAR_ARCHIVE | CVAR_LATCH_VIDEO );
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_screenshot_fmtstr = ri.Cvar_Get( "r_screenshot_fmtstr", va_r( tmp, sizeof( tmp ), "%s%y%%m%%d_%H%M%%S", screenshotsPrefix ), CVAR_ARCHIVE );
	r_screenshot_format = ri.Cvar_Get( "r_screenshot_format", "1", CVAR_ARCHIVE );

#if defined(GLX_VERSION) && !defined(USE_SDL2)
	r_swapinterval = ri.Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
#else
	r_swapinterval = ri.Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE );
#endif
	r_swapinterval_min = ri.Cvar_Get( "r_swapinterval_min", "0", CVAR_READONLY ); // exposes vsync support to UI

	r_temp1 = ri.Cvar_Get( "r_temp1", "0", 0 );

	r_drawflat = ri.Cvar_Get( "r_drawflat", "0", CVAR_ARCHIVE );
	r_wallcolor = ri.Cvar_Get( "r_wallcolor", "255 255 255", CVAR_ARCHIVE );
	r_floorcolor = ri.Cvar_Get( "r_floorcolor", "255 153 0", CVAR_ARCHIVE );

	// make sure we rebuild our 3D texture after vid_restart
	r_wallcolor->modified = r_floorcolor->modified = true;

	r_maxfps->modified = true;

	// set to 1 to enable use of the checkerboard texture for missing world and model images
	r_usenotexture = ri.Cvar_Get( "r_usenotexture", "0", CVAR_ARCHIVE );

	r_maxglslbones = ri.Cvar_Get( "r_maxglslbones", STR_TOSTR( MAX_GLSL_UNIFORM_BONES ), CVAR_LATCH_VIDEO );

	r_multithreading = ri.Cvar_Get( "r_multithreading", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	gl_cull = ri.Cvar_Get( "gl_cull", "1", 0 );
	gl_drawbuffer = ri.Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );

	driver = QGL_GetDriverInfo();
	if( driver && driver->dllcvarname )
		gl_driver = ri.Cvar_Get( driver->dllcvarname, driver->dllname, CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	else
		gl_driver = NULL;

	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "shaderdump", R_ShaderDump_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "envshot", R_EnvShot_f );
	ri.Cmd_AddCommand( "modellist", Mod_Modellist_f );
	ri.Cmd_AddCommand( "gfxinfo", R_GfxInfo_f );
	ri.Cmd_AddCommand( "glslprogramlist", RP_ProgramList_f );
	ri.Cmd_AddCommand( "cinlist", R_CinList_f );
}

/*
 * R_GfxInfo_f
 */
static void R_GfxInfo_f( void )
{
	Com_Printf( "\n" );
	if( r_backend_api == BACKEND_OPENGL_LEGACY ) {
		GLimp_PrintConfig();

		Com_Printf( "\n" );
		Com_Printf( "Video memory information:\n" );

		char mem[12];
		if( glConfig.ext.gpu_memory_info ) {
			// NV
			qglGetIntegerv( GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, mem );
			Com_Printf( "total: %i MB\n", mem[0] >> 10 );

			qglGetIntegerv( GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, mem );
			Com_Printf( "dedicated: %i MB\n", mem[0] >> 10 );

			qglGetIntegerv( GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, mem );
			Com_Printf( "available: %i MB\n", mem[0] >> 10 );

			qglGetIntegerv( GPU_MEMORY_INFO_EVICTION_COUNT_NVX, mem );
			Com_Printf( "eviction count: %i MB\n", mem[0] >> 10 );

			qglGetIntegerv( GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, mem );
			Com_Printf( "totally evicted: %i MB\n", mem[0] >> 10 );
		} else if( glConfig.ext.meminfo ) {
			// ATI
			qglGetIntegerv( VBO_FREE_MEMORY_ATI, mem );
			qglGetIntegerv( TEXTURE_FREE_MEMORY_ATI, mem + 4 );
			qglGetIntegerv( RENDERBUFFER_FREE_MEMORY_ATI, mem + 8 );

			Com_Printf( "total memory free in the pool: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[0] >> 10, mem[4] >> 10, mem[8] >> 10 );
			Com_Printf( "largest available free block in the pool: (V:%i, Tex:%i, RBuf:%i) MB\n", mem[5] >> 10, mem[4] >> 10, mem[9] >> 10 );
			Com_Printf( "total auxiliary memory free: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[2] >> 10, mem[6] >> 10, mem[10] >> 10 );
			Com_Printf( "largest auxiliary free block: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[3] >> 10, mem[7] >> 10, mem[11] >> 10 );
		} else {
			Com_Printf( "not available\n" );
		}
	}
}

/*
* R_Init
*/
rserr_t R_Init( r_app_init_desc_t* desc)
{
	r_mempool = R_AllocPool( NULL, "Rendering Frontend" );
	r_verbose = desc->verbose;
	r_postinit = true;
	r_backend_api = desc->api;
	const char* applicationName = desc->applicationName;
	const char* screenshotPrefix = desc->screenshotPrefix;
	memset( &glConfig, 0, sizeof(glconfig_t) );
	memset( &rsh, 0, sizeof( rsh ) );
	memset( &rf, 0, sizeof( rf ) );

	if( !applicationName ) applicationName = "Qfusion";
	if( !screenshotPrefix ) screenshotPrefix = "";

	R_Register( screenshotPrefix );

	switch(r_backend_api) {
		case BACKEND_OPENGL_LEGACY: {
			qgl_initerr_t initerr;
			const char *dllname = NULL;
			const qgl_driverinfo_t *driver = QGL_GetDriverInfo();
			if( driver )
				dllname = driver->dllname;
		init_qgl:
			initRendererGL();
			initerr = QGL_Init( gl_driver ? gl_driver->string : dllname );
			if( initerr != qgl_initerr_ok ) {
				QGL_Shutdown();
				Com_Printf( "ref_gl::R_Init() - could not load \"%s\"\n", gl_driver ? gl_driver->string : dllname );

				if( ( initerr == qgl_initerr_invalid_driver ) && gl_driver && strcmp( gl_driver->string, dllname ) ) {
					ri.Cvar_ForceSet( gl_driver->name, dllname );
					goto init_qgl;
				}
				return rserr_invalid_driver;
			}

			// initialicvar_t OS-specific parts of OpenGL
			if( !GLimp_Init( applicationName, desc->hinstance, desc->wndProc, desc->parenthWnd, desc->iconResource, desc->iconXPM) ) {
				QGL_Shutdown();
				return rserr_unknown;
			}
			break;
		}
		case BACKEND_NRI_VULKAN:
		case BACKEND_NRI_METAL:
		case BACKEND_NRI_DX12: {
			initRendererNRI();
			if( !GLimp_Init( applicationName, desc->hinstance, desc->wndProc, desc->parenthWnd, desc->iconResource, desc->iconXPM) ) {
				return rserr_unknown;
			}
			nri_desc_t nriDesc = {};
			nriDesc.width = desc->width;
			nriDesc.height = desc->height;
			switch( desc->winType ) {
				case WINDOW_TYPE_WAYLAND:
					nriDesc.window.wayland.surface = desc->window.wayland.surface;
					nriDesc.window.wayland.display = desc->window.wayland.display;
					break;
				case WINDOW_TYPE_X11:
					nriDesc.window.x11.window = desc->window.x11.window;
					nriDesc.window.x11.dpy = desc->window.x11.dpy;
					break;
				case WINDOW_TYPE_WIN32:
					nriDesc.window.windows.hwnd = desc->window.win.hwnd;
					break;
				default:
					Com_Printf( "Unhandled window Type %d\n", desc->winType);
					assert(false);
					break;
			}
			nri_Init( &nriDesc );
			break;
		}
	}
	// FIXME: move this elsewhere?
	glConfig.startupColor = desc->startupColor;
	rf.applicationName = R_CopyString( applicationName );
	rf.screenshotPrefix = R_CopyString( screenshotPrefix );

	return rserr_ok;
}

/*
* R_PostInit
*/
static rserr_t R_PostInit( void )
{
	rsh.registrationSequence = 1;
	rsh.registrationOpen = false;
	rsh.worldModelSequence = 1;

	rf.swapInterval = -1;
	rf.speedsMsgLock = ri.Mutex_Create();
	rf.debugSurfaceLock = ri.Mutex_Create();

	R_InitDrawLists();
	
	switch(r_backend_api) {
		case BACKEND_OPENGL_LEGACY: {
			if( !GLimp_InitConfig() ) {
				QGL_Shutdown();
				return rserr_unknown;
			}
			R_FillStartupBackgroundColor( COLOR_R( glConfig.startupColor ) / 255.0f, COLOR_G( glConfig.startupColor ) / 255.0f, COLOR_B( glConfig.startupColor ) / 255.0f );
			R_SetSwapInterval( 0, -1 );
			break;
		}
		case BACKEND_NRI_DX12:
		case BACKEND_NRI_VULKAN:
		case BACKEND_NRI_METAL: {
			R_InitResourceUpload();
			break;
		}
	}


	R_TextureMode( r_texturemode->string );

	R_AnisotropicFilter( r_texturefilter->integer );

	if ( r_verbose )
		R_GfxInfo_f();

	// load and compile GLSL programs
	RP_Init();

	R_InitVBO();

	R_InitImages();

	R_InitShaders();

	R_InitCinematics();

	R_InitSkinFiles();

	R_InitModels();

	R_ClearScene();

	R_InitVolatileAssets();

	R_ClearRefInstStack();

	R_SceneInit();

	if( r_backend_api == BACKEND_NRI_VULKAN ) {
		GLenum glerr = qglGetError();
		if( glerr != GL_NO_ERROR ) {
			Com_Printf( "glGetError() = 0x%x\n", glerr );
		}
	}

	return rserr_ok;
}

/*
* R_SetMode
*/
rserr_t R_SetMode( int x, int y, int width, int height, int displayFrequency, bool fullScreen, bool stereo )
{
	rserr_t err;
	
	err = GLimp_SetMode( x, y, width, height, displayFrequency, fullScreen, stereo );
	if( err != rserr_ok )
	{
		Com_Printf( "Could not GLimp_SetMode()\n" );
		return err;
	}

	if( r_postinit )
	{
		err = R_PostInit();
		r_postinit = false;
	}

	return err;
}

/*
* R_InitVolatileAssets
*/
static void R_InitVolatileAssets( void )
{
	// init volatile data
	R_InitSkeletalCache();
	R_InitCoronas();
	R_InitCustomColors();

	rsh.envShader = R_LoadShader( "$environment", SHADER_TYPE_OPAQUE_ENV, true );
	rsh.skyShader = R_LoadShader( "$skybox", SHADER_TYPE_SKYBOX, true );
	rsh.whiteShader = R_LoadShader( "$whiteimage", SHADER_TYPE_2D, true );
	rsh.emptyFogShader = R_LoadShader( "$emptyfog", SHADER_TYPE_FOG, true );

	if( !rsh.nullVBO ) {
		rsh.nullVBO = R_InitNullModelVBO();
	}
	else {
		R_TouchMeshVBO( rsh.nullVBO );
	}
	if( !rsh.postProcessingVBO ) {
		const vattribmask_t vattribs = VATTRIB_POSITION_BIT|VATTRIB_TEXCOORDS_BIT;
		mesh_vbo_t *vbo = R_CreateMeshVBO( &rf, 4, 6, 0, vattribs, VBO_TAG_NONE, vattribs );
		assert(vbo);
		if( vbo ) {
			vec4_t xyz[4] = { {0,0,0,1}, {1,0,0,1}, {1,1,0,1}, {0,1,0,1} };
			vec2_t texcoords[4] = { {0,1}, {1,1}, {1,0}, {0,0} };
			elem_t elems[6] = { 0, 1, 2, 0, 2, 3 };
			mesh_t mesh = { 
				.numVerts = 4, 
				.xyzArray = xyz, 
				.stArray = texcoords, 
				.numElems = 6, 
				.elems = elems
			};

			R_UploadVBOVertexData( vbo, 0, vattribs, &mesh );
			R_UploadVBOElemData( vbo, 0, 0, &mesh );

			rsh.postProcessingVBO = vbo;
		}

	}
	else {
		R_TouchMeshVBO( rsh.postProcessingVBO );
	}
}

/*
* R_DestroyVolatileAssets
*/
static void R_DestroyVolatileAssets( void )
{
	// kill volatile data
	R_ShutdownCustomColors();
	R_ShutdownCoronas();
	R_ShutdownSkeletalCache();
}

/*
* R_BeginRegistration
*/
void R_BeginRegistration( void )
{
	R_FinishLoadingImages();

	R_DestroyVolatileAssets();

	rsh.registrationSequence++;
	if( !rsh.registrationSequence ) {
		// make sure assumption that an asset is free it its registrationSequence is 0
		// since rsh.registrationSequence never equals 0
		rsh.registrationSequence = 1; 
	}
	rsh.registrationOpen = true;

	R_InitVolatileAssets();

	R_DeferDataSync();

	R_DataSync();
}

/*
* R_EndRegistration
*/
void R_EndRegistration( void )
{
	if( rsh.registrationOpen == false ) {
		return;
	}

	rsh.registrationOpen = false;

	R_FreeUnusedModels();
	R_FreeUnusedVBOs();
	R_FreeUnusedSkinFiles();
	R_FreeUnusedShaders();
	R_FreeUnusedCinematics();
	R_FreeUnusedImages();

	R_RestartCinematics();

	R_DeferDataSync();

	R_DataSync();
}

/*
* R_Shutdown
*/
void R_Shutdown( bool verbose )
{
	ri.Cmd_RemoveCommand( "modellist" );
	ri.Cmd_RemoveCommand( "screenshot" );
	ri.Cmd_RemoveCommand( "envshot" );
	ri.Cmd_RemoveCommand( "imagelist" );
	ri.Cmd_RemoveCommand( "gfxinfo" );
	ri.Cmd_RemoveCommand( "shaderdump" );
	ri.Cmd_RemoveCommand( "shaderlist" );
	ri.Cmd_RemoveCommand( "glslprogramlist" );
	ri.Cmd_RemoveCommand( "cinlist" );

	// free shaders, models, etc.

	R_DestroyVolatileAssets();

	R_ShutdownModels();

	R_ShutdownSkinFiles();

	R_ShutdownVBO();

	R_ShutdownShaders();

	R_ShutdownCinematics();

	R_ShutdownImages();

	// destroy compiled GLSL programs
	RP_Shutdown();

	// restore original gamma
	if(r_backend_api == BACKEND_OPENGL_LEGACY) {
		if( glConfig.hwGamma ) {
			GLimp_SetGammaRamp( GAMMARAMP_STRIDE, glConfig.gammaRampSize, glConfig.originalGammaRamp );
		}
		// shut down OS specific OpenGL stuff like contexts, etc.
		GLimp_Shutdown();
	}

  ri.Mutex_Destroy( &rf.speedsMsgLock );
	ri.Mutex_Destroy( &rf.debugSurfaceLock );


	// shutdown our QGL subsystem
	QGL_Shutdown();

	R_FreePool( &r_mempool );
}
