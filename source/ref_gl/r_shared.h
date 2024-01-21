#ifndef R_SHARED_H
#define R_SHARED_H

#include "../gameshared/q_shared.h"

typedef enum {
	WINDOW_TYPE_WAYLAND,
	WINDOW_TYPE_X11,
	//WINDOW_TYPE_XCB,
	WINDOW_TYPE_WIN32,
	WINDOW_TYPE_OSX
} window_type_t;

typedef enum { 
	BACKEND_OPENGL_LEGACY, 
	BACKEND_NRI_VULKAN, 
	BACKEND_NRI_METAL, 
	BACKEND_NRI_DX12 
} backend_api_t;

typedef struct r_app_init_desc_s {
	backend_api_t api;
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
	const char *applicationName;
	const char *screenshotPrefix;
	int startupColor;
	int iconResource;
	const int *iconXPM;
	void *hinstance;
	void *wndProc;
	void *parenthWnd;
	uint16_t width;
	uint16_t height;
	bool verbose;
} r_app_init_desc_t;

#endif

