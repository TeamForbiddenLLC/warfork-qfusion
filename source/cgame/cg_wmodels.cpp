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
==========================================================================

- SPLITMODELS -

==========================================================================
*/
// - Adding the weapon models in split pieces
// by Jalisk0

#include "cg_local.h"

#include "../qcommon/mod_fs.h"


//======================================================================
//						weaponinfo Registering
//======================================================================

weaponinfo_t cg_pWeaponModelInfos[WEAP_TOTAL];

static const char *wmPartSufix[] = { "", "_expansion", "_barrel", "_barrel2", "_flash", "_hand", NULL };

/*
* CG_vWeap_ParseAnimationScript
* 
* script:
* 0 = first frame
* 1 = lastframe/number of frames
* 2 = looping frames
* 3 = frame time
* 
* keywords:
* "islastframe":Will read the second value of each animation as lastframe (usually means numframes)
* "rotationscale": value witch will scale the barrel rotation speed
*/
static bool CG_vWeap_ParseAnimationScript( weaponinfo_t *weaponinfo, const char *filename )
{
	uint8_t *buf;
	char *ptr, *token;
	int rounder, counter, i;
	bool debug = true;
	int anim_data[4][VWEAP_MAXANIMS];
	int length, filenum;

	rounder = 0;
	counter = 1; // reserve 0 for 'no animation'

	// set some defaults
	weaponinfo->barrelInfo.speed = 0;
	weaponinfo->barrel2Info.speed = 0;
	weaponinfo->flashFade = true;

	if( !cg_debugWeaponModels->integer )
		debug = false;

	// load the file
	length = FS_FOpenFile( filename, &filenum, FS_READ );
	if( length == -1 )
		return false;
	if( !length )
	{
		FS_FCloseFile( filenum );
		return false;
	}
	buf = ( uint8_t * )CG_Malloc( length + 1 );
	FS_Read( buf, length, filenum );
	FS_FCloseFile( filenum );

	if( !buf )
	{
		CG_Free( buf );
		return false;
	}

	if( debug )
		CG_Printf( "%sLoading weapon animation script:%s%s\n", S_COLOR_BLUE, filename, S_COLOR_WHITE );

	memset( anim_data, 0, sizeof( anim_data ) );

	//proceed
	ptr = ( char * )buf;
	while( ptr )
	{
		token = COM_ParseExt( &ptr, true );
		if( !token[0] )
			break;

		//see if it is keyword or number
		if( *token < '0' || *token > '9' )
		{
			if( !Q_stricmp( token, "barrel" ) )
			{
				if( debug )
					CG_Printf( "%sScript: barrel:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				//time
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->barrelInfo.time = (unsigned int)( i > 0 ? i : 0 );
				// speed
				weaponinfo->barrelInfo.speed = atof( COM_ParseExt( &ptr, false ) );
				
				// initial delay	
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->barrelInfo.initialDelay = (unsigned int)( i > 0 ? i : 0 );

				// recoil delay	
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->barrelInfo.recoilDelay = (unsigned int)( i > 0 ? i : 0 );

				if( debug )
					CG_Printf( "%s time:%i, speed:%.2f\n", S_COLOR_BLUE, (int)weaponinfo->barrelInfo.time, weaponinfo->barrelInfo.speed, S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "barrel2" ) ) {
				if( debug )
					CG_Printf( "%sScript: barrel2:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				// time
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->barrel2Info.time = (unsigned int)( i > 0 ? i : 0 );

				// speed
				weaponinfo->barrel2Info.speed = atof( COM_ParseExt( &ptr, false ) );

				// initial delay
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->barrel2Info.initialDelay = (unsigned int)( i > 0 ? i : 0 );

				// recoil delay
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->barrel2Info.recoilDelay = (unsigned int)( i > 0 ? i : 0 );

				if( debug )
					CG_Printf( "%s time:%i, speed:%.2f\n", S_COLOR_BLUE, (int)weaponinfo->barrel2Info.time, weaponinfo->barrel2Info.speed, S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "flash" ) )
			{
				if( debug )
					CG_Printf( "%sScript: flash:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				// time
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->flashTime = (unsigned int)( i > 0 ? i : 0 );

				// radius
				i = atoi( COM_ParseExt( &ptr, false ) );
				weaponinfo->flashRadius = (float)( i > 0 ? i : 0 );

				// fade
				token = COM_ParseExt( &ptr, false );
				if( !Q_stricmp( token, "no" ) )
					weaponinfo->flashFade = false;

				if( debug )
					CG_Printf( "%s time:%i, radius:%i, fade:%s%s\n", S_COLOR_BLUE, (int)weaponinfo->flashTime, (int)weaponinfo->flashRadius, weaponinfo->flashFade ? "YES" : "NO", S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "flashColor" ) )
			{
				if( debug )
					CG_Printf( "%sScript: flashColor:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				weaponinfo->flashColor[0] = atof( token = COM_ParseExt( &ptr, false ) );
				weaponinfo->flashColor[1] = atof( token = COM_ParseExt( &ptr, false ) );
				weaponinfo->flashColor[2] = atof( token = COM_ParseExt( &ptr, false ) );

				if( debug )
					CG_Printf( "%s%f %f %f%s\n", S_COLOR_BLUE,
					weaponinfo->flashColor[0], weaponinfo->flashColor[1], weaponinfo->flashColor[2],
					S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "handOffset" ) )
			{
				if( debug )
					CG_Printf( "%sScript: handPosition:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				weaponinfo->handpositionOrigin[FORWARD] = atof( COM_ParseExt( &ptr, false ) );
				weaponinfo->handpositionOrigin[RIGHT] = atof( COM_ParseExt( &ptr, false ) );
				weaponinfo->handpositionOrigin[UP] = atof( COM_ParseExt( &ptr, false ) );
				weaponinfo->handpositionAngles[PITCH] = atof( COM_ParseExt( &ptr, false ) );
				weaponinfo->handpositionAngles[YAW] = atof( COM_ParseExt( &ptr, false ) );
				weaponinfo->handpositionAngles[ROLL] = atof( COM_ParseExt( &ptr, false ) );

				if( debug )
					CG_Printf( "%s%f %f %f %f %f %f%s\n", S_COLOR_BLUE,
					weaponinfo->handpositionOrigin[0], weaponinfo->handpositionOrigin[1], weaponinfo->handpositionOrigin[2],
					weaponinfo->handpositionAngles[0], weaponinfo->handpositionAngles[1], weaponinfo->handpositionAngles[2],
					S_COLOR_WHITE );

			}
			else if( !Q_stricmp( token, "firesound" ) )
			{
				if( debug )
					CG_Printf( "%sScript: firesound:%s", S_COLOR_BLUE, S_COLOR_WHITE );
				if( weaponinfo->num_fire_sounds >= WEAPONINFO_MAX_FIRE_SOUNDS )
				{
					if( debug )
						CG_Printf( S_COLOR_BLUE "too many firesounds defined. Max is %i" S_COLOR_WHITE "\n", WEAPONINFO_MAX_FIRE_SOUNDS );
					break;
				}

				token = COM_ParseExt( &ptr, false );
				if( Q_stricmp( token, "NULL" ) )
				{
					weaponinfo->sound_fire[weaponinfo->num_fire_sounds] = trap_S_RegisterSound( token );
					if( weaponinfo->sound_fire[weaponinfo->num_fire_sounds] != NULL )
						weaponinfo->num_fire_sounds++;
				}
				if( debug )
					CG_Printf( "%s%s%s\n", S_COLOR_BLUE, token, S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "strongfiresound" ) )
			{
				if( debug )
					CG_Printf( "%sScript: strongfiresound:%s", S_COLOR_BLUE, S_COLOR_WHITE );
				if( weaponinfo->num_strongfire_sounds >= WEAPONINFO_MAX_FIRE_SOUNDS )
				{
					if( debug )
						CG_Printf( S_COLOR_BLUE "too many strongfiresound defined. Max is %i" S_COLOR_WHITE "\n", WEAPONINFO_MAX_FIRE_SOUNDS );
					break;
				}

				token = COM_ParseExt( &ptr, false );
				if( Q_stricmp( token, "NULL" ) )
				{
					weaponinfo->sound_strongfire[weaponinfo->num_strongfire_sounds] = trap_S_RegisterSound( token );
					if( weaponinfo->sound_strongfire[weaponinfo->num_strongfire_sounds] != NULL )
						weaponinfo->num_strongfire_sounds++;
				}
				if( debug )
					CG_Printf( "%s%s%s\n", S_COLOR_BLUE, token, S_COLOR_WHITE );
			}
			else if( token[0] && debug )
				CG_Printf( "%signored: %s%s\n", S_COLOR_YELLOW, token, S_COLOR_WHITE );
		}
		else
		{
			//frame & animation values
			i = (int)atoi( token );
			if( debug )
			{
				if( rounder == 0 )
					CG_Printf( "%sScript: %s", S_COLOR_BLUE, S_COLOR_WHITE );
				CG_Printf( "%s%i - %s", S_COLOR_BLUE, i, S_COLOR_WHITE );
			}
			anim_data[rounder][counter] = i;
			rounder++;
			if( rounder > 3 )
			{
				rounder = 0;
				if( debug )
					CG_Printf( "%s anim: %i%s\n", S_COLOR_BLUE, counter, S_COLOR_WHITE );
				counter++;
				if( counter == VWEAP_MAXANIMS )
					break;
			}
		}
	}

	CG_Free( buf );

	if( counter < VWEAP_MAXANIMS )
	{
		CG_Printf( "%sERROR: incomplete WEAPON script: %s - Using default%s\n", S_COLOR_YELLOW, filename, S_COLOR_WHITE );
		return false;
	}

	//reorganize to make my life easier
	for( i = 0; i < VWEAP_MAXANIMS; i++ )
	{
		weaponinfo->firstframe[i] = anim_data[0][i];
		weaponinfo->lastframe[i] = anim_data[1][i];
		weaponinfo->loopingframes[i] = anim_data[2][i];

		if( anim_data[3][i] < 10 )  //never allow less than 10 fps
			anim_data[3][i] = 10;

		weaponinfo->frametime[i] = 1000/anim_data[3][i];
	}

	return true;
}

/*
* CG_LoadHandAnimations
*/
static void CG_CreateHandDefaultAnimations( weaponinfo_t *weaponinfo )
{
	float defaultfps = 15.0f;

	weaponinfo->barrelInfo.speed = 0; // default
	weaponinfo->barrel2Info.speed = 0;

	// default wsw hand
	weaponinfo->firstframe[WEAPMODEL_STANDBY] = 0;
	weaponinfo->lastframe[WEAPMODEL_STANDBY] = 0;
	weaponinfo->loopingframes[WEAPMODEL_STANDBY] = 1;
	weaponinfo->frametime[WEAPMODEL_STANDBY] = 1000/defaultfps;

	weaponinfo->firstframe[WEAPMODEL_ATTACK_WEAK] = 1; // attack animation (1-5)
	weaponinfo->lastframe[WEAPMODEL_ATTACK_WEAK] = 5;
	weaponinfo->loopingframes[WEAPMODEL_ATTACK_WEAK] = 0;
	weaponinfo->frametime[WEAPMODEL_ATTACK_WEAK] = 1000/defaultfps;

	weaponinfo->firstframe[WEAPMODEL_ATTACK_STRONG] = 0;
	weaponinfo->lastframe[WEAPMODEL_ATTACK_STRONG] = 0;
	weaponinfo->loopingframes[WEAPMODEL_ATTACK_STRONG] = 1;
	weaponinfo->frametime[WEAPMODEL_ATTACK_STRONG] = 1000/defaultfps;

	weaponinfo->firstframe[WEAPMODEL_WEAPDOWN] = 0;
	weaponinfo->lastframe[WEAPMODEL_WEAPDOWN] = 0;
	weaponinfo->loopingframes[WEAPMODEL_WEAPDOWN] = 1;
	weaponinfo->frametime[WEAPMODEL_WEAPDOWN] = 1000/defaultfps;

	weaponinfo->firstframe[WEAPMODEL_WEAPONUP] = 6; // flipout animation (6-10)
	weaponinfo->lastframe[WEAPMODEL_WEAPONUP] = 10;
	weaponinfo->loopingframes[WEAPMODEL_WEAPONUP] = 1;
	weaponinfo->frametime[WEAPMODEL_WEAPONUP] = 1000/defaultfps;

	return;
}

/*
* CG_BuildProjectionOrigin
* store the orientation_t closer to the tag_flash we can create,
* or create one using an offset we consider acceptable.
* NOTE: This tag will ignore weapon models animations. You'd have to
* do it in realtime to use it with animations. Or be careful on not
* moving the weapon too much
*/
static void CG_BuildProjectionOrigin( weaponinfo_t *weaponinfo )
{
	orientation_t tag, tag_barrel, tag_barrel2;
	static entity_t	ent;

	if( !weaponinfo )
		return;

	if( weaponinfo->model[WEAPON] )
	{

		// assign the model to an entity_t, so we can build boneposes
		memset( &ent, 0, sizeof( ent ) );
		ent.rtype = RT_MODEL;
		ent.scale = 1.0f;
		ent.model = weaponinfo->model[WEAPON];
		CG_SetBoneposesForTemporaryEntity( &ent ); // assigns and builds the skeleton so we can use grabtag

		// try getting the tag_flash from the weapon model
		if( CG_GrabTag( &weaponinfo->tag_projectionsource, &ent, "tag_flash" ) )
			return; // succesfully

		// if it didn't work, try getting it from the barrel model
		if( CG_GrabTag( &tag_barrel, &ent, "tag_barrel" ) && weaponinfo->model[BARREL] )
		{
			// assign the model to an entity_t, so we can build boneposes
			memset( &ent, 0, sizeof( ent ) );
			ent.rtype = RT_MODEL;
			ent.scale = 1.0f;
			ent.model = weaponinfo->model[BARREL];
			CG_SetBoneposesForTemporaryEntity( &ent );
			if( CG_GrabTag( &tag, &ent, "tag_flash" ) && weaponinfo->model[BARREL] )
			{
				VectorCopy( vec3_origin, weaponinfo->tag_projectionsource.origin );
				Matrix3_Identity( weaponinfo->tag_projectionsource.axis );
				CG_MoveToTag( weaponinfo->tag_projectionsource.origin,
					weaponinfo->tag_projectionsource.axis,
					tag_barrel.origin,
					tag_barrel.axis,
					tag.origin,
					tag.axis );
				return; // successfully
			}
		}
		if( CG_GrabTag( &tag_barrel2, &ent, "tag_barrel2" ) && weaponinfo->model[BARREL2] ) {
			// assign the model to an entity_t, so we can build boneposes
			memset( &ent, 0, sizeof( ent ) );
			ent.rtype = RT_MODEL;
			ent.scale = 1.0f;
			ent.model = weaponinfo->model[BARREL2];
			CG_SetBoneposesForTemporaryEntity( &ent );
		}
	}

	// doesn't have a weapon model, or the weapon model doesn't have a tag
	VectorSet( weaponinfo->tag_projectionsource.origin, 16, 0, 8 );
	Matrix3_Identity( weaponinfo->tag_projectionsource.axis );
}

/*
* CG_WeaponModelUpdateRegistration
*/
static bool CG_WeaponModelUpdateRegistration( weaponinfo_t *weaponinfo, char *filename )
{
	int p;
	char scratch[MAX_QPATH];

	for( p = 0; p < WEAPMODEL_PARTS; p++ )
	{
		// md3
		if( !weaponinfo->model[p] )
		{
			Q_snprintfz( scratch, sizeof( scratch ), "models/weapons/%s%s.md3", filename, wmPartSufix[p] );
			weaponinfo->model[p] = CG_RegisterModel( scratch );
		}
		// skm
		if( !weaponinfo->model[p] )
		{
			Q_snprintfz( scratch, sizeof( scratch ), "models/weapons/%s%s.iqm", filename, wmPartSufix[p] );
			weaponinfo->model[p] = CG_RegisterModel( scratch );
		}
	}

	// load failed
	if( !weaponinfo->model[HAND] )
	{
		weaponinfo->name[0] = 0;
		for( p = 0; p < WEAPMODEL_PARTS; p++ )
			weaponinfo->model[p] = NULL;

		return false;
	}

	// load animation script for the hand model
	Q_snprintfz( scratch, sizeof( scratch ), "models/weapons/%s.cfg", filename );

	if( !CG_vWeap_ParseAnimationScript( weaponinfo, scratch ) )
		CG_CreateHandDefaultAnimations( weaponinfo );

	//create a tag_projection from tag_flash, to position fire effects
	CG_BuildProjectionOrigin( weaponinfo );
	Vector4Set( weaponinfo->outlineColor, 0, 0, 0, 255 );

	if( cg_debugWeaponModels->integer )
		CG_Printf( "%sWEAPmodel: Loaded successful%s\n", S_COLOR_BLUE, S_COLOR_WHITE );

	Q_strncpyz( weaponinfo->name, filename, sizeof( weaponinfo->name ) );

	return true;
}

/*
* CG_FindWeaponModelSpot
* 
* Stored names format is without extension, like this: "rocketl/rocketl"
*/
static struct weaponinfo_s *CG_FindWeaponModelSpot( char *filename )
{
	int i;
	int freespot = -1;


	for( i = 0; i < WEAP_TOTAL; i++ )
	{
		if( cg_pWeaponModelInfos[i].inuse == true )
		{
			if( !Q_stricmp( cg_pWeaponModelInfos[i].name, filename ) ) //found it
			{
				if( cg_debugWeaponModels->integer )
					CG_Printf( "WEAPModel: found at spot %i: %s\n", i, filename );

				return &cg_pWeaponModelInfos[i];
			}
		}
		else if( freespot < 0 )
			freespot = i;
	}

	if( freespot < 0 )
		CG_Error( "%sCG_FindWeaponModelSpot: Couldn't find a free weaponinfo spot%s", S_COLOR_RED, S_COLOR_WHITE );

	//we have a free spot
	if( cg_debugWeaponModels->integer )
		CG_Printf( "WEAPmodel: assigned free spot %i for weaponinfo %s\n", freespot, filename );

	return &cg_pWeaponModelInfos[freespot];
}

/*
* CG_RegisterWeaponModel
*/
struct weaponinfo_s *CG_RegisterWeaponModel( char *cgs_name, int weaponTag )
{
	char filename[MAX_QPATH];
	weaponinfo_t *weaponinfo;

	Q_strncpyz( filename, cgs_name, sizeof( filename ) );
	COM_StripExtension( filename );

	weaponinfo = CG_FindWeaponModelSpot( filename );

	if( weaponinfo->inuse == true )
		return weaponinfo;

	weaponinfo->inuse = CG_WeaponModelUpdateRegistration( weaponinfo, filename );
	if( !weaponinfo->inuse )
	{
		if( cg_debugWeaponModels->integer )
			CG_Printf( "%sWEAPmodel: Failed:%s%s\n", S_COLOR_YELLOW, filename, S_COLOR_WHITE );

		return NULL;
	}

	// find the item for this weapon and try to assign the outline color
	if( weaponTag )
	{
		gsitem_t *item = GS_FindItemByTag( weaponTag );
		if( item )
		{
			if( item->color && strlen( item->color ) > 1 )
			{
				byte_vec4_t colorByte;

				Vector4Scale( color_table[ColorIndex( item->color[1] )], 255, colorByte );
				CG_SetOutlineColor( weaponinfo->outlineColor, colorByte );
			}
		}
	}

	return weaponinfo;
}


/*
* CG_CreateWeaponZeroModel
* 
* we can't allow NULL weaponmodels to be passed to the viewweapon.
* They will produce crashes because the lack of animation script.
* We need to have at least one weaponinfo with a script to be used
* as a replacement, so, weapon 0 will have the animation script
* even if the registration failed
*/
struct weaponinfo_s *CG_CreateWeaponZeroModel( char *filename )
{
	weaponinfo_t *weaponinfo;

	COM_StripExtension( filename );

	weaponinfo = CG_FindWeaponModelSpot( filename );

	if( weaponinfo->inuse == true )
		return weaponinfo;

	if( cg_debugWeaponModels->integer )
		CG_Printf( "%sWEAPmodel: Failed to load generic weapon. Creating a fake one%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );

	CG_CreateHandDefaultAnimations( weaponinfo );
	Vector4Set( weaponinfo->outlineColor, 0, 0, 0, 255 );
	weaponinfo->inuse = true;

	Q_strncpyz( weaponinfo->name, filename, sizeof( weaponinfo->name ) );

	return weaponinfo; //no checks
}


//======================================================================
//							weapons
//======================================================================

/*
* CG_GetWeaponFromClientIndex
*/
struct weaponinfo_s *CG_GetWeaponInfo( int weapon )
{
	if( weapon < 0 || ( weapon >= WEAP_TOTAL ) )
		weapon = WEAP_NONE;

	return cgs.weaponInfos[weapon] ? cgs.weaponInfos[weapon] : cgs.weaponInfos[WEAP_NONE];
}

/*
* CG_AddWeaponOnTag
*
* Add weapon model(s) positioned at the tag
*/
void CG_AddWeaponOnTag( entity_t *ent, orientation_t *tag, int weaponid, int effects, orientation_t *projectionSource, unsigned int flash_time, cg_barrelstate_t barrel, cg_barrelstate_t barrel2 )
{
	entity_t weapon;
	weaponinfo_t *weaponInfo;
	float intensity;

	//don't try without base model
	if( !ent->model )
		return;

	//don't try without a tag
	if( !tag )
		return;

	weaponInfo = CG_GetWeaponInfo( weaponid );
	if( !weaponInfo )
		return;

	//if( ent->renderfx & RF_WEAPONMODEL )
	//	effects &= ~EF_RACEGHOST;

	//weapon
	memset( &weapon, 0, sizeof( weapon ) );
	Vector4Set( weapon.shaderRGBA, 255, 255, 255, ent->shaderRGBA[3] );
	weapon.scale = ent->scale;
	weapon.renderfx = ent->renderfx;
	weapon.frame = 0;
	weapon.oldframe = 0;
	weapon.model = weaponInfo->model[WEAPON];

	CG_PlaceModelOnTag( &weapon, ent, tag );

	CG_AddColoredOutLineEffect( &weapon, effects, 0, 0, 0, 255 );

	if( !( effects & EF_RACEGHOST ) )
		CG_AddEntityToScene( &weapon );

	if( !weapon.model )
		return;

	CG_AddShellEffects( &weapon, effects );

	// update projection source
	if( projectionSource != NULL )
	{
		VectorCopy( vec3_origin, projectionSource->origin );
		Matrix3_Copy( axis_identity, projectionSource->axis );
		CG_MoveToTag( projectionSource->origin, projectionSource->axis,
			weapon.origin, weapon.axis,
			weaponInfo->tag_projectionsource.origin,
			weaponInfo->tag_projectionsource.axis );
	}

	//expansion
	if( ( effects & EF_STRONG_WEAPON ) && weaponInfo->model[EXPANSION] )
	{
		if( CG_GrabTag( tag, &weapon, "tag_expansion" ) )
		{
			entity_t expansion;
			memset( &expansion, 0, sizeof( expansion ) );
			Vector4Set( expansion.shaderRGBA, 255, 255, 255, ent->shaderRGBA[3] );
			expansion.model = weaponInfo->model[EXPANSION];
			expansion.scale = ent->scale;
			expansion.renderfx = ent->renderfx;
			expansion.frame = 0;
			expansion.oldframe = 0;

			CG_PlaceModelOnTag( &expansion, &weapon, tag );

			CG_AddColoredOutLineEffect( &expansion, effects, 0, 0, 0, 255 );

			if( !( effects & EF_RACEGHOST ) )
				CG_AddEntityToScene( &expansion ); // skelmod

			CG_AddShellEffects( &expansion, effects );
		}
	}

// barrel
	if( weaponInfo->model[BARREL] ) {
		if( CG_GrabTag( tag, &weapon, "tag_barrel" ) ) {
			orientation_t barrel_recoiled;
			vec3_t rotangles = { 0, 0, 0 };
			vec3_t placementOrigin;
			entity_t barrelEnt;
			memset( &barrelEnt, 0, sizeof( barrelEnt ) );
			Vector4Set( barrelEnt.shaderRGBA, 255, 255, 255, ent->shaderRGBA[3] );
			barrelEnt.model = weaponInfo->model[BARREL];
			barrelEnt.scale = ent->scale;
			barrelEnt.renderfx = ent->renderfx;
			barrelEnt.frame = 0;
			barrelEnt.oldframe = 0;

			VectorCopy( tag->origin, placementOrigin );

			if( barrel.time > cg.time ) {
				unsigned int elapsed = cg.time - barrel.startTime;
				unsigned int timeRemaining = barrel.time - cg.time;

				if( elapsed < weaponInfo->barrelInfo.initialDelay ) {
					intensity = 0.0f;
				} else {
					unsigned int activeElapsed = elapsed - weaponInfo->barrelInfo.initialDelay;
					unsigned int activeDuration = weaponInfo->barrelInfo.time - weaponInfo->barrelInfo.initialDelay;

					intensity = (float)timeRemaining / (float)weaponInfo->barrelInfo.time;
					rotangles[2] = anglemod( 360.0f * weaponInfo->barrelInfo.speed * intensity * intensity );

					if( CG_GrabTag( &barrel_recoiled, &weapon, "tag_recoil" ) ) {
						float recoilIntensity;

						if( activeElapsed < weaponInfo->barrelInfo.recoilDelay ) {
							recoilIntensity = 1.0f;
						} else {
							unsigned int recoilElapsed = activeElapsed - weaponInfo->barrelInfo.recoilDelay;
							unsigned int recoilDuration = activeDuration - weaponInfo->barrelInfo.recoilDelay;
							recoilIntensity = recoilDuration > 0 ? 1.0f - (float)recoilElapsed / (float)recoilDuration : 0.0f;
						}

						clamp( recoilIntensity, 0.0f, 1.0f );
						VectorLerp( tag->origin, recoilIntensity, barrel_recoiled.origin, placementOrigin );
					}
				}
			}

			VectorCopy( placementOrigin, tag->origin );
			AnglesToAxis( rotangles, barrelEnt.axis );
			CG_PlaceRotatedModelOnTag( &barrelEnt, &weapon, tag );
			CG_AddColoredOutLineEffect( &barrelEnt, effects, 0, 0, 0, ent->shaderRGBA[3] );
			if( !( effects & EF_RACEGHOST ) )
				CG_AddEntityToScene( &barrelEnt );
			CG_AddShellEffects( &barrelEnt, effects );
		}
	}

	// barrel 2 Added for additional rotating detail (eg: double chaingun)
	if( weaponInfo->model[BARREL2] ) {
		if( CG_GrabTag( tag, &weapon, "tag_barrel2" ) ) {
			orientation_t barrel_recoiled;
			vec3_t rotangles = { 0, 0, 0 };
			vec3_t placementOrigin;
			entity_t barrel2Ent;
			memset( &barrel2Ent, 0, sizeof( barrel2Ent ) );
			Vector4Set( barrel2Ent.shaderRGBA, 255, 255, 255, ent->shaderRGBA[3] );
			barrel2Ent.model = weaponInfo->model[BARREL2];
			barrel2Ent.scale = ent->scale;
			barrel2Ent.renderfx = ent->renderfx;
			barrel2Ent.frame = 0;
			barrel2Ent.oldframe = 0;

			VectorCopy( tag->origin, placementOrigin );

			if( barrel2.time > cg.time ) {
				unsigned int elapsed = cg.time - barrel2.startTime;
				unsigned int timeRemaining = barrel2.time - cg.time;

				if( elapsed < weaponInfo->barrel2Info.initialDelay ) {
					intensity = 0.0f;
				} else {
					unsigned int activeElapsed = elapsed - weaponInfo->barrel2Info.initialDelay;
					unsigned int activeDuration = weaponInfo->barrel2Info.time - weaponInfo->barrel2Info.initialDelay;
					intensity = (float)timeRemaining / (float)weaponInfo->barrel2Info.time;
					rotangles[2] = anglemod( 360.0f * weaponInfo->barrel2Info.speed * intensity * intensity );

					if( CG_GrabTag( &barrel_recoiled, &weapon, "tag_recoil2" ) ) {
						float recoilIntensity;

						if( activeElapsed < weaponInfo->barrel2Info.recoilDelay ) {
							recoilIntensity = 1.0f;
						} else {
							unsigned int recoilElapsed = activeElapsed - weaponInfo->barrel2Info.recoilDelay;
							unsigned int recoilDuration = activeDuration - weaponInfo->barrel2Info.recoilDelay;
							recoilIntensity = recoilDuration > 0 ? 1.0f - (float)recoilElapsed / (float)recoilDuration : 0.0f;
						}

						clamp( recoilIntensity, 0.0f, 1.0f );
						VectorLerp( tag->origin, recoilIntensity, barrel_recoiled.origin, placementOrigin );
					}
				}
			}

			VectorCopy( placementOrigin, tag->origin );
			AnglesToAxis( rotangles, barrel2Ent.axis );
			CG_PlaceRotatedModelOnTag( &barrel2Ent, &weapon, tag );
			CG_AddColoredOutLineEffect( &barrel2Ent, effects, 0, 0, 0, ent->shaderRGBA[3] );
			if( !( effects & EF_RACEGHOST ) )
				CG_AddEntityToScene( &barrel2Ent );
			CG_AddShellEffects( &barrel2Ent, effects );
		}
	}

	if( flash_time < cg.time )
		return;

	// flash
	if( !CG_GrabTag( tag, &weapon, "tag_flash" ) )
		return;

	if( weaponInfo->model[FLASH] )
	{
		entity_t flash;
		uint8_t c;

		if( weaponInfo->flashFade )
		{
			intensity = (float)( flash_time - cg.time )/(float)weaponInfo->flashTime;
			c = ( uint8_t )( 255 * intensity );
		}
		else
		{
			intensity = 1.0f;
			c = 255;
		}

		memset( &flash, 0, sizeof( flash ) );
		Vector4Set( flash.shaderRGBA, c, c, c, c );
		flash.model = weaponInfo->model[FLASH];
		flash.scale = ent->scale;
		flash.renderfx = ent->renderfx | RF_NOSHADOW;
		flash.frame = 0;
		flash.oldframe = 0;

		CG_PlaceModelOnTag( &flash, &weapon, tag );

		if( !( effects & EF_RACEGHOST ) )
			CG_AddEntityToScene( &flash );

		CG_AddLightToScene( flash.origin, weaponInfo->flashRadius * intensity,
			weaponInfo->flashColor[0], weaponInfo->flashColor[1], weaponInfo->flashColor[2] );
	}
}
