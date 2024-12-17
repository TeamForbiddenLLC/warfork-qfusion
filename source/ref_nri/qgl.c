#include "../qcommon/qcommon.h"

#define QGL_EXTERN

#define QGL_FUNC( type, name, params ) type( APIENTRY *q##name ) params;
#define QGL_FUNC_OPT( type, name, params ) type( APIENTRY *q##name ) params;
#define QGL_EXT( type, name, params ) type( APIENTRY *q##name ) params;
#define QGL_WGL( type, name, params )
#define QGL_WGL_EXT( type, name, params )
#define QGL_GLX( type, name, params )
#define QGL_GLX_EXT( type, name, params )
#define QGL_EGL( type, name, params )
#define QGL_EGL_EXT( type, name, params )

#include "../ref_gl/qgl.h"

#undef QGL_EGL_EXT
#undef QGL_EGL
#undef QGL_GLX_EXT
#undef QGL_GLX
#undef QGL_WGL_EXT
#undef QGL_WGL
#undef QGL_EXT
#undef QGL_FUNC_OPT
#undef QGL_FUNC


