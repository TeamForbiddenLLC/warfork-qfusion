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

/*
This code is part of DynGL, a method of dynamically loading an OpenGL
library without much pain designed by Joseph Carter and is based
loosely on previous work done both by Zephaniah E. Hull and Joseph.

Both contributors have decided to disclaim all Copyright to this work.
It is released to the Public Domain WITHOUT ANY WARRANTY whatsoever,
express or implied, in the hopes that others will use it instead of
other less-evolved hacks which usually don't work right.  ;)
*/

/*
The following code is loosely based on DynGL code by Joseph Carter
and Zephaniah E. Hull. Adapted by Victor Luchits for qfusion project.
*/

/*
** SDL_QGL.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Qfusion you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/

#include <SDL3/SDL.h>

#include "../qcommon/qcommon.h"
#include "sdl_glw.h"

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

static const char *_qglGetGLWExtensionsString( void );
static const char *_qglGetGLWExtensionsStringInit( void );

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	glw_state.OpenGLLib = NULL;

#define QGL_FUNC( type, name, params ) ( q##name ) = NULL;
#define QGL_FUNC_OPT( type, name, params ) ( q##name ) = NULL;
#define QGL_EXT( type, name, params ) ( q##name ) = NULL;
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
}

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to
** the appropriate GL stuff.  In Windows this means doing a
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
**
*/
qgl_initerr_t QGL_Init( const char *dllname )
{
	glw_state.OpenGLLib = (void *)0;
	// SDL3 SDL_InitSubSystem / SDL_GL_LoadLibrary return true on success.
	if( !SDL_InitSubSystem( SDL_INIT_VIDEO ) ) {
		Com_Printf( "SDL_InitSubSystem(SDL_INIT_VIDEO) failed: %s", SDL_GetError() );
		return qgl_initerr_unknown;
	}

	// try the system default first
	bool loaded = SDL_GL_LoadLibrary( NULL );
	if( !loaded )
		loaded = SDL_GL_LoadLibrary( dllname );

	if( !loaded ) {
		Com_Printf( "Error loading %s: %s\n", dllname ? dllname : "OpenGL dlib", SDL_GetError() );
		return qgl_initerr_invalid_driver;
	} else {
		glw_state.OpenGLLib = (void *)1;
		if( dllname )
			Com_Printf( "Using %s for OpenGL...\n", dllname );
	}

#define QGL_FUNC( type, name, params )                                   \
	( q##name ) = (void *)qglGetProcAddress( (const GLubyte *)#name );   \
	if( !( q##name ) ) {                                                 \
		Com_Printf( "QGL_Init: Failed to get address for %s\n", #name ); \
		return qgl_initerr_invalid_driver;                               \
	}
#define QGL_FUNC_OPT( type, name, params ) ( q##name ) = (void *)qglGetProcAddress( (const GLubyte *)#name );
#define QGL_EXT( type, name, params ) ( q##name ) = NULL;
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

	qglGetGLWExtensionsString = _qglGetGLWExtensionsString;

	return qgl_initerr_ok;
}

/*
** QGL_GetDriverInfo
**
** Returns information about the GL DLL.
*/
const qgl_driverinfo_t *QGL_GetDriverInfo( void )
{
	return NULL;
}

/*
** qglGetProcAddress
*/
void *qglGetProcAddress( const GLubyte *procName )
{
	if( glw_state.OpenGLLib )
		return (void *)SDL_GL_GetProcAddress( (char *)procName );
	return NULL;
}

/*
** qglGetGLWExtensionsString
*/
static const char *_qglGetGLWExtensionsString( void )
{
	return NULL;
}
