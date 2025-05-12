/*
Copyright (C) 2023 - Team Forbidden LLC.
Copyright (C) 2023 - Paril
Copyright (C) 2023 - MSC

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

#include "client.h"
#include "../gameshared/gs_public.h"
#include "../steamshim/src/packet_utils.h"
#include "../steamshim/src/parent/parent.h"

#include "discord_register.h"
#include "discord_rpc.h"

#define DISCORD_APP_ID 1074038981802598491
#define DISCORD_PARTY_PRIVATE 0
#define DISCORD_PARTY_PUBLIC 1
#define G_STRINGIFY( macro_or_string ) G_STRINGIFY_ARG( macro_or_string )
#define G_STRINGIFY_ARG( contents ) #contents

enum EDiscordResult {
	DiscordResult_Ok,
	DiscordResult_ServiceUnavailable,
	DiscordResult_InvalidVersion,
	DiscordResult_LockFailed,
	DiscordResult_InternalError,
	DiscordResult_InvalidPayload,
	DiscordResult_InvalidCommand,
	DiscordResult_InvalidPermissions,
	DiscordResult_NotFetched,
	DiscordResult_NotFound,
	DiscordResult_Conflict,
	DiscordResult_InvalidSecret,
	DiscordResult_InvalidJoinSecret,
	DiscordResult_NoEligibleActivity,
	DiscordResult_InvalidInvite,
	DiscordResult_NotAuthenticated,
	DiscordResult_InvalidAccessToken,
	DiscordResult_ApplicationMismatch,
	DiscordResult_InvalidDataUrl,
	DiscordResult_InvalidBase64,
	DiscordResult_NotFiltered,
	DiscordResult_LobbyFull,
	DiscordResult_InvalidLobbySecret,
	DiscordResult_InvalidFilename,
	DiscordResult_InvalidFileSize,
	DiscordResult_InvalidEntitlement,
	DiscordResult_NotInstalled,
	DiscordResult_NotRunning,
	DiscordResult_InsufficientBuffer,
	DiscordResult_PurchaseCanceled,
	DiscordResult_InvalidGuild,
	DiscordResult_InvalidEvent,
	DiscordResult_InvalidChannel,
	DiscordResult_InvalidOrigin,
	DiscordResult_RateLimited,
	DiscordResult_OAuth2Error,
	DiscordResult_SelectChannelTimeout,
	DiscordResult_GetGuildTimeout,
	DiscordResult_SelectVoiceForceRequired,
	DiscordResult_CaptureShortcutAlreadyListening,
	DiscordResult_UnauthorizedForAchievement,
	DiscordResult_InvalidGiftCode,
	DiscordResult_PurchaseError,
	DiscordResult_TransactionAborted
};

/*
 * CL_DiscordErrorString
 */
static const char *CL_DiscordErrorString( enum EDiscordResult result )
{
	switch( result ) {
		case DiscordResult_Ok:
			return "Ok";
		case DiscordResult_ServiceUnavailable:
			return "Service Unavailable";
		case DiscordResult_InvalidVersion:
			return "Invalid Version";
		case DiscordResult_LockFailed:
			return "Lock Failed";
		case DiscordResult_InternalError:
			return "Internal Error";
		case DiscordResult_InvalidPayload:
			return "Invalid Payload";
		case DiscordResult_InvalidCommand:
			return "Invalid Command";
		case DiscordResult_InvalidPermissions:
			return "Invalid Permissions";
		case DiscordResult_NotFetched:
			return "Not Fetched";
		case DiscordResult_NotFound:
			return "Not Found";
		case DiscordResult_Conflict:
			return "Conflict";
		case DiscordResult_InvalidSecret:
			return "Invalid Secret";
		case DiscordResult_InvalidJoinSecret:
			return "Invalid Join Secret";
		case DiscordResult_NoEligibleActivity:
			return "No Eligible Activity";
		case DiscordResult_InvalidInvite:
			return "Invalid Invite";
		case DiscordResult_NotAuthenticated:
			return "Not Authenticated";
		case DiscordResult_InvalidAccessToken:
			return "Invalid Access Token";
		case DiscordResult_ApplicationMismatch:
			return "Application Mismatch";
		case DiscordResult_InvalidDataUrl:
			return "Invalid Data Url";
		case DiscordResult_InvalidBase64:
			return "Invalid Base64";
		case DiscordResult_NotFiltered:
			return "Not Filtered";
		case DiscordResult_LobbyFull:
			return "Lobby Full";
		case DiscordResult_InvalidLobbySecret:
			return "Invalid Lobby Secret";
		case DiscordResult_InvalidFilename:
			return "Invalid Filename";
		case DiscordResult_InvalidFileSize:
			return "Invalid File Size";
		case DiscordResult_InvalidEntitlement:
			return "Invalid Entitlement";
		case DiscordResult_NotInstalled:
			return "Not Installed";
		case DiscordResult_NotRunning:
			return "Not Running";
		case DiscordResult_InsufficientBuffer:
			return "Insufficient Buffer";
		case DiscordResult_PurchaseCanceled:
			return "Purchase Canceled";
		case DiscordResult_InvalidGuild:
			return "Invalid Guild";
		case DiscordResult_InvalidEvent:
			return "Invalid Event";
		case DiscordResult_InvalidChannel:
			return "Invalid Channel";
		case DiscordResult_InvalidOrigin:
			return "Invalid Origin";
		case DiscordResult_RateLimited:
			return "Rate Limited";
		case DiscordResult_OAuth2Error:
			return "OAuth2 Error";
		case DiscordResult_SelectChannelTimeout:
			return "Select Channel Timeout";
		case DiscordResult_GetGuildTimeout:
			return "Get Guild Timeout";
		case DiscordResult_SelectVoiceForceRequired:
			return "Select Voice Force Required";
		case DiscordResult_CaptureShortcutAlreadyListening:
			return "Capture Shortcut Already Listening";
		case DiscordResult_UnauthorizedForAchievement:
			return "Unauthorized For Achievement";
		case DiscordResult_InvalidGiftCode:
			return "Invalid Gift Code";
		case DiscordResult_PurchaseError:
			return "Purchase Error";
		case DiscordResult_TransactionAborted:
			return "Transaction Aborted";
	}

	return "Unknown Error ID";
}

static const char *valid_maps[] = {
	"13vast",
	"36dm2_b1",
	"36dm4",
	"36dm5",
	"3hours",
	"3v11",
	"3v11_b4",
	"50u1ca1",
	"50u1ca100",
	"50u1ca101",
	"abandoned",
	"acid3dm10",
	"acid3dm5",
	"acid3dm7",
	"acid3dm9",
	"acidwctf2",
	"acidwdm11",
	"acidwdm2",
	"acidwdm6",
	"actdm1",
	"ae",
	"aerorun",
	"air-nasa",
	"airfun",
	"alley",
	"alley_cr",
	"antistat",
	"aow_fragland_b7",
	"aow-fragland_b5",
	"aow-fragland_b7-1",
	"army",
	"b1601763810",
	"baby_wca1",
	"billyhill",
	"blood_covenant",
	"boot_camp_b3",
	"btmid1_004",
	"btmid1_004_i",
	"btmid2_002",
	"btmid2_002_i",
	"btmid4_001",
	"btmid4_001_i",
	"cal_ca1",
	"cal_da1",
	"cal_da3",
	"capturecity_coder",
	"carbon",
	"ccdm1-tekktonic",
	"chiropteradm",
	"cht2",
	"claustrophobia",
	"cocaine_b1",
	"coldwarctf",
	"criziswdm3",
	"ct3dm4",
	"ct3tourney2",
	"ctctf6",
	"ctf_kothet",
	"ctf_kothet_cc",
	"cunete",
	"cure",
	"cwbomb1",
	"cwctf1",
	"cwl1",
	"cwl2",
	"cwl3",
	"cwl4",
	"cwl5",
	"cwm1",
	"cwm2",
	"cwm3",
	"cwm4",
	"cwm5",
	"cwm6",
	"cwm7",
	"cwrace1",
	"cwrace2",
	"cwrace3",
	"cwrace4",
	"cwrace5",
	"cws1",
	"cws2",
	"cxbomb2b",
	"d3xf1",
	"darkbomb1",
	"de06",
	"debonaire",
	"decompression",
	"decompression00",
	"decompression01",
	"dm_etages_b",
	"downtown",
	"duel_boogyvan",
	"emctf1",
	"emsummer",
	"emtown",
	"emwarehouse",
	"etramphi",
	"etramphi2",
	"etramphi3",
	"face",
	"factory_beta",
	"feros",
	"fi_ctf1m",
	"focal_p132",
	"fr3dm1",
	"fragcity_coder",
	"furiousheights",
	"giantshome",
	"goldleaf",
	"gork",
	"gwsw3aprefinal",
	"haduca_1",
	"hesus_ra",
	"hylith_ctc1_b5",
	"icepart2",
	"ictfmoar",
	"ik3dm2",
	"industrial",
	"infinifinal",
	"infinirace",
	"infinirace_covered",
	"instactf1",
	"instactf1-x",
	"ionicsslidehouse",
	"jerms_beta1",
	"jerms_ca1",
	"jerms_ca2",
	"jerms_ca3",
	"jerms_ca4",
	"jerms_ctf2",
	"jerms_da1",
	"jerms_da2",
	"jerms_da4",
	"jerms_da5_1",
	"jerms_da5_2",
	"jerms_da5_3",
	"jerms_ica1",
	"jihras",
	"jof3dm2",
	"josher_b1",
	"kikimid_b1",
	"lawbomb1b3",
	"lawbombb2",
	"lazurab1",
	"lg",
	"lltdm1v2",
	"lltdm1v350",
	"lolface",
	"losthope",
	"lsdm1",
	"lulz",
	"lun3_20b1",
	"lun3dm5",
	"malicious",
	"midair_b3",
	"midair_b5",
	"midair_b6",
	"moddm17",
	"motoville",
	"mxl_school",
	"necro6",
	"nodm14",
	"outpost",
	"outpost-rf",
	"overkill",
	"ozbomb",
	"partdm1",
	"paswdm1",
	"paswdm1_b2",
	"paswdm1_b2_sounds",
	"plduel2",
	"pressure",
	"pro-ct3tourney2",
	"psl_koth",
	"pukka3tourney3",
	"pukka3tourney7",
	"qfraggel3ffa",
	"qfraggel3tdm",
	"rab3_duel1_v1",
	"rab3dm1",
	"rab3dm1_b1",
	"raketliga_slide",
	"rats_waitingroom",
	"reactors",
	"reactors_2",
	"reqbath",
	"reqkitchen",
	"retard2",
	"return",
	"rota3ctf1",
	"rota3ctf2",
	"scduel1a3",
	"seasoned",
	"sf-xmas08",
	"sfkoth1_b1",
	"shepas",
	"shiz_q1dm2",
	"shredder",
	"sinister",
	"sinjage_",
	"skyscrapers",
	"slide_air_b1",
	"slide_air_b2",
	"slide_arena_v3",
	"slide_baby_v7",
	"slide_bipbeta_dirty",
	"slide_flat",
	"slide_flat_v2",
	"slide_flat_v3",
	"slide_nyan_b1",
	"slide_nyan_b4",
	"slide_nyan_b6",
	"slide_tortus_b1",
	"slidehouse",
	"slidehouse_lava",
	"slidehouse_lava_b1",
	"slidehouse_lava_lol",
	"sohca1",
	"sokar3dm5",
	"sparth",
	"speedyctf",
	"stormsector7_alpha3",
	"summerbeta",
	"svartholma",
	"t3ch",
	"theedged",
	"tlebomb1",
	"tlebomb1a",
	"tlebomb2",
	"torn",
	"tournw1",
	"trespass",
	"undergroundb2",
	"unitooldm6",
	"wca1xmas",
	"wca3",
	"wcai",
	"wctc1_b2",
	"wctc2",
	"wfa1insta",
	"wfamphi1",
	"wfbomb1",
	"wfbomb2",
	"wfbomb3",
	"wfbomb4",
	"wfbomb5",
	"wfbomb6",
	"wfca1",
	"wfca2",
	"wfctf1",
	"wfctf2",
	"wfctf3",
	"wfctf4",
	"wfctf5",
	"wfctf6",
	"wfda1",
	"wfda2",
	"wfda3",
	"wfda4",
	"wfda5",
	"wfdm1",
	"wfdm10",
	"wfdm11",
	"wfdm12",
	"wfdm13",
	"wfdm14",
	"wfdm15",
	"wfdm16",
	"wfdm17",
	"wfdm18",
	"wfdm19",
	"wfdm2",
	"wfdm20",
	"wfdm3",
	"wfdm4",
	"wfdm5",
	"wfdm6",
	"wfdm7",
	"wfdm8",
	"wfdm9",
	"wfrace1",
	"wftutorial1",
	"wsw00021",
	"wsw00049",
	"xfdm2",
	"xmas_river",
	"ztn2dm2",
	"ztn2dm3",
	"zxymid1",
};

/*
 * CL_DiscordDisconnected
 */
static void CL_DiscordDisconnected( int errorCode, const char *message )
{
	if( cl_discord_errors->integer ) {
	Com_Printf( S_COLOR_RED "Discord error %s: %s\n", CL_DiscordErrorString( errorCode ), message );
	}
}

typedef struct RichPresence {
	char state[128];
	char details[128];
	int64_t startTimestamp;
	int64_t endTimestamp;
	char largeImageKey[32];
	char largeImageText[128];
	char smallImageKey[32];
	char smallImageText[128];
	char partyId[128];
	int partySize;
	int partyMax;
	int partyPrivacy;
	char matchSecret[128];
	char joinSecret[128];
	char spectateSecret[128];
	int8_t instance;
} RichPresence;

typedef struct {
	_Bool discord_initialized;
	unsigned int next_update;
	RichPresence discord_old_presence;
	RichPresence steam_old_presence;
} cl_presence_state_t;

static cl_presence_state_t cl_presence_state;

void UpdatePresenceIfChanged( RichPresence presence )
{
	if( memcmp( &cl_presence_state.steam_old_presence, &presence, sizeof( presence ) ) != 0 && STEAMSHIM_active()) {
		char rpc_packet[sizeof( struct buffer_rpc_s ) + 256];
		struct buffer_rpc_s *req = (struct buffer_rpc_s *)rpc_packet;
		req->cmd = RPC_SET_RICH_PRESENCE;
		char steam_display[128];
		Q_snprintfz( steam_display, sizeof steam_display, "%s | %s", presence.state, presence.details );

		{
			const char *pairs[] = { "score", steam_display };
			const size_t bufferLen = pack_cstr_null_terminated( (char*)req->buf, 256, pairs, 2 );
			STEAMSHIM_sendRPC( req, bufferLen + sizeof( struct buffer_rpc_s ), NULL, NULL, NULL);
		}
		
		{
			const char *pairs[] = { "details", presence.details};
			const size_t bufferLen = pack_cstr_null_terminated( (char *)req->buf, 256, pairs, 2 );
			STEAMSHIM_sendRPC( req, bufferLen + sizeof( struct buffer_rpc_s ), NULL, NULL, NULL);
		}
		
		{
			const char *pairs[] = { "steam_display", "#Status_Score"};
			const size_t bufferLen = pack_cstr_null_terminated( (char *)req->buf, 256, pairs, 2 );
			STEAMSHIM_sendRPC( req, bufferLen + sizeof( struct buffer_rpc_s ), NULL, NULL, NULL);
		}

		cl_presence_state.steam_old_presence = presence;
	}

	if( memcmp( &cl_presence_state.discord_old_presence, &presence, sizeof( presence ) ) != 0 && cl_presence_state.discord_initialized) {
		DiscordRichPresence discord = { 0 };
		discord.state = presence.state;
		discord.details = presence.details;
		discord.startTimestamp = presence.startTimestamp;
		discord.endTimestamp = presence.endTimestamp;
		discord.largeImageKey = presence.largeImageKey;
		discord.largeImageText = presence.largeImageText;
		discord.smallImageKey = presence.smallImageKey;
		discord.smallImageText = presence.smallImageText;
		discord.partyId = presence.partyId;
		discord.partySize = presence.partySize;
		discord.partyMax = presence.partyMax;
		discord.partyPrivacy = presence.partyPrivacy;
		discord.matchSecret = presence.matchSecret;
		discord.joinSecret = presence.joinSecret;
		discord.spectateSecret = presence.spectateSecret;
		discord.instance = presence.instance;

		Discord_UpdatePresence( &discord );
		cl_presence_state.discord_old_presence = presence;
	}

}

/*
 * CL_DiscordReady
 */
static void CL_DiscordReady( const DiscordUser *user )
{
	const char *discord_id;
	discord_id = user->userId;

	Com_Printf( "Loading Discord module... (%s)\n", discord_id );
	Cvar_ForceSet( "discord_id", discord_id );


	Discord_RegisterSteamGame(G_STRINGIFY(DISCORD_APP_ID), G_STRINGIFY(APP_STEAMID));
	cl_presence_state.discord_initialized = true;
}

/*
 * CL_PlayerStatus
 */

static const char *CL_PlayerStatus( snapshot_t *frame )
{
	if( cls.sv_tv ) {
		return va( "tv" );
	} else if( cls.demo.playing ) {
		return va( "demo" );
	} else if( frame->playerState.stats[STAT_REALTEAM] == TEAM_SPECTATOR ) {
		return va( "spectating" );		
	} else if( frame->gameState.stats[GAMESTAT_MATCHSTATE] == MATCH_STATE_WARMUP ) {
		return va( "warmup" ); // Only applicable if you're playing.
	} else if( frame->gameState.stats[GAMESTAT_FLAGS] & GAMESTAT_FLAG_PAUSED ) {
		return va( "timeout" ); // Only applicable if you're playing.
	} else if( frame->gameState.stats[GAMESTAT_MATCHSTATE] == MATCH_STATE_POSTMATCH ) {
		return va( "gameover" ); // Only applicable if you're playing.
	} else
		return va( "playing" );
}

/*
 * CL_UpdatePresence
 */

void CL_UpdatePresence( void )
{
	if( cl_presence_state.discord_initialized || STEAMSHIM_active() ) {
		unsigned int now = Sys_Milliseconds();
		if( cl_presence_state.next_update <= now ) {
			// Discord rate limit is 15s, but this has been tested and is fine!
			cl_presence_state.next_update = now + 1000;

			RichPresence presence = { 0 };
			snapshot_t *frame = &cl.snapShots[cl.currentSnapNum & UPDATE_MASK];

			char details[128];

			if( cls.state == CA_ACTIVE ) {
				const size_t num_valid_maps = sizeof( valid_maps ) / sizeof( valid_maps[0] );
				const char *mapname = cl.configstrings[CS_MAPNAME];
				bool valid_map = false;
				for( size_t i = 0; i < num_valid_maps; i++ ) {
					if( !Q_stricmp( mapname, valid_maps[i] ) ) {
						valid_map = true;
						break;
					}
				}

				char sanitized_mapname[25] = {0};
				strncpy(sanitized_mapname, mapname, 24);

				if (strlen(sanitized_mapname) == 1){
					sanitized_mapname[1] = ' '; // discord rpc HATES 1 character strings (but for some reason arRPC handles them fine?)
				}

				char sanitized_hostname[45] = {0};
				strncpy(sanitized_hostname, cl.configstrings[CS_HOSTNAME], 44);

				if (strlen(sanitized_hostname) == 1){
					sanitized_hostname[1] = ' ';
				}


				char levelshot_id[25];
				strcpy(levelshot_id, sanitized_mapname);
				Q_strlwr(levelshot_id);

				strcpy( presence.largeImageKey, valid_map ? levelshot_id : "unknownmap" ); // Levelshot
				strcpy( presence.largeImageText, COM_RemoveColorTokens(sanitized_hostname) );	  // Server name
				strcpy( presence.smallImageKey, CL_PlayerStatus( frame ) );
				strcpy( presence.smallImageText, CL_PlayerStatus( frame ) );
				strcpy( presence.state, COM_RemoveColorTokens(sanitized_mapname)); // Map name

				// Gametype and Score (if available)
				if( cl.configstrings[CS_GAMETYPENAME][0] || cl.configstrings[CS_MATCHSCORE][0] ) {
					// Discord does not like a 'details' of less than 2 characters
					Q_snprintfz( presence.details, sizeof( details ), "%s %s", cl.configstrings[CS_GAMETYPENAME], cl.configstrings[CS_MATCHSCORE] );
				} else {
					presence.details[0] = '\0';
				}

				strcpy( presence.partyId, sanitized_hostname );

				// If server is not localhost
				if( !NET_IsLocalAddress( &cls.serveraddress ) ) {
					strcpy( presence.joinSecret, NET_AddressToString( &cls.serveraddress ) );
				}

				presence.partySize = frame->numplayers;						 // Total clients connected
				presence.partyMax = atoi( cl.configstrings[CS_MAXCLIENTS] ); // Max clients
				presence.instance = true;

                {
					time_t unix_time;
					time( &unix_time );

					unsigned int cur_time = frame->serverTime;
					unsigned int clock_override = frame->gameState.longstats[GAMELONG_CLOCKOVERRIDE];
					unsigned int start_time = frame->gameState.longstats[GAMELONG_MATCHSTART];
					unsigned int duration = frame->gameState.longstats[GAMELONG_MATCHDURATION];
					unsigned int elapsed = cur_time - start_time;

					if( clock_override != 0 ) {
						// assume counting down, can't tell afaik
						presence.endTimestamp = unix_time + clock_override / 1000;
					} else if( duration == 0 ) {
						// count up
						presence.startTimestamp = unix_time - elapsed / 1000;
					} else {
						// count down
						unsigned int remaining = duration - elapsed;
						presence.endTimestamp = unix_time + remaining / 1000;
					}
				}

			} else if( cls.state > CA_DISCONNECTED && cls.state < CA_ACTIVE ) {
				strcpy( presence.largeImageKey, "connecting" );
				strcpy( presence.state, "Attempting to fork!" );
				strcpy( presence.details, "Connecting" );
			} else {
				strcpy( presence.largeImageKey, "mainmenu" );
				strcpy( presence.state, "Not ready to fork!" );
				strcpy( presence.details, "Main Menu" );
			}

			UpdatePresenceIfChanged( presence );
		}
	}
	Discord_RunCallbacks();
}

/*
 * CL_DiscordJoinGame
 */
static void CL_DiscordJoinGame( const char *secret )
{
	char cmd[256];
	Q_snprintfz( cmd, sizeof( cmd ), "connect %s", secret );
	Cbuf_ExecuteText( EXEC_NOW, cmd );
}

/*
 * CL_DiscordJoinRequest
 */
static void CL_DiscordJoinRequest( const DiscordUser *request )
{
	// TODO: display UI for this
	Discord_Respond( request->userId, DISCORD_REPLY_YES );
}

/*
 * CL_InitDiscord
 */
void CL_InitDiscord( void )
{
	static DiscordEventHandlers handlers = {
		.ready = CL_DiscordReady,
		.disconnected = CL_DiscordDisconnected,
		.joinGame = CL_DiscordJoinGame,
		.joinRequest = CL_DiscordJoinRequest,
		.errored = CL_DiscordDisconnected,
	};
	Discord_Initialize( G_STRINGIFY( DISCORD_APP_ID ), &handlers, 1, NULL );
}

/*
 * CL_ShutdownDiscord
 */
void CL_ShutdownDiscord( void )
{
	Discord_Shutdown();

	memset( &cl_presence_state, 0, sizeof( cl_presence_state ) );

	Com_Printf( "Discord module unloaded.\n" );
}
