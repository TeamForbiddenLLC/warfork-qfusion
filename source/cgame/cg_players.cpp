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

#include "cg_local.h"
#include "../qcommon/steam.h"
#include "../qcommon/mod_fs.h"
#include <cstdint>

static const char *cg_defaultSexedSounds[] =
{
	"*death", //"*death2", "*death3", "*death4",
	"*fall_0_1", "*fall_0_2", "*fall_1", "*fall_2",
	"*falldeath",
	"*gasp", "*drown",
	"*jump_1", "*jump_2", "*jump_3", "*jump_4",
	"*pain25", "*pain50", "*pain75", "*pain100",
	"*wj_1", "*wj_2",
	"*dash_1", "*dash_2",
	"*taunt",
	"*rkill_1", "*rkill_2",
	NULL
};

static const char *cg_vsaySexedSounds[VSAY_TOTAL] = {
	"", // VSAY_GENERIC
	"*needhealth", // VSAY_NEEDHEALTH
	"*needweapon", // VSAY_NEEDWEAPON
	"*needarmor", // VSAY_NEEDARMOR
	"*affirmative", // VSAY_AFFIRMATIVE
	"*negative", // VSAY_NEGATIVE
	"*yes", // VSAY_YES
	"*no", // VSAY_NO
	"*ondefense", // VSAY_ONDEFENSE
	"*onoffense", // VSAY_ONOFFENSE
	"*oops", // VSAY_OOPS
	"*sorry", // VSAY_SORRY
	"*thanks", // VSAY_THANKS
	"*noproblem", // VSAY_NOPROBLEM
	"*yeehaa", // VSAY_YEEHAA
	"*goodgame", // VSAY_GOODGAME
	"*defend", // VSAY_DEFEND
	"*attack", // VSAY_ATTACK
	"*needbackup", // VSAY_NEEDBACKUP
	"*booo", // VSAY_BOO
	"*needdefense", // VSAY_NEEDDEFENSE
	"*needoffense", // VSAY_NEEDOFFENSE
	"*needhelp", // VSAY_NEEDHELP
	"*roger", // VSAY_ROGER
	"*armorfree", // VSAY_ARMORFREE
	"*areasecured", // VSAY_AREASECURED
	"*shutup", // VSAY_SHUTUP
	"*boomstick", // VSAY_BOOMSTICK
	"*gotowarshell", //VSAY_GOTOWARSHELL
	"*gotoquad", // VSAY_GOTOQUAD
	"*ok", // VSAY_OK
	"*defend_a", // VSAY_DEFEND_A
	"*attack_a", // VSAY_ATTACK_A
	"*defend_b", // VSAY_DEFEND_B
	"*attack_b", // VSAY_ATTACK_B
	NULL
};


/*
* CG_RegisterPmodelSexedSound
*/
static struct sfx_s *CG_RegisterPmodelSexedSound( pmodelinfo_t *pmodelinfo, const char *name )
{
	char *p, *s, model[MAX_QPATH];
	cg_sexedSfx_t *sexedSfx;
	char oname[MAX_QPATH];
	char sexedFilename[MAX_QPATH];

	if( !pmodelinfo )
		return NULL;

	Q_strncpyz( oname, name, sizeof( oname ) );
	COM_StripExtension( oname );
	for( sexedSfx = pmodelinfo->sexedSfx; sexedSfx; sexedSfx = sexedSfx->next )
	{
		if( !Q_stricmp( sexedSfx->name, oname ) )
			return sexedSfx->sfx;
	}

	// find out what's the model name
	s = pmodelinfo->name;
	if( s[0] )
	{
		p = strchr( s, '/' );
		if( p )
		{
			s = p + 1;
			p = strchr( s, '/' );
			if( p )
			{
				Q_strncpyz( model, p + 1, sizeof( model ) );
				p = strchr( model, '/' );
				if( p )
					*p = 0;
			}
		}
	}

	// if we can't figure it out, they're DEFAULT_PLAYERMODEL
	if( !model[0] )
		Q_strncpyz( model, DEFAULT_PLAYERMODEL, sizeof( model ) );

	sexedSfx = ( cg_sexedSfx_t * )CG_Malloc( sizeof( cg_sexedSfx_t ) );
	sexedSfx->name = CG_CopyString( oname );
	sexedSfx->next = pmodelinfo->sexedSfx;
	pmodelinfo->sexedSfx = sexedSfx;

	// see if we already know of the model specific sound
	Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/%s/%s", model, oname+1 );

	if( ( !COM_FileExtension( sexedFilename ) &&
		FS_FirstExtension( sexedFilename, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS ) ) ||
		FS_FOpenFile( sexedFilename, NULL, FS_READ ) != -1 )
	{
		sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
	}
	else
	{       // no, revert to default player sounds folders
		if( pmodelinfo->sex == GENDER_FEMALE )
		{
			Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/%s/%s", "female", oname+1 );
			sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
		}
		else
		{
			Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/%s/%s", "male", oname+1 );
			sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
		}
	}

	return sexedSfx->sfx;
}

/*
* CG_UpdateSexedSoundsRegistration
*/
void CG_UpdateSexedSoundsRegistration( pmodelinfo_t *pmodelinfo )
{
	cg_sexedSfx_t *sexedSfx, *next;
	const char *name;
	int i;

	if( !pmodelinfo )
		return;

	// free loaded sounds
	for( sexedSfx = pmodelinfo->sexedSfx; sexedSfx; sexedSfx = next )
	{
		next = sexedSfx->next;
		CG_Free( sexedSfx );
	}
	pmodelinfo->sexedSfx = NULL;

	// load default sounds
	for( i = 0;; i++ )
	{
		name = cg_defaultSexedSounds[i];
		if( !name )
			break;
		CG_RegisterPmodelSexedSound( pmodelinfo, name );
	}
	for( i = 0;; i++) {
		name = cg_vsaySexedSounds[i];
		if( !name )
			break;
		if( name[0] == '*' )
			CG_RegisterPmodelSexedSound( pmodelinfo, name );
	}

	// load sounds server told us
	for( i = 1; i < MAX_SOUNDS; i++ )
	{
		name = cgs.configStrings[CS_SOUNDS+i];
		if( !name[0] )
			break;
		if( name[0] == '*' )
			CG_RegisterPmodelSexedSound( pmodelinfo, name );
	}
}

/*
* CG_RegisterSexedSound
*/
struct sfx_s *CG_RegisterSexedSound( int entnum, const char *name )
{
	if( entnum < 0 || entnum >= MAX_EDICTS )
		return NULL;
	return CG_RegisterPmodelSexedSound( cg_entPModels[entnum].pmodelinfo, name );
}

/*
* CG_SexedSound
*/
void CG_SexedSound( int entnum, int entchannel, const char *name, float fvol, float attn )
{
	bool fixed;

	fixed = entchannel & CHAN_FIXED ? true : false;
	entchannel &= ~CHAN_FIXED;

	if( fixed )
		trap_S_StartFixedSound( CG_RegisterSexedSound( entnum, name ), cg_entities[entnum].current.origin, entchannel, fvol, attn );
	else if( ISVIEWERENTITY( entnum ) )
		trap_S_StartGlobalSound( CG_RegisterSexedSound( entnum, name ), entchannel, fvol );
	else
		trap_S_StartRelativeSound( CG_RegisterSexedSound( entnum, name ), entnum, entchannel, fvol, attn );
}

void CG_SexedVSay( int entnum, int vsay, float fvol )
{
	if( vsay <= VSAY_GENERIC || vsay >= VSAY_TOTAL)
		return;
	CG_SexedSound( entnum, CHAN_AUTO, cg_vsaySexedSounds[vsay], fvol, ATTN_NONE);
}

/*
* CG_SetClientName
*/
static void CG_SetClientName( cg_clientInfo_t *ci, const char *src, cg_nameSource_e source )
{
	Q_strncpyz( ci->name, src, sizeof( ci->name ) );
	Q_strncpyz( ci->cleanname, COM_RemoveColorTokens( ci->name ), sizeof( ci->cleanname ) );
	ci->nameSource = source;
}

/*
* CG_ClientNetName
*
* The name the server knows this client by. Server commands (stats, whois, ...) resolve
* against this rather than the displayed name, which may be a steam nickname.
*/
void CG_ClientNetName( int client, char *out, size_t outSize )
{
	const char *s = Info_ValueForKey( cgs.configStrings[CS_PLAYERINFOS + client], "name" );
	Q_strncpyz( out, s && s[0] ? s : "badname", outSize );
}

/*
* CG_SanitizeSteamName
*
* Steam nicknames are arbitrary UTF-8 and never went through the server's name
* validation, so they get cleaned up here before anything renders them.
*/
static void CG_SanitizeSteamName( const char *in, char *out, size_t outSize )
{
	char escaped[STEAM_PERSONA_NAME_MAX * 2 + 1];
	char sanitized[sizeof( escaped ) + 1];
	size_t len = 0;

	out[0] = '\0';

	// strip control characters and escape every color token so a nickname containing
	// "^1" recolors nothing and "^0" can't render itself invisible. high bytes are
	// left alone - unlike server-side names, these may legitimately be UTF-8
	for( const char *p = in; *p && len + 2 < sizeof( escaped ); p++ ) {
		if( (unsigned char)*p < 32 )
			continue;
		if( *p == Q_COLOR_ESCAPE )
			escaped[len++] = Q_COLOR_ESCAPE;
		escaped[len++] = *p;
	}
	escaped[len] = '\0';

	// -1 for maxprintablechars: it counts bytes rather than codepoints, so using it
	// to clamp would cut UTF-8 sequences in half
	COM_SanitizeColorString( escaped, sanitized, sizeof( sanitized ), -1, COLOR_WHITE );

	// clamp on a codepoint boundary. downstream consumers assume a name fits
	// MAX_NAME_BYTES and truncate by bytes (CG_SC_Obituary's center print), which was
	// safe only while names were ascii-only - clamping here keeps all of them correct
	len = strlen( sanitized );
	if( len >= outSize ) {
		len = Q_Utf8SyncPos( sanitized, outSize - 1, UTF8SYNC_LEFT );
		sanitized[len] = '\0';
	}

	Q_trim( sanitized );

	// COM_SanitizeColorString doubles a trailing '^' so it renders literally, but the
	// clamp above can cut that pair back apart. a lone trailing '^' would swallow the
	// S_COLOR_WHITE that call sites append after the name, so drop it
	len = strlen( sanitized );
	size_t carets = 0;
	while( carets < len && sanitized[len - 1 - carets] == Q_COLOR_ESCAPE )
		carets++;
	if( carets & 1 )
		sanitized[len - 1] = '\0';

	const char *colorless = COM_RemoveColorTokens( sanitized );

	// a nickname that sanitizes down to nothing falls back to the configured name
	if( !colorless[0] )
		return;

	// the server rejects these prefixes on netnames (G_SetName) because chat prints
	// "console: ..." for server messages and "[TEAM]name: ..." for team chat. a steam
	// nickname must not be able to walk around that
	static const char *invalid_prefixes[] = { "console", "[team]", "[spec]", "[bot]", "[coach]", "[tv]", NULL };
	for( int i = 0; invalid_prefixes[i]; i++ ) {
		if( !Q_strnicmp( colorless, invalid_prefixes[i], strlen( invalid_prefixes[i] ) ) )
			return;
	}

	Q_strncpyz( out, sanitized, outSize );
}

static void CG_RPC_cb_userInformation( void *self, struct steam_rpc_pkt_s *rec )
{
	assert( rec->common.cmd == RPC_REQUEST_USER_INFORMATION );

	// either half of the reply can come back empty - steam hasn't cached the name yet, or
	// the avatar image is still downloading. an EVT_PERSONA_CHANGED / EVT_AVATAR_LOADED
	// tells us when to ask again, so apply whichever half did arrive and drop the rest
	const bool hasName = ( rec->user_info_recv.flags & STEAM_USER_INFO_NAME ) && rec->user_info_recv.name[0];
	const bool hasAvatar = ( rec->user_info_recv.flags & STEAM_USER_INFO_AVATAR ) && rec->user_info_recv.width > 0 &&
						   rec->user_info_recv.height > 0;
	if( !hasName && !hasAvatar )
		return;

	// match on the steamid the reply carries - the slot this request was made for
	// may have been handed to a different player while the RPC was in flight.
	// update every match rather than the first: two slots can briefly carry the same
	// steamid across a reconnect
	for( int i = 0; i < gs.maxclients; i++ ) {
		cg_clientInfo_t *ci = &cgs.clientInfo[i];
		if( ci->steamid != rec->user_info_recv.steamID )
			continue;

		// the steam nickname outranks the netname whenever one comes back - the netname is
		// only what stands in for a player steam has nothing cached for
		if( hasName ) {
			char steamname[MAX_NAME_BYTES];
			CG_SanitizeSteamName( rec->user_info_recv.name, steamname, sizeof( steamname ) );
			if( steamname[0] )
				CG_SetClientName( ci, steamname, CG_NAME_SOURCE_STEAM );
		}
		if( hasAvatar ) {
			ci->avatar = R_RegisterRawPic( va( "avatar-%llu", ci->steamid ), rec->user_info_recv.width,
										   rec->user_info_recv.height, rec->user_info_recv.buf, 4 );
		}
	}
}

static void CG_RequestUserInfo( uint64_t steamid, uint32_t flags )
{
	if( !steamid || !flags || !STEAMSHIM_active() )
		return;

	struct user_information_req_s req;
	req.cmd = RPC_REQUEST_USER_INFORMATION;
	req.steamID = steamid;
	req.flags = flags;
	req.size = STEAM_AVATAR_SMALL;
	STEAMSHIM_sendRPC( &req, sizeof( struct user_information_req_s ), NULL, CG_RPC_cb_userInformation, NULL );
}

static void CG_EVT_cb_personaChanged(void* self, struct steam_evt_pkt_s* pkt) {
	assert( pkt->common.cmd == EVT_PERSONA_CHANGED );

	// only ask for what actually changed - a name change shouldn't drag the avatar
	// pixels back across the pipe
	uint32_t flags = 0;
	if( pkt->persona_changed.name_changed > 0 )
		flags |= STEAM_USER_INFO_NAME;
	if( pkt->persona_changed.avatar_changed > 0 )
		flags |= STEAM_USER_INFO_AVATAR;
	if( !flags )
		return;

	for( int i = 0; i < gs.maxclients; i++ ) {
		cg_clientInfo_t *ci = &cgs.clientInfo[i];
		if( ci->steamid == pkt->persona_changed.steamID ) {
			CG_RequestUserInfo( ci->steamid, flags );
		}
	}
}

static void CG_EVT_cb_avatarLoaded( void *self, struct steam_evt_pkt_s *pkt )
{
	assert( pkt->common.cmd == EVT_AVATAR_LOADED );

	// the earlier request came back empty because the image was still downloading
	for( int i = 0; i < gs.maxclients; i++ ) {
		cg_clientInfo_t *ci = &cgs.clientInfo[i];
		if( ci->steamid == pkt->avatar_loaded.steamID ) {
			CG_RequestUserInfo( ci->steamid, STEAM_USER_INFO_AVATAR );
		}
	}
}

void CG_initPlayer() {
	STEAMSHIM_subscribeEvent(EVT_PERSONA_CHANGED, NULL, CG_EVT_cb_personaChanged);
	STEAMSHIM_subscribeEvent(EVT_AVATAR_LOADED, NULL, CG_EVT_cb_avatarLoaded);
}

void CG_deinitPlayer() {
	STEAMSHIM_unsubscribeEvent(EVT_PERSONA_CHANGED, CG_EVT_cb_personaChanged);
	STEAMSHIM_unsubscribeEvent(EVT_AVATAR_LOADED, CG_EVT_cb_avatarLoaded);
}
/*
* CG_LoadClientInfo
*/
void CG_LoadClientInfo( cg_clientInfo_t *ci, const char *info, int client )
{
	char *s;
	int rgbcolor;

	assert( ci );
	assert( info );
	assert( client >= 0 && client < gs.maxclients );

	if( !Info_Validate( info ) )
		CG_Error( "Invalid client info" );

	// before the name: a recycled slot must not keep the previous player's steam nickname
	s = Info_ValueForKey( info, "steam_id" );
	uint64_t steamid = ( s && s[0] ) ? strtoull( s, NULL, 10 ) : 0;
	const bool steamidChanged = ( steamid != ci->steamid );
	if( steamidChanged ) {
		ci->steamid = steamid;
		ci->avatar = NULL;
		ci->nameSource = CG_NAME_SOURCE_NET;
	}

	s = Info_ValueForKey( info, "name" );
	const char *netname = s && s[0] ? s : "badname";

	// hold on to a steam nickname - it outranks the netname, and this runs on every
	// playerinfo update
	if( ci->nameSource != CG_NAME_SOURCE_STEAM )
		CG_SetClientName( ci, netname, CG_NAME_SOURCE_NET );

	// only on a transition - this runs on every playerinfo update. a steamid of 0 is
	// dropped by CG_RequestUserInfo, so a player without steam keeps the netname
	uint32_t userInfoFlags = steamidChanged ? ( STEAM_USER_INFO_NAME | STEAM_USER_INFO_AVATAR ) : 0;
	CG_RequestUserInfo( ci->steamid, userInfoFlags );

	s = Info_ValueForKey( info, "hand" );
	ci->hand = s && s[0] ? atoi( s ) : 2;

	// color
	s = Info_ValueForKey( info, "color" );
	rgbcolor = s && s[0] ? COM_ReadColorRGBString( s ) : -1;
	if( rgbcolor != -1 )
		Vector4Set( ci->color, COLOR_R( rgbcolor ), COLOR_G( rgbcolor ), COLOR_B( rgbcolor ), 255 );
	else
		Vector4Set( ci->color, 255, 255, 255, 255 );

	s = Info_ValueForKey( info, "m" );
	ci->modelindex = s && s[0] ? atoi( s ) : 0;
}
