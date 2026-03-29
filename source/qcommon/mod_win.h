#ifndef R_WIN_MODULE_H
#define R_WIN_MODULE_H

#include "../gameshared/q_arch.h"

typedef enum {
	VID_WINDOW_VULKAN,
	VID_WINDOW_GL
} window_display_driver_t;

typedef enum {
	VID_WINDOW_TYPE_UNKNOWN,
	VID_WINDOW_TYPE_X11,
	VID_WINDOW_WAYLAND,
	VID_WINDOW_OSX,
	VID_WINDOW_WIN32
} window_type_t;

typedef struct {
	window_display_driver_t backend;
	uint16_t width;
	uint16_t height;
	int x;
	int y;
} win_init_t;

typedef struct win_handle_s {
	window_type_t winType;
	union {
		struct {
  		void* dpy; // Display*
  		uint64_t window; // Window
		} x11;
		struct {
			void* display;
			void* surface;
		} wayland;
		struct {
			void* metalLayer;
		} osx;
		struct {
			void* hwnd;
		} win;
	} window;
} win_handle_t;


#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
	ret name( __VA_ARGS__ );

DECLARE_TYPEDEF_METHOD( void, R_WIN_Init, const char *applicationName, void *hinstance, void *wndproc, void *parenthWnd, int iconResource, const int *iconXPM );
DECLARE_TYPEDEF_METHOD( void, R_WIN_Shutdown );
DECLARE_TYPEDEF_METHOD( bool, R_WIN_InitWindow, win_init_t *init );
DECLARE_TYPEDEF_METHOD( bool, R_WIN_SetFullscreen, int displayFrequency, uint16_t width, uint16_t height );
DECLARE_TYPEDEF_METHOD( bool, R_WIN_SetWindowed, int x, int y, uint16_t width, uint16_t height );
DECLARE_TYPEDEF_METHOD( bool, R_WIN_GetWindowHandle, win_handle_t *handle );

#undef DECLARE_TYPEDEF_METHOD

struct win_import_s {
	R_WIN_InitFn R_WIN_Init;
	R_WIN_ShutdownFn R_WIN_Shutdown;
	R_WIN_InitWindowFn R_WIN_InitWindow;
	R_WIN_SetFullscreenFn R_WIN_SetFullscreen;
	R_WIN_SetWindowedFn R_WIN_SetWindowed;
	R_WIN_GetWindowHandleFn R_WIN_GetWindowHandle;
};

#define DECLARE_WIN_STRUCT() { \
	R_WIN_Init, \
	R_WIN_Shutdown, \
	R_WIN_InitWindow, \
	R_WIN_SetFullscreen, \
	R_WIN_SetWindowed, \
	R_WIN_GetWindowHandle \
};

#if WIN_DEFINE_INTERFACE_IMPL
static struct win_import_s win_import;
void R_WIN_Init(const char *applicationName, void *hinstance, void *wndproc, void *parenthWnd, int iconResource, const int *iconXPM ) { win_import.R_WIN_Init(applicationName, hinstance, wndproc, parenthWnd, iconResource, iconXPM); }
void R_WIN_Shutdown(){ win_import.R_WIN_Shutdown(); } 
bool R_WIN_InitWindow(win_init_t *init ){ win_import.R_WIN_InitWindow(init); }  
bool R_WIN_SetFullscreen(int displayFrequency, uint16_t width, uint16_t height ){ win_import.R_WIN_SetFullscreen(displayFrequency, width, height); }   
bool R_WIN_SetWindowed(int x, int y, uint16_t width, uint16_t height ){ win_import.R_WIN_SetWindowed(x, y, width, height); }    
bool R_WIN_GetWindowHandle(win_handle_t *handle ){ win_import.R_WIN_GetWindowHandle(handle); }     

static inline void Q_ImportWinModule(const struct win_import_s* ref) {
	win_import = *ref;
}
#endif

#endif
