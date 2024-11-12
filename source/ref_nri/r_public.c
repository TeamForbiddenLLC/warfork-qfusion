/*
Copyright (C) 2013 Victor Luchits

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

#define FS_DEFINE_INTERFACE_IMPL 1
#include "../qcommon/mod_fs.h"

#define MEM_DEFINE_INTERFACE_IMPL 1
#include "../qcommon/mod_mem.h"

#include "../ref_base/ref_mod.h"

// r_public.c
#include "r_local.h"
#include "r_frontend.h"


ref_import_t ri;


/*
* GetRefAPIVersion
*/
static int GetRefAPIVersion( void )
{
	return REF_API_VERSION;
}

/*
* GetRefAPI
* 
* Returns a pointer to the structure with all entry points
*/
#ifdef __cplusplus
extern "C"
{
#endif

QF_DLL_EXPORT ref_export_t *GetRefAPI( ref_import_t *import )
{
	static ref_export_t globals;
	fs_import = *import->fsImport;
	Q_ImportMemModule(import->memImport);	

	ri = *import;

	globals.API = GetRefAPIVersion;
	globals.refImport =  (struct ref_import_s)DECLARE_REF_STRUCT();

	return &globals;
}

#ifdef __cplusplus
}
#endif

#ifndef REF_HARD_LINKED

// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[3072];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	ri.Com_Error( ERR_FATAL, "%s", msg );
}

void Com_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[3072];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	ri.Com_Printf( "%s", msg );
}

void Com_DPrintf( const char *format, ... )
{
	va_list	argptr;
	char msg[3072];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	ri.Com_DPrintf( "%s", msg );
}

#endif

#if defined(HAVE_DLLMAIN) && !defined(REF_HARD_LINKED)
int WINAPI DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
