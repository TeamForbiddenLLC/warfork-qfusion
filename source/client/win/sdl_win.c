#include "../../qcommon/mod_win.h"
#include "../../gameshared/q_cvar.h"
#include <SDL3/SDL.h>


typedef int (* wndproc_t)(void *, int, int, int);

typedef struct {
	char *applicationName;
	int *applicationIcon;
	SDL_Window *sdl_window;
	SDL_MetalView metal_view;
	wndproc_t wndproc;
} winstate_t;

static winstate_t r_winState;
extern cvar_t *vid_fullscreen;

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

	// SDL3 dropped SDL_WINDOW_FULLSCREEN_DESKTOP; SDL_SetWindowFullscreen now toggles
	// borderless-desktop fullscreen via a bool (exclusive modes use SDL_SetWindowFullscreenMode).
	SDL_SetWindowSize(r_winState.sdl_window, width, height);
	return SDL_SetWindowFullscreen( r_winState.sdl_window, true );
}
bool R_WIN_GetWindowHandle(win_handle_t* handle) {
  assert(handle);
  if(r_winState.sdl_window == NULL) {
    return false;
  }

	handle->winType = VID_WINDOW_TYPE_UNKNOWN;

	// SDL3 removed SDL_GetWindowWMInfo/SDL_syswm.h; native handles come from window properties,
	// dispatched on the active video driver.
	const char *driver = SDL_GetCurrentVideoDriver();
	if( driver == NULL ) {
		return false;
	}
	SDL_PropertiesID props = SDL_GetWindowProperties( r_winState.sdl_window );

	if( SDL_strcmp( driver, "x11" ) == 0 ) {
		handle->winType = VID_WINDOW_TYPE_X11;
		handle->window.x11.dpy = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL );
		handle->window.x11.window = (uint64_t)SDL_GetNumberProperty( props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0 );
	} else if( SDL_strcmp( driver, "wayland" ) == 0 ) {
		handle->winType = VID_WINDOW_WAYLAND;
		handle->window.wayland.display = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL );
		handle->window.wayland.surface = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL );
	} else if( SDL_strcmp( driver, "windows" ) == 0 ) {
		handle->winType = VID_WINDOW_WIN32;
		handle->window.win.hwnd = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL );
	}
#ifdef __APPLE__
	else if( SDL_strcmp( driver, "cocoa" ) == 0 ) {
		handle->winType = VID_WINDOW_OSX;
		if( r_winState.metal_view == NULL ) {
			r_winState.metal_view = SDL_Metal_CreateView( r_winState.sdl_window );
		}
		handle->window.osx.metalLayer = SDL_Metal_GetLayer( r_winState.metal_view );
	}
#endif
	return handle->winType != VID_WINDOW_TYPE_UNKNOWN;
}

bool R_WIN_SetWindowed(int x, int y, uint16_t width, uint16_t height) {
  if(!r_winState.sdl_window) {
    return false;
  }
	SDL_SetWindowFullscreen( r_winState.sdl_window, false );
	SDL_SetWindowSize(r_winState.sdl_window, width, height);
	SDL_SetWindowPosition(r_winState.sdl_window, x, y);
	return true;
}

bool R_WIN_InitWindow(win_init_t* init) {
  assert(init);
  assert(r_winState.sdl_window == NULL);

  SDL_WindowFlags winFlags = 0;
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
	// SDL3 SDL_CreateWindow no longer takes a position; set it afterwards.
	r_winState.sdl_window = SDL_CreateWindow( r_winState.applicationName, init->width, init->height, winFlags );
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
		// XPM payload is packed 0xAARRGGBB ints == SDL_PIXELFORMAT_ARGB8888 (endian-independent in SDL).
		SDL_Surface *surface = SDL_CreateSurfaceFrom( xpm_icon[0], xpm_icon[1], SDL_PIXELFORMAT_ARGB8888,
			(void *)( xpm_icon + 2 ), xpm_icon[0] * 4 );

		SDL_SetWindowIcon( r_winState.sdl_window, surface );
		SDL_DestroySurface( surface );
	}
#endif
	return true;
}

void R_WIN_Shutdown() {

#ifdef __APPLE__
	if( r_winState.metal_view ) {
		SDL_Metal_DestroyView( r_winState.metal_view );
	}
#endif
	SDL_DestroyWindow(r_winState.sdl_window);

	free( r_winState.applicationName );
	free( r_winState.applicationIcon );

	memset(&r_winState, 0, sizeof(r_winState) );
}
