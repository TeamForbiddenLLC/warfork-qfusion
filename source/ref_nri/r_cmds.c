/*
Copyright (C) 2014 Victor Luchits

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

// r_cmds.c
#include "r_local.h"
#include "r_frontend.h"
#include "../qalgo/glob.h"

#include "../qcommon/mod_cmd.h"
#include "../qcommon/mod_cvar.h"
#include "ri_pogoBuffer.h"


/*
 * R_Localtime
 */
static struct tm *R_Localtime( const time_t time, struct tm* _tm )
{
#ifdef _WIN32
	struct tm* __tm = localtime( &time );
	*_tm = *__tm;
#else
	localtime_r( &time, _tm );
#endif
	return _tm;
}

/*
* R_ScreenShot_f
*/
void R_ScreenShot_f( void )
{
	int i;
	const char *name;
	const char *mediadir;
	size_t path_size;
	char *path;
	char timestamp_str[MAX_QPATH];
	struct tm newtime;
	
	R_Localtime( time( NULL ), &newtime );

	name = Cmd_Argv( 1 );

	mediadir = FS_MediaDirectory( FS_MEDIA_IMAGES );
	if( mediadir )
	{
		path_size = strlen( mediadir ) + 1 /* '/' */ + strlen( glConfig.applicationName ) + 1 /* '/' */ + 1;
		path = alloca( path_size );
		Q_snprintfz( path, path_size, "%s/%s/", mediadir, glConfig.applicationName );
	}
	else
	{
		path_size = strlen( FS_WriteDirectory() ) + 1 /* '/' */ + strlen( FS_GameDirectory() ) + strlen( "/screenshots/" ) + 1;
		path = alloca( path_size );
		Q_snprintfz( path, path_size, "%s/%s/screenshots/", FS_WriteDirectory(), FS_GameDirectory() );
	}
	
	// validate timestamp string
	for( i = 0; i < 2; i++ )
	{
		strftime( timestamp_str, sizeof( timestamp_str ), r_screenshot_fmtstr->string, &newtime );
		if( !COM_ValidateRelativeFilename( timestamp_str ) )
			Cvar_ForceSet( r_screenshot_fmtstr->name, r_screenshot_fmtstr->dvalue );
		else
			break;
	}
	
	// hm... shouldn't really happen, but check anyway
	if( i == 2 )
		Cvar_ForceSet( r_screenshot_fmtstr->name, glConfig.screenshotPrefix );

	RF_ScreenShot( path, name, r_screenshot_fmtstr->string,
				  Cmd_Argc() >= 3 && !Q_stricmp( Cmd_Argv( 2 ), "silent" ) ? true : false );
}

/*
 * R_TakeEnvShot
 */
void R_TakeEnvShot(struct FrameState_s* cmd, const char *path, const char *name, unsigned maxPixels )
{
	// Never ported to ref_nri: this needs the six cubemap faces rendered offscreen, which the backend
	// has no path for yet. It used to abort here instead of saying so.
	Com_Printf( "envshot: not implemented under ref_nri\n" );
}

/*
* R_EnvShot_f
*/
void R_EnvShot_f( void )
{
	const char *writedir, *gamedir;
	int path_size;
	char *path;

	if( !rsh.worldModel )
		return;

	if( Cmd_Argc() != 3 )
	{
		Com_Printf( "usage: envshot <name> <size>\n" );
		return;
	}

	writedir = FS_WriteDirectory();
	gamedir = FS_GameDirectory();
	path_size = strlen( writedir ) + 1 + strlen( gamedir ) + 1 + strlen( "env/" ) + 1;
	path = alloca( path_size );
	Q_snprintfz( path, path_size, "%s/%s/env/", writedir, gamedir );

	RF_EnvShot( path, Cmd_Argv( 1 ), atoi( Cmd_Argv( 2 ) ) );
}

/*
* R_GlobFilter
*/
static bool R_GlobFilter( const char *pattern, const char *value )
{
	if( *pattern && !glob_match( pattern, value, 0 ) ) {
		return false;
	}
	return true;
}

/*
* R_ImageList_f
*/
void R_ImageList_f( void )
{
	R_PrintImageList( Cmd_Argv( 1 ), R_GlobFilter );
}

/*
* R_ShaderList_f
*/
void R_ShaderList_f( void )
{
	R_PrintShaderList( Cmd_Argv( 1 ), R_GlobFilter );
}

/*
* R_ShaderDump_f
*/
void R_ShaderDump_f( void )
{
	const char *name;
	const msurface_t *debugSurface;
	
	debugSurface = R_GetDebugSurface();

	if( (Cmd_Argc() < 2) && !debugSurface )
	{
		Com_Printf( "Usage: %s [name]\n", Cmd_Argv(0) );
		return;
	}

	if( Cmd_Argc() < 2 )
		name = debugSurface->shader->name;
	else
		name = Cmd_Argv( 1 );

	R_PrintShaderCache( name );
}
