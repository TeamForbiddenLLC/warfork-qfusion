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
#include <cstdint>

static const char *cg_defaultSexedSounds[] =
{
	"*death", //"*death2", "*death3", "*death4",
	"*fall_0_1", "*fall_0_2", "*fall_1", "*fall_2",
	"*falldeath",
	"*gasp", "*drown",
	"*jump_1", "*jump_2",
	"*pain25", "*pain50", "*pain75", "*pain100",
	"*wj_1", "*wj_2",
	"*dash_1", "*dash_2",
	"*taunt",
	"*rkill_1", "*rkill_2", "*rkill_3", "*rkill_4", "*rkill_5", "*rkill_6", "*rkill_7", "*rkill_8", "*rkill_9", "*rkill_10", "*rkill_11", "*rkill_12", "*rkill_13", "*rkill_14", "*rkill_15", "*rkill_16", "*rkill_17", "*rkill_18", "*rkill_19", "*rkill_20", "*rkill_21", "*rkill_22", "*rkill_23", "*rkill_24", "*rkill_25", "*rkill_26", "*rkill_27", "*rkill_28", "*rkill_29", "*rkill_30", "*rkill_31", "*rkill_32", "*rkill_33", "*rkill_34", "*rkill_35", "*rkill_36", "*rkill_37", "*rkill_38", "*rkill_39", "*rkill_40", "*rkill_41", "*rkill_42", "*rkill_43", "*rkill_44", "*rkill_45", "*rkill_46", "*rkill_47", "*rkill_48", "*rkill_49", "*rkill_50",
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
		trap_FS_FirstExtension( sexedFilename, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS ) ) ||
		trap_FS_FOpenFile( sexedFilename, NULL, FS_READ ) != -1 )
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

	s = Info_ValueForKey( info, "name" );
	Q_strncpyz( ci->name, s && s[0] ? s : "badname", sizeof( ci->name ) );

	// name with color tokes stripped
	Q_strncpyz( ci->cleanname, COM_RemoveColorTokens( ci->name ), sizeof( ci->cleanname ) );

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


	s = Info_ValueForKey( info, "steam_id" );
	if (s && atol(s)){
		ci->steamid = atol(s);
	}
}

void CG_CallbackRequestAvatar(uint64_t steamid, char *avatar){
	for (int i = 0; i < gs.maxclients; i++){
		cg_clientInfo_t ci= cgs.clientInfo[i];
		if (ci.steamid == steamid){

			memcpy(ci.avatar, avatar, sizeof ci.avatar);
			return;
		}
	}
}
