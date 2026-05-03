/*
Copyright (C) 2002-2003 Victor Luchits

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

#ifdef FTLIB_HARD_LINKED
#define FTLIB_IMPORT ftlibi_imp_local
#endif

extern ftlib_import_t FTLIB_IMPORT;

static inline void trap_Print( const char *msg )
{
	FTLIB_IMPORT.Print( msg );
}

static inline void trap_Error( const char *msg )
{
	FTLIB_IMPORT.Error( msg );
}

// cvars
static inline cvar_t *trap_Cvar_Get( const char *name, const char *value, int flags )
{
	return FTLIB_IMPORT.Cvar_Get( name, value, flags );
}

static inline cvar_t *trap_Cvar_Set( const char *name, const char *value )
{
	return FTLIB_IMPORT.Cvar_Set( name, value );
}

static inline void trap_Cvar_SetValue( const char *name, float value )
{
	FTLIB_IMPORT.Cvar_SetValue( name, value );
}

static inline cvar_t *trap_Cvar_ForceSet( const char *name, const char *value )
{
	return FTLIB_IMPORT.Cvar_ForceSet( name, value );
}

static inline float trap_Cvar_Value( const char *name )
{
	return FTLIB_IMPORT.Cvar_Value( name );
}

static inline const char *trap_Cvar_String( const char *name )
{
	return FTLIB_IMPORT.Cvar_String( name );
}

static inline int trap_Cmd_Argc( void )
{
	return FTLIB_IMPORT.Cmd_Argc();
}

static inline char *trap_Cmd_Argv( int arg )
{
	return FTLIB_IMPORT.Cmd_Argv( arg );
}

static inline char *trap_Cmd_Args( void )
{
	return FTLIB_IMPORT.Cmd_Args();
}

static inline void trap_Cmd_AddCommand( char *name, void ( *cmd )(void) )
{
	FTLIB_IMPORT.Cmd_AddCommand( name, cmd );
}

static inline void trap_Cmd_RemoveCommand( char *cmd_name )
{
	FTLIB_IMPORT.Cmd_RemoveCommand( cmd_name );
}

static inline void trap_Cmd_ExecuteText( int exec_when, char *text )
{
	FTLIB_IMPORT.Cmd_ExecuteText( exec_when, text );
}

static inline void trap_Cmd_Execute( void )
{
	FTLIB_IMPORT.Cmd_Execute();
}

// clock
static inline unsigned int trap_Milliseconds( void )
{
	return FTLIB_IMPORT.Sys_Milliseconds();
}

static inline uint64_t trap_Microseconds( void )
{
	return FTLIB_IMPORT.Sys_Microseconds();
}

static inline void *trap_LoadLibrary( char *name, dllfunc_t *funcs )
{
	return FTLIB_IMPORT.Sys_LoadLibrary( name, funcs );
}

static inline void trap_UnloadLibrary( void **lib )
{
	FTLIB_IMPORT.Sys_UnloadLibrary( lib );
}
