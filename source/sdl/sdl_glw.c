/*
   Copyright (C) 1997-2001 Id Software, Inc.

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

#include <SDL.h>
#ifdef __WASM__
#include <emscripten.h>
#endif

#include "../ref_gl/r_local.h"
#include "sdl_glw.h"

#ifdef __WASM__
void GLimp_FillBrowserCanvas( void )
{
	EM_ASM({
		var c = ( typeof Module !== 'undefined' ) ? Module['canvas'] : null;
		if( !c ) {
			c = document.getElementById('canvas');
		}
		if( !c || !c.style ) {
			return;
		}
		c.style.position = 'fixed';
		c.style.left = '0';
		c.style.top = '0';
		c.style.right = '0';
		c.style.bottom = '0';
		c.style.width = '100%';
		c.style.height = '100%';
		c.style.margin = '0';
		c.style.padding = '0';
		c.style.border = '0';
		c.style.display = 'block';
	});
}

void GLimp_UpdateSize( int width, int height )
{
	if( width > 0 && height > 0 ) {
		glConfig.width = width;
		glConfig.height = height;
	}
	GLimp_FillBrowserCanvas();
}

void GLimp_SyncBrowserCanvas( int *outw, int *outh )
{
	int w, h, curw, curh;

	/* Do not shrink the drawing buffer when the mobile URL bar appears:
	 * assigning canvas.width/height clears WebGL to black until the next
	 * rendered frame, which looks like a black rectangle while idle. */
	w = EM_ASM_INT({
		var c = ( typeof Module !== 'undefined' ) ? Module['canvas'] : null;
		if( !c ) {
			c = document.getElementById( 'canvas' );
		}
		var iw = Math.round( window.innerWidth || document.documentElement.clientWidth || 64 ) | 0;
		var ih = Math.round( window.innerHeight || document.documentElement.clientHeight || 64 ) | 0;
		if( iw < 64 ) { iw = 64; }
		if( ih < 64 ) { ih = 64; }
		if( !c ) {
			return iw;
		}
		var landscape = iw >= ih;
		var wasLandscape = c.width >= c.height;
		if( c.width < 64 || c.height < 64 || landscape !== wasLandscape ) {
			c.width = iw;
			c.height = ih;
		} else {
			if( iw > c.width ) {
				c.width = iw;
			}
			if( ih > c.height ) {
				c.height = ih;
			}
		}
		c.style.width = '100%';
		c.style.height = '100%';
		c.style.left = '0';
		c.style.top = '0';
		c.style.right = '0';
		c.style.bottom = '0';
		return c.width | 0;
	});
	h = EM_ASM_INT({
		var c = ( typeof Module !== 'undefined' ) ? Module['canvas'] : null;
		if( !c ) {
			c = document.getElementById( 'canvas' );
		}
		return c ? ( c.height | 0 ) : 64;
	});
	if( w < 64 ) {
		w = 64;
	}
	if( h < 64 ) {
		h = 64;
	}

	if( glw_state.sdl_window ) {
		SDL_GetWindowSize( glw_state.sdl_window, &curw, &curh );
		if( curw != w || curh != h ) {
			SDL_SetWindowSize( glw_state.sdl_window, w, h );
		}
	}

	glConfig.width = w;
	glConfig.height = h;
	GLimp_FillBrowserCanvas();

	if( outw ) {
		*outw = w;
	}
	if( outh ) {
		*outh = h;
	}
}
#endif

glwstate_t glw_state = {NULL, false};
cvar_t *vid_fullscreen;

static int GLimp_InitGL( int stencilbits, bool stereo );

void GLimp_SetWindowIcon( void )
{
#ifndef __APPLE__
	const int *xpm_icon = glw_state.applicationIcon;

	if( xpm_icon )
	{
		SDL_Surface *surface;

		surface = SDL_CreateRGBSurfaceFrom( (void *)(xpm_icon+2), xpm_icon[0], xpm_icon[1], 32, xpm_icon[0]*4,
#ifdef ENDIAN_LITTLE
			0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 );
#else
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );
#endif 

		SDL_SetWindowIcon( glw_state.sdl_window, surface );

		SDL_FreeSurface( surface );
	}
#endif
}

rserr_t GLimp_SetFullscreen( bool fullscreen, int xpos, int ypos )
{
	Uint32 flags = 0;

	if( fullscreen ) {
		xpos = ypos = 0;
#ifdef __APPLE__
		// we need to use SDL_WINDOW_FULLSCREEN_DESKTOP to support Alt+Tab from fullscreen on OS X
		flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
		flags = SDL_WINDOW_FULLSCREEN;
#endif
	}

#ifdef __WASM__
	(void)flags;
	(void)xpos;
	(void)ypos;
	glConfig.fullScreen = fullscreen;
	return rserr_ok;
#else
    if( SDL_SetWindowFullscreen( glw_state.sdl_window, flags ) == 0 ) {
        SDL_SetWindowPosition( glw_state.sdl_window, xpos, ypos );
        glConfig.fullScreen = fullscreen;
        return rserr_ok;
    }
#endif

    return rserr_invalid_fullscreen;
}

static void GLimp_CreateWindow( int x, int y, int width, int height )
{
#ifdef __WASM__
	glw_state.sdl_window = SDL_CreateWindow( glw_state.applicationName,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
#else
	glw_state.sdl_window = SDL_CreateWindow( glw_state.applicationName, 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL );
#endif

	if( !glw_state.sdl_window )
		Sys_Error( "Couldn't create window: \"%s\"", SDL_GetError() );

	if( glw_state.wndproc ) {
		glw_state.wndproc( glw_state.sdl_window, 0, 0, 0 );
	}

	SDL_SetWindowPosition( glw_state.sdl_window, x, y );

#ifdef __WASM__
	GLimp_FillBrowserCanvas();
#endif

	GLimp_SetWindowIcon();
}

/**
 * Set video mode.
 * @param mode number of the mode to set
 * @param fullscreen <code>true</code> for a fullscreen mode,
 *     <code>false</code> otherwise
 */
rserr_t GLimp_SetMode( int x, int y, int width, int height, int displayFrequency, bool fullscreen, bool stereo )
{
	const char *win_fs[] = {"W", "FS"};

#ifdef __WASM__
	{
		int cw, ch;
		GLimp_SyncBrowserCanvas( &cw, &ch );
		if( cw >= 64 && ch >= 64 ) {
			width = cw;
			height = ch;
		}
		/* Browser fullscreen is handled in JS. Recreating the WebGL
		 * context on a phone usually fails (VID_ChangeMode code 2). */
		fullscreen = false;
		stereo = false;
	}
#endif

	ri.Com_Printf( "Initializing OpenGL display\n" );
	ri.Com_Printf( "...setting mode:" );
	ri.Com_Printf( " %d %d %s\n", width, height, win_fs[fullscreen] );

#ifdef __WASM__
	if( glw_state.sdl_window && glw_state.sdl_glcontext ) {
		SDL_SetWindowSize( glw_state.sdl_window, width, height );
		glConfig.fullScreen = false;
		glConfig.width = width;
		glConfig.height = height;
		GLimp_FillBrowserCanvas();
		return rserr_ok;
	}
#endif

	// destroy the existing window
	if( glw_state.sdl_window ) {
		GLimp_Shutdown();
	}

	GLimp_CreateWindow( x, y, width, height );

	// init all the gl stuff for the window
	if( !GLimp_InitGL( r_stencilbits->integer, stereo ) ) {
		ri.Com_Printf( "VID_CreateWindow() - GLimp_InitGL failed\n" );
		return rserr_invalid_mode;
	}

    glConfig.fullScreen = fullscreen ? GLimp_SetFullscreen( fullscreen, x, y ) == rserr_ok : false;
	glConfig.width = width;
	glConfig.height = height;

#ifdef __WASM__
	glConfig.fullScreen = false;
	return rserr_ok;
#else
    return glConfig.fullScreen == fullscreen ? rserr_ok : rserr_invalid_fullscreen;
#endif
}

/**
 * Shutdown GLimp sub system.
 */
void GLimp_Shutdown()
{
	SDL_GL_DeleteContext( glw_state.sdl_glcontext );
	SDL_DestroyWindow( glw_state.sdl_window );

	free( glw_state.applicationName );
	free( glw_state.applicationIcon );

	memset( &glw_state, 0, sizeof( glw_state ) );

	glConfig.width = 0;
	glConfig.height = 0;
}

/**
 * Initialize GLimp sub system.
 * @param hinstance
 * @param wndproc
 */

int GLimp_Init( const char *applicationName, void *hinstance, void *wndproc, void *parenthWnd,
	int iconResource, const int *iconXPM )
{
	glw_state.wndproc = wndproc;
	glw_state.applicationName = strdup( applicationName );
	glw_state.applicationIcon = NULL;
	memcpy( glw_state.applicationName, applicationName, strlen( applicationName ) + 1 );
	
	if( iconXPM )
	{
		size_t icon_memsize = iconXPM[0] * iconXPM[1] * sizeof( int );
		glw_state.applicationIcon = malloc( icon_memsize );
		memcpy( glw_state.applicationIcon, iconXPM, icon_memsize );
	}

	return true;
}

static int GLimp_InitGL( int stencilbits, bool stereo )
{
	int colorBits, depthBits, stencilBits, stereo_;
	SDL_GLContext sdl_glcontext = 0;

#ifdef __WASM__
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
	SDL_GL_SetAttribute( SDL_GL_STEREO, 0 );
	(void)stereo;
	stencilbits = 0;
#else
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, max( 0, stencilbits ) );
#ifndef NDEBUG
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
#endif

	if( stereo ) {
		ri.Com_DPrintf( "...attempting to use stereo\n" );
		SDL_GL_SetAttribute( SDL_GL_STEREO, 1 );
	}
#endif

	sdl_glcontext = SDL_GL_CreateContext( glw_state.sdl_window );
	if( sdl_glcontext == 0 ) {
#ifdef __WASM__
		ri.Com_Printf( "GLimp_Init() - retry GL context without ES profile: \"%s\"\n", SDL_GetError() );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, 0 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
		sdl_glcontext = SDL_GL_CreateContext( glw_state.sdl_window );
#endif
	}
	if( sdl_glcontext == 0 ) {
		ri.Com_Printf( "GLimp_Init() - SDL_GL_CreateContext failed: \"%s\"\n", SDL_GetError() );
		goto fail;
	}

	if( SDL_GL_MakeCurrent( glw_state.sdl_window, sdl_glcontext ) ) {
		ri.Com_Printf( "GLimp_Init() - SDL_GL_MakeCurrent failed: \"%s\"\n", SDL_GetError() );
		goto fail;
	}

	/*
	 ** print out PFD specifics
	 */
	SDL_GL_GetAttribute( SDL_GL_BUFFER_SIZE, &colorBits );
	SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &depthBits );
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &stencilBits );
	SDL_GL_GetAttribute( SDL_GL_STEREO, &stereo_ );

	glConfig.stencilBits = stencilBits;
	glConfig.stereoEnabled = stereo_ != 0;

	ri.Com_Printf( "GL PFD: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", colorBits, depthBits, stencilBits );

	glw_state.sdl_glcontext = sdl_glcontext;
	return true;

fail:
	if( sdl_glcontext ) {
		SDL_GL_DeleteContext( sdl_glcontext );
		sdl_glcontext = 0;
	}

	return false;
}

/**
 * TODO documentation
 */
void GLimp_BeginFrame( void )
{
}

/**
 * Swap the buffers and possibly do other stuff that yet needs to be
 * determined.
 */
void GLimp_EndFrame( void )
{
#ifdef __WASM__
	GLimp_SyncBrowserCanvas( NULL, NULL );
#endif
	SDL_GL_SwapWindow( glw_state.sdl_window );
}

/**
 * TODO documentation
 */
bool GLimp_GetGammaRamp( size_t stride, unsigned short *psize, unsigned short *ramp )
{
	unsigned short ramp256[3 * 256];

#if defined( __EMSCRIPTEN__ ) || defined( __WASM__ )
	(void)stride;
	(void)psize;
	(void)ramp;
	return false;
#endif

	if( stride < 256 ) {
		// SDL only supports gamma ramps with 256 mappings per channel
		return false;
	}

	if( SDL_GetWindowGammaRamp( glw_state.sdl_window, ramp256, ramp256 + 256, ramp256 + ( 256 << 1 ) ) != -1 ) {
		*psize = 256;
		memcpy( ramp, ramp256, 256 * sizeof( *ramp ) );
		memcpy( ramp + stride, ramp256 + 256, 256 * sizeof( *ramp ) );
		memcpy( ramp + 2 * stride, ramp256 + 2 * 256, 256 * sizeof( *ramp ) );
		return true;
	}
	return false;
}

/**
 * TODO documentation
 */
void GLimp_SetGammaRamp( size_t stride, unsigned short size, unsigned short *ramp )
{
	unsigned short ramp256[3 * 256];

#if defined( __EMSCRIPTEN__ ) || defined( __WASM__ )
	(void)stride;
	(void)size;
	(void)ramp;
	(void)ramp256;
	return;
#endif

	if( size != 256 )
		return;

	memcpy( ramp256, ramp, size * sizeof( *ramp ) );
	memcpy( ramp256 + 256, ramp + stride, size * sizeof( *ramp ) );
	memcpy( ramp256 + 2 * 256, ramp + 2 * stride, size * sizeof( *ramp ) );
	if( SDL_SetWindowGammaRamp( glw_state.sdl_window, ramp256, ramp256 + 256, ramp256 + ( 256 << 1 ) ) == -1 ) {
		Com_Printf( "SDL_SetWindowGammaRamp(...) failed: ", SDL_GetError() );
	}
}

/**
 * TODO documentation
 */
void GLimp_AppActivate( bool active, bool destroy )
{
}

/*
** GLimp_SetWindow
*/
rserr_t GLimp_SetWindow( void *hinstance, void *wndproc, void *parenthWnd, bool *surfaceChangePending )
{
	if( surfaceChangePending )
		*surfaceChangePending = false;

	return rserr_ok; // surface cannot be lost
}

/*
** GLimp_RenderingEnabled
*/
bool GLimp_RenderingEnabled( void )
{
	return true;
}

/*
** GLimp_SetSwapInterval
*/
void GLimp_SetSwapInterval( int swapInterval )
{
	SDL_GL_SetSwapInterval( swapInterval );
}

/*
** GLimp_MakeCurrent
*/
bool GLimp_MakeCurrent( void *context, void *surface )
{
	return SDL_GL_MakeCurrent( glw_state.sdl_window, (SDL_GLContext)context ) == 0;
}

/*
** GLimp_EnableMultithreadedRendering
*/
void GLimp_EnableMultithreadedRendering( bool enable )
{
}

/*
** GLimp_GetWindowSurface
*/
void *GLimp_GetWindowSurface( bool *renderable )
{
	if( renderable )
		*renderable = true;
	return NULL;
}

/*
** GLimp_UpdatePendingWindowSurface
*/
void GLimp_UpdatePendingWindowSurface( void )
{
}

/*
** GLimp_SharedContext_Create
*/
bool GLimp_SharedContext_Create( void **context, void **surface )
{
	SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );
	
	*context = (void*)SDL_GL_CreateContext( glw_state.sdl_window );
	if( surface )
		*surface = NULL;
	
	// SDL_GL_CreateContext makes the newly created context current
	// we don't want that, so revert to our main context
	return SDL_GL_MakeCurrent( glw_state.sdl_window, glw_state.sdl_glcontext ) == 0;
}

/*
** GLimp_SharedContext_Destroy
*/
void GLimp_SharedContext_Destroy( void *context, void *surface )
{
	SDL_GL_DeleteContext( (SDL_GLContext)context );
}
