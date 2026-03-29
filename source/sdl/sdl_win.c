#include "sdl_win.h"
#include "../ref_nri/r_win.h"

#include "SDL_video.h"
#include <SDL_syswm.h>

winstate_t r_winState;

void R_WIN_Init(const char *applicationName, void *hinstance, void *wndproc, void *parenthWnd, int iconResource, const int *iconXPM) {
	r_winState.wndproc = wndproc;
	r_winState.applicationName = strdup( applicationName );
	r_winState.applicationIcon = NULL;
	memcpy( r_winState.applicationName, applicationName, strlen( applicationName ) + 1 );
	
	if( iconXPM )
	{
		size_t icon_memsize = iconXPM[0] * iconXPM[1] * sizeof( int );
		r_winState.applicationIcon = malloc( icon_memsize );
		memcpy( r_winState.applicationIcon, iconXPM, icon_memsize );
	}
}


bool R_WIN_SetFullscreen(int displayFrequency, uint16_t width, uint16_t height ) {
	assert( r_winState.sdl_window );
	uint32_t flags = 0;
#ifdef __APPLE__
		// we need to use SDL_WINDOW_FULLSCREEN_DESKTOP to support Alt+Tab from fullscreen on OS X
		flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
		flags = SDL_WINDOW_FULLSCREEN;
#endif

	SDL_SetWindowSize(r_winState.sdl_window, width, height);
	if( SDL_SetWindowFullscreen( r_winState.sdl_window, flags ) == 0 ) {
		return true;
	}

	return false;
}
bool R_WIN_GetWindowHandle(win_handle_t* handle) {
  assert(handle);
  if(r_winState.sdl_window == NULL) {
    return false;
  }

	SDL_SysWMinfo wmi = {};
	SDL_VERSION(&wmi.version);
	handle->winType = VID_WINDOW_TYPE_UNKNOWN;
	if( SDL_GetWindowWMInfo( r_winState.sdl_window, &wmi ) ) {
		switch( wmi.subsystem ) {
#if defined( SDL_VIDEO_DRIVER_X11 )
			case SDL_SYSWM_X11:
				handle->winType = VID_WINDOW_TYPE_X11;
				handle->window.x11.window = wmi.info.x11.window;
				handle->window.x11.dpy = wmi.info.x11.display;
				break;
#endif
#if defined( SDL_VIDEO_DRIVER_WAYLAND )
			case SDL_SYSWM_WAYLAND:
				handle->winType = VID_WINDOW_WAYLAND;
				handle->window.wayland.display = wmi.info.wl.display;
				handle->window.wayland.surface = wmi.info.wl.surface;
				break;
#endif
#if defined( SDL_VIDEO_DRIVER_WINDOWS )
			case SDL_SYSWM_WINDOWS:
				assert( false ); // TODO implement
				// init.winType = WINDOW_TYPE_WIN32;
				// init.window.win.hwnd = wmi.info.;
				// init.window.win.surface = wmi.info.wl.surface;
				break;
#endif
			default:
				assert( false );
				break;
		}
	}
	return handle->winType != VID_WINDOW_TYPE_UNKNOWN;
}

bool R_WIN_SetWindowed(int x, int y, uint16_t width, uint16_t height) {
  if(!r_winState.sdl_window) {
    return false;
  }
	SDL_SetWindowFullscreen( r_winState.sdl_window, 0); 
	SDL_SetWindowSize(r_winState.sdl_window, width, height);
	SDL_SetWindowPosition(r_winState.sdl_window, x, y);
	return true;
}

bool R_WIN_InitWindow(win_init_t* init) {
  assert(init);
  assert(r_winState.sdl_window == NULL);
  
  uint32_t winFlags = 0;
  switch(init->backend) {
    case VID_WINDOW_GL:
      winFlags = SDL_WINDOW_OPENGL;
      break;
    case VID_WINDOW_VULKAN:
      winFlags = SDL_WINDOW_VULKAN;
      break;
    default:
    	assert(0);
    	break;
  }
	r_winState.sdl_window= SDL_CreateWindow( r_winState.applicationName, 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, init->width, init->height, winFlags);
	// Check that the window was successfully created
	if (r_winState.sdl_window == NULL) {
		// In the case that the window could not be made...
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return false;
	}	

	SDL_SetWindowPosition( r_winState.sdl_window, init->x, init->y );

  if( r_winState.wndproc ) {
	  r_winState.wndproc( r_winState.sdl_window, 0, 0, 0 );
  }

#ifndef __APPLE__

	if( r_winState.applicationIcon ) {
		const int *xpm_icon = r_winState.applicationIcon;
		SDL_Surface *surface = SDL_CreateRGBSurfaceFrom( (void *)( xpm_icon + 2 ), xpm_icon[0], xpm_icon[1], 32, xpm_icon[0] * 4,
#ifdef ENDIAN_LITTLE
											0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 );
#else
											0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );
#endif

		SDL_SetWindowIcon( r_winState.sdl_window, surface );
		SDL_FreeSurface( surface );
	}
#endif
	return true;
}

void R_WIN_Shutdown() {

	SDL_DestroyWindow(r_winState.sdl_window);
	
	free( r_winState.applicationName );
	free( r_winState.applicationIcon );

	memset(&r_winState, 0, sizeof(r_winState) );
}
