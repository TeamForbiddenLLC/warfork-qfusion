#include "../ref_nri/r_win.h"
#include "../gameshared/q_arch.h"
#include <windows.h>


#define WINDOW_CLASSNAME_SIZE	120
#define WINDOW_STYLE	( WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX )
#define WITH_UTF8

typedef struct
{
	HINSTANCE hInstance;
	void *wndproc;

	char *applicationName;
	WCHAR *applicationNameW;
	HDC hDC;                    // handle to device context
	HWND hWnd;                  // handle to window
	HWND parenthWnd;			// handle to parent window
	HGLRC hGLRC;                // handle to GL rendering context
	int applicationIconResourceID;

	char windowClassName[WINDOW_CLASSNAME_SIZE];
	WCHAR windowClassNameW[WINDOW_CLASSNAME_SIZE];

	HINSTANCE hinstOpenGL;      // HINSTANCE for the OpenGL library

	// int win_x, win_y;
	bool allowdisplaydepthchange;
} glwstate_t;

static glwstate_t glw_state;

void R_WIN_Init(const char *applicationName, void *hinstance, void *wndproc, void *parenthWnd, int iconResource, const int *iconXPM) {
    
    size_t applicationNameSize = strlen( applicationName ) + 1;
    // save off hInstance and wndproc
    glw_state.applicationName = malloc( applicationNameSize );
    memcpy( glw_state.applicationName, applicationName, applicationNameSize );
#ifdef WITH_UTF8
    glw_state.applicationNameW = malloc( applicationNameSize * sizeof( WCHAR ) ); // may be larger than needed, but not smaller
    MultiByteToWideChar( CP_UTF8, 0, applicationName, -1, glw_state.applicationNameW, applicationNameSize * sizeof( WCHAR ) );
    glw_state.applicationNameW[applicationNameSize - 1] = 0;
#endif
    glw_state.hInstance = ( HINSTANCE ) hinstance;
    glw_state.wndproc = wndproc;
    glw_state.parenthWnd = ( HWND )parenthWnd;
    glw_state.applicationIconResourceID = iconResource;
}


bool R_WIN_SetFullscreen(int displayFrequency, bool fullscreen) {
}


bool R_WIN_GetWindowHandle(win_handle_t* handle) {
      assert(handle);
    handle->winType = VID_WINDOW_WIN32;
    handle->window.win.hwnd = glw_state.hWnd;
	return handle->winType != VID_WINDOW_TYPE_UNKNOWN;
}


bool R_WIN_SetWindowSize(int x, int y, uint16_t width, uint16_t height) {

}


bool R_WIN_InitWindow(win_init_t* init) {

    HWND parentHWND = glw_state.parenthWnd;
#ifdef WITH_UTF8
	WNDCLASSW wc;
#else
	WNDCLASS  wc;
#endif
 	RECT r;
	int stylebits;
	int exstyle;

	if( parentHWND )
	{
		exstyle = 0;
		stylebits = WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_VISIBLE;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	r.left = 0;
	r.top = 0;
	r.right  = init->width;
	r.bottom = init->height;

	AdjustWindowRect( &r, stylebits, FALSE );

	const int adjustWidth  = r.right - r.left;
	const int adjustHeight = r.bottom - r.top;

	Q_snprintfz( glw_state.windowClassName, sizeof( glw_state.windowClassName ), "%sWndClass", glw_state.applicationName );
#ifdef WITH_UTF8
	MultiByteToWideChar( CP_UTF8, 0, glw_state.windowClassName, -1, glw_state.windowClassNameW, sizeof( glw_state.windowClassNameW ) );
	glw_state.windowClassNameW[sizeof( glw_state.windowClassNameW )/sizeof( glw_state.windowClassNameW[0] ) - 1] = 0;
#endif

	/* Register the frame class */
	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC)glw_state.wndproc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = glw_state.hInstance;
	wc.hIcon         = LoadIcon( glw_state.hInstance, MAKEINTRESOURCE( glw_state.applicationIconResourceID ) );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = 0;
#ifdef WITH_UTF8
	wc.lpszClassName = (LPCWSTR)glw_state.windowClassNameW;
	if( !RegisterClassW( &wc ) )
#else
	wc.lpszClassName = (LPCSTR)glw_state.windowClassName;
	if( !RegisterClass( &wc ) )
#endif
		Sys_Error( "Couldn't register window class" );

	glw_state.hWnd =
#ifdef WITH_UTF8
		CreateWindowExW(
#else
		CreateWindowEx(
#endif
	        0,
#ifdef WITH_UTF8
	        glw_state.windowClassNameW,
	        glw_state.applicationNameW,
#else
	        glw_state.windowClassName,
	        glw_state.applicationName,
#endif
            0,
			init->x,
	        init->y, adjustWidth, adjustHeight,
	        parentHWND,
	        NULL,
	        glw_state.hInstance,
	        NULL );

	if( !glw_state.hWnd ) {
        return false;
    }
  

    SetActiveWindow( glw_state.hWnd );

	SetWindowLong( glw_state.hWnd, GWL_EXSTYLE, exstyle );
	SetWindowLong( glw_state.hWnd, GWL_STYLE, stylebits );

	// SetWindowPos( glw_state.hWnd, HWND_TOP, init->x, init->y, adjustWidth, adjustHeight, SWP_FRAMECHANGED | WS_VISIBLE );

	ShowWindow( glw_state.hWnd, SW_SHOW );
	
	SetForegroundWindow( glw_state.hWnd );
	SetFocus( glw_state.hWnd );

    UpdateWindow(glw_state.hWnd);

	return true;
}