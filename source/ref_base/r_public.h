/*
Copyright (C) 2002-2007 Victor Luchits

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
#ifndef R_PUBLIC_H
#define R_PUBLIC_H

#include "../cgame/ref.h"

#include "../qcommon/mod_mem.h"
#include "ref_mod.h"

#define REF_API_VERSION 21

struct mempool_s;
struct cinematics_s;

typedef struct qthread_s qthread_t;
typedef struct qmutex_s qmutex_t;
typedef struct qbufPipe_s qbufPipe_t;

//
// these are the functions exported by the refresh module
//
typedef struct
{
	// halts the application or drops to console
	void ( *Com_Error )( com_error_code_t code, const char *format, ... );

	// console messages
	void ( *Com_Printf )( const char *format, ... );
	void ( *Com_DPrintf )( const char *format, ... );

	// console variable interaction
	cvar_t *( *Cvar_Get )( const char *name, const char *value, int flags );
	cvar_t *( *Cvar_Set )( const char *name, const char *value );
	cvar_t *( *Cvar_ForceSet )( const char *name, const char *value );      // will return 0 0 if not found
	void ( *Cvar_SetValue )( const char *name, float value );
	float ( *Cvar_Value )( const char *name );
	const char *( *Cvar_String )( const char *name );

	int ( *Cmd_Argc )( void );
	char *( *Cmd_Argv )( int arg );
	char *( *Cmd_Args )( void );        // concatenation of all argv >= 1
	void ( *Cmd_AddCommand )( const char *name, void ( *cmd )( void ) );
	void ( *Cmd_RemoveCommand )( const char *cmd_name );
	void ( *Cmd_ExecuteText )( int exec_when, const char *text );
	void ( *Cmd_Execute )( void );
	void ( *Cmd_SetCompletionFunc )( const char *cmd_name, char **( *completion_func )( const char *partial ) );

	unsigned int ( *Sys_Milliseconds )( void );
	uint64_t ( *Sys_Microseconds )( void );
	void ( *Sys_Sleep )( unsigned int milliseconds );

	void *( *Com_LoadSysLibrary )( const char *name, dllfunc_t *funcs );
	void ( *Com_UnloadLibrary )( void **lib );
	void *( *Com_LibraryProcAddress )( void *lib, const char *name );

	struct cinematics_s *( *CIN_Open )( const char *name, unsigned int start_time, bool *yuv, float *framerate );
	bool ( *CIN_NeedNextFrame )( struct cinematics_s *cin, unsigned int curtime );
	uint8_t *( *CIN_ReadNextFrame )( struct cinematics_s *cin, int *width, int *height, 
		int *aspect_numerator, int *aspect_denominator, bool *redraw );
	ref_yuv_t *( *CIN_ReadNextFrameYUV )( struct cinematics_s *cin, int *width, int *height, 
		int *aspect_numerator, int *aspect_denominator, bool *redraw );
	void ( *CIN_Reset )( struct cinematics_s *cin, unsigned int cur_time );
	void ( *CIN_Close )( struct cinematics_s *cin );

	struct mempool_s *( *Mem_AllocPool )( struct mempool_s *parent, const char *name, const char *filename, int fileline );
	void ( *Mem_FreePool )( struct mempool_s **pool, const char *filename, int fileline );
	void ( *Mem_EmptyPool )( struct mempool_s *pool, const char *filename, int fileline );
	void *( *Mem_AllocExt )( struct mempool_s *pool, size_t size, size_t alignment, int z, const char *filename, int fileline );
	void ( *Mem_Free )( void *data, const char *filename, int fileline );
	void *( *Mem_Realloc )( void *data, size_t size, const char *filename, int fileline );
	size_t ( *Mem_PoolTotalSize )( struct mempool_s *pool );

	// multithreading
	struct qthread_s *( *Thread_Create )( void *(*routine) (void*), void *param );
	void ( *Thread_Join )( struct qthread_s *thread );
	void ( *Thread_Yield )( void );
	struct qmutex_s *( *Mutex_Create )( void );
	void ( *Mutex_Destroy )( struct qmutex_s **mutex );
	void ( *Mutex_Lock )( struct qmutex_s *mutex );
	void ( *Mutex_Unlock )( struct qmutex_s *mutex );

	qbufPipe_t *( *BufPipe_Create )( size_t bufSize, int flags );
	void ( *BufPipe_Destroy )( qbufPipe_t **pqueue );
	void ( *BufPipe_Finish )( qbufPipe_t *queue );
	void ( *BufPipe_WriteCmd )( qbufPipe_t *queue, const void *cmd, unsigned cmd_size );
	int ( *BufPipe_ReadCmds )( qbufPipe_t *queue, unsigned (**cmdHandlers)( const void * ) );
	void ( *BufPipe_Wait )( qbufPipe_t *queue, int (*read)( qbufPipe_t *, unsigned( ** )(const void *), bool ), 
	unsigned (**cmdHandlers)( const void * ), unsigned timeout_msec );
	
	const struct fs_import_s* fsImport;
	const struct mem_import_s* memImport;
} ref_import_t;



typedef struct
{
	// if API is different, the dll cannot be used
	int			( *API )( void );
	struct ref_import_s refImport;
} ref_export_t;

typedef ref_export_t *(*GetRefAPI_t)(const ref_import_t *imports);

#ifdef REF_HARD_LINKED
#ifdef __cplusplus
extern "C" {
#endif
	ref_export_t *GetRefAPI( ref_import_t *import );
#ifdef __cplusplus
}
#endif
#endif

#endif // R_PUBLIC_H
