#include "../gameshared/q_cvar.h"

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

void R_WIN_Init(const char *applicationName, void *hinstance, void *wndproc, void *parenthWnd, int iconResource, const int *iconXPM );
void R_WIN_Shutdown();
bool R_WIN_InitWindow(win_init_t* init);
bool R_WIN_SetFullscreen(int displayFrequency, uint16_t width, uint16_t height );
bool R_WIN_SetWindowed(int x, int y, uint16_t width, uint16_t height);
bool R_WIN_GetWindowHandle(win_handle_t* handle);
