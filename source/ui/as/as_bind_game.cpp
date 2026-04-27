/*
Copyright (C) 2011 Victor Luchits

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

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"
#include "kernel/ui_demoinfo.h"
#include "kernel/ui_downloadinfo.h"
#include "as/asui.h"
#include "as/asui_local.h"
#include "../../steamshim/src/mod_steam.h"
#include "../../extern/stb/stb_ds.h"

namespace ASUI {

// dummy cGame class, single referenced, sorta like 'window' JS
class Game 
{
public:
	Game()
	{
	}
};

typedef WSWUI::DemoInfo DemoInfo;
typedef WSWUI::DownloadInfo DownloadInfo;

static Game dummyGame;

// =====================================================================================

void PrebindGame( ASInterface *as )
{
	ASBind::Class<Game, ASBind::class_singleref>( as->getEngine() );
}

static const DemoInfo & Game_GetDemoInfo( Game *game )
{
	return *UI_Main::Get()->getDemoInfo();
}

static asstring_t *Game_Name( Game *game )
{
	return ASSTR( trap::Cvar_String( "gamename" ) );
}

static asstring_t *Game_Version( Game *game )
{
	return ASSTR( trap::Cvar_String( "version" ) );
}

static asstring_t *Game_Revision( Game *game )
{
	return ASSTR( trap::Cvar_String( "revision" ) );
}

static asstring_t *Game_ServerName( Game *game )
{
	return ASSTR( UI_Main::Get()->getServerName() );
}

static asstring_t *Game_RejectMessage( Game *game )
{
	return ASSTR( UI_Main::Get()->getRejectMessage() );
}

static const DownloadInfo & Game_GetDownloadInfo( Game *game )
{
	return *UI_Main::Get()->getDownloadInfo();
}

static asstring_t *Game_ConfigString( Game *game, int cs )
{
	char configstring[MAX_CONFIGSTRING_CHARS];

	if( cs < 0 || cs >= MAX_CONFIGSTRINGS ) {
		Com_Printf( S_COLOR_RED "Game_ConfigString: bogus configstring index: %i", cs );
		return ASSTR( "" );
	}

	trap::GetConfigString( cs, configstring, sizeof( configstring ) );
	return ASSTR( configstring );
}

static asstring_t *Game_Cvar( Game *game, asstring_t* name )
{
	return ASSTR (trap::Cvar_String(name->buffer));
}

static void Game_SteamOpenProfile( Game *game, asstring_t* steamid )
{
	struct steam_id_rpc_s request;
	request.cmd = RPC_ACTIVATE_OVERLAY;
	request.id = atoll( steamid->buffer );
	STEAMSHIM_sendRPC( &request, sizeof( struct steam_id_rpc_s ), NULL, NULL, NULL);
}

static bool Game_SteamAvaliable( Game *game )
{
	return STEAMSHIM_active();
}

static void Game_WorkshopRefresh( Game *game )
{
	Steam_RefreshWorkshopMods();
}

// Returns one entry per local (publishable) mod: "name\tindex\n"
// index is the position in Steam_GetWorkshopMods() so workshopSubmitMap can look it up.
static asstring_t *Game_WorkshopLocalMods( Game *game )
{
	const struct steam_workshop_mod_s *mods = Steam_GetWorkshopMods();
	size_t count = Steam_GetWorkshopModCount();
	std::ostringstream stream;
	for( size_t i = 0; i < count; i++ ) {
		if( !mods[i].is_local )
			continue;
		const char *name = mods[i].name ? mods[i].name : ( mods[i].path ? mods[i].path : "" );
		stream << name << "\t" << i << "\n";
	}
	return ASSTR( stream.str() );
}

static asstring_t *Game_WorkshopSubmitMap( Game *game, int modIndex, const asstring_t &title,
	const asstring_t &description, const asstring_t &tags,
	const asstring_t &visibility, const asstring_t &changeNote )
{

	struct steam_workshop_publish_s params;
	params.title       = title.buffer;
	params.description = description.buffer;
	params.tags        = tags.buffer;
	params.visibility  = visibility.buffer;
	params.change_note = changeNote.buffer;

	struct steam_workshop_publish_result_s result = Steam_PublishLocalMod( modIndex, &params );

	switch( result.res ) {
		case STEAM_PUBLISH_OK: {
			std::ostringstream stream;
			stream << "Workshop item " << ( result.is_new_item ? "published" : "updated" )
			       << " (" << result.file_id << ").";
			return ASSTR( stream.str() );
		}
		case STEAM_PUBLISH_ERR_STEAM_UNAVAILABLE:  return ASSTR( "Steam is not available." );
		case STEAM_PUBLISH_ERR_TITLE_REQUIRED:     return ASSTR( "Title is required." );
		case STEAM_PUBLISH_ERR_INVALID_MOD:        return ASSTR( "Invalid mod selection." );
		case STEAM_PUBLISH_ERR_TOO_MANY_TAGS:      return ASSTR( "Too many tags." );
		case STEAM_PUBLISH_ERR_CREATE_FAILED:      return ASSTR( "Creating workshop item failed." );
		case STEAM_PUBLISH_ERR_UPDATE_FAILED:      return ASSTR( "Starting workshop update failed." );
		case STEAM_PUBLISH_ERR_SET_TITLE:          return ASSTR( "Setting workshop title failed." );
		case STEAM_PUBLISH_ERR_SET_DESCRIPTION:    return ASSTR( "Setting workshop description failed." );
		case STEAM_PUBLISH_ERR_SET_CONTENT:        return ASSTR( "Setting workshop content directory failed." );
		case STEAM_PUBLISH_ERR_SET_PREVIEW:        return ASSTR( "Setting workshop preview image failed." );
		case STEAM_PUBLISH_ERR_SET_VISIBILITY:     return ASSTR( "Setting workshop visibility failed." );
		case STEAM_PUBLISH_ERR_SET_TAGS:           return ASSTR( "Setting workshop tags failed." );
		case STEAM_PUBLISH_ERR_SUBMIT_FAILED:      return ASSTR( "Submitting workshop item failed." );
		default:                                   return ASSTR( "Unknown error." );
	}
}

static int Game_ClientState( Game *game )
{
	return UI_Main::Get()->getRefreshState().clientState;
}

static int Game_ServerState( Game *game )
{
	return UI_Main::Get()->getRefreshState().serverState;
}

static void Game_Exec( Game *game, const asstring_t &cmd )
{
	trap::Cmd_ExecuteText( EXEC_NOW, cmd.buffer );
}

static void Game_ExecAppend( Game *game, const asstring_t &cmd )
{
	trap::Cmd_ExecuteText( EXEC_APPEND, cmd.buffer );
}

static void Game_ExecInsert( Game *game, const asstring_t &cmd )
{
	trap::Cmd_ExecuteText( EXEC_INSERT, cmd.buffer );
}

static int Game_PlayerNum( Game *game )
{
	return trap::CL_PlayerNum();
}

static bool Game_isTV( Game *game )
{
	char tv[MAX_CONFIGSTRING_CHARS];

	trap::GetConfigString( CS_TVSERVER, tv, sizeof( tv ) );

	return atoi( tv ) != 0;
}

void BindGame( ASInterface *as )
{
	ASBind::Enum( as->getEngine(), "eConfigString" )
		( "CS_TVSERVER", CS_TVSERVER )
		( "CS_MODMANIFEST", CS_MODMANIFEST )
		( "CS_MESSAGE", CS_MESSAGE )
		( "CS_USESTEAMAUTH", CS_USESTEAMAUTH )
		( "CS_MAPNAME", CS_MAPNAME )
		( "CS_AUDIOTRACK", CS_AUDIOTRACK )
		( "CS_HOSTNAME", CS_HOSTNAME )
		( "CS_GAMETYPETITLE", CS_GAMETYPETITLE )
		( "CS_GAMETYPENAME", CS_GAMETYPENAME )
		( "CS_GAMETYPEVERSION", CS_GAMETYPEVERSION )
		( "CS_GAMETYPEAUTHOR", CS_GAMETYPEAUTHOR )
		( "CS_TEAM_ALPHA_NAME", CS_TEAM_ALPHA_NAME )
		( "CS_TEAM_BETA_NAME", CS_TEAM_BETA_NAME )
		( "CS_MATCHNAME", CS_MATCHNAME )
		( "CS_MATCHSCORE", CS_MATCHSCORE )
		( "CS_ACTIVE_CALLVOTE", CS_ACTIVE_CALLVOTE )
		( "CS_ACTIVE_CALLVOTE_VOTES", CS_ACTIVE_CALLVOTE_VOTES )
	;

	ASBind::Enum( as->getEngine(), "eClientState" )
		( "CA_UNITIALIZED", CA_UNINITIALIZED )
		( "CA_DISCONNECTED", CA_DISCONNECTED )
		( "CA_GETTING_TICKET", CA_GETTING_TICKET )
		( "CA_CONNECTING", CA_CONNECTING )
		( "CA_HANDSHAKE", CA_HANDSHAKE )
		( "CA_CONNECTED", CA_CONNECTED )
		( "CA_LOADING", CA_LOADING )
		( "CA_ACTIVE", CA_ACTIVE )
	;

	ASBind::Enum( as->getEngine(), "eDropReason" )
		( "DROP_REASON_CONNFAILED", DROP_REASON_CONNFAILED )
		( "DROP_REASON_CONNTERMINATED", DROP_REASON_CONNTERMINATED )
		( "DROP_REASON_CONNERROR", DROP_REASON_CONNERROR )
		;

	ASBind::Enum( as->getEngine(), "eDropType" )
		( "DROP_TYPE_GENERAL", DROP_TYPE_GENERAL )
		( "DROP_TYPE_PASSWORD", DROP_TYPE_PASSWORD )
		( "DROP_TYPE_NORECONNECT", DROP_TYPE_NORECONNECT )
		( "DROP_TYPE_TOTAL", DROP_TYPE_TOTAL )
		;

	ASBind::Enum( as->getEngine(), "eDownloadType" )
		( "DOWNLOADTYPE_NONE", DOWNLOADTYPE_NONE )
		( "DOWNLOADTYPE_SERVER", DOWNLOADTYPE_SERVER )
		( "DOWNLOADTYPE_WEB", DOWNLOADTYPE_WEB )
		;

	ASBind::GetClass<Game>( as->getEngine() )
		// gives access to properties and controls of the currently playing demo instance
		.constmethod( Game_GetDemoInfo, "get_demo", true )

		.constmethod( Game_Name, "get_name", true )
		.constmethod( Game_Version, "get_version", true )
		.constmethod( Game_Revision, "get_revision", true )

		.constmethod( Game_ConfigString, "configString", true )
		.constmethod( Game_ConfigString, "cs", true )
		.constmethod( Game_Cvar, "cvar", true )

		.constmethod( Game_SteamOpenProfile, "steamopenprofile", true )
		.constmethod( Game_SteamAvaliable, "steamAvaliable", true )
		.constmethod( Game_WorkshopRefresh, "workshopRefresh", true )
		.constmethod( Game_WorkshopLocalMods, "workshopLocalMods", true )
		.method2( Game_WorkshopSubmitMap,
			"String @workshopSubmitMap( int modIndex, const String &in title, const String &in description, const String &in tags, const String &in visibility, const String &in changeNote ) const",
			true )

		.constmethod( Game_PlayerNum, "get_playerNum", true )

		.constmethod( Game_ClientState, "get_clientState", true )
		.constmethod( Game_ServerState, "get_serverState", true )

		.constmethod( Game_Exec, "exec", true )
		.constmethod( Game_ExecAppend, "execAppend", true )
		.constmethod( Game_ExecInsert, "execInsert", true )

		.constmethod( Game_ServerName, "get_serverName", true )
		.constmethod( Game_RejectMessage, "get_rejectMessage", true )
		.constmethod( Game_GetDownloadInfo, "get_download", true )

		.constmethod( Game_isTV, "get_isTV", true )
	;
}

void BindGameGlobal( ASInterface *as )
{
	ASBind::Global( as->getEngine() )
		// global variable
		.var( &dummyGame, "game" )
	;
}

}

ASBIND_TYPE( ASUI::Game, Game );
