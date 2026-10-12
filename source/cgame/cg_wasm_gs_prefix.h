/*
 * WASM static link: game and cgame both used one copy of gameshared globals
 * (gs, module_*, Pmove). CG_InitGameShared then wiped the game module and
 * replaced PMoveTouchTriggers with the client predictor, so door/button
 * triggers never fired on the listen server.
 * Prefix cgame's gameshared symbols so each module has its own copy.
 */
#ifndef CG_WASM_GS_PREFIX_H
#define CG_WASM_GS_PREFIX_H

#if defined( __WASM__ ) || defined( __EMSCRIPTEN__ )

#define gs cg_gs
#define module_Printf cg_module_Printf
#define module_Error cg_module_Error
#define module_Malloc cg_module_Malloc
#define module_Free cg_module_Free
#define module_Trace cg_module_Trace
#define module_GetEntityState cg_module_GetEntityState
#define module_PointContents cg_module_PointContents
#define module_PredictedEvent cg_module_PredictedEvent
#define module_PMoveTouchTriggers cg_module_PMoveTouchTriggers
#define module_RoundUpToHullSize cg_module_RoundUpToHullSize
#define module_GetConfigString cg_module_GetConfigString
#define Pmove CG_Pmove
#define GS_SlideMove CG_GS_SlideMove
#define GS_ClipVelocity CG_GS_ClipVelocity
#define GS_SnapVelocity CG_GS_SnapVelocity
#define GS_SnapPosition CG_GS_SnapPosition
#define GS_SnapInitialPosition CG_GS_SnapInitialPosition
#define GS_LinearMovement CG_GS_LinearMovement
#define GS_LinearMovementDelta CG_GS_LinearMovementDelta
#define GS_Armor_TagForCount CG_GS_Armor_TagForCount
#define GS_Armor_MaxCountForTag CG_GS_Armor_MaxCountForTag
#define GS_Armor_PickupCountForTag CG_GS_Armor_PickupCountForTag
#define GS_TeamName CG_GS_TeamName
#define GS_DefaultTeamName CG_GS_DefaultTeamName
#define GS_TeamSkinName CG_GS_TeamSkinName
#define GS_Teams_TeamFromName CG_GS_Teams_TeamFromName
#define GS_IsTeamDamage CG_GS_IsTeamDamage
#define GS_UpdateBaseAnims CG_GS_UpdateBaseAnims
#define GS_PModel_AnimToFrame CG_GS_PModel_AnimToFrame
#define GS_PlayerModel_ClearEventAnimations CG_GS_PlayerModel_ClearEventAnimations
#define GS_PlayerModel_AddAnimation CG_GS_PlayerModel_AddAnimation
#define GS_SetGametypeName CG_GS_SetGametypeName
#define GS_Obituary CG_GS_Obituary
#define GS_TouchPushTrigger CG_GS_TouchPushTrigger
#define GS_WaterLevel CG_GS_WaterLevel
#define GS_BBoxForEntityState CG_GS_BBoxForEntityState
#define GS_FrameForTime CG_GS_FrameForTime
#define GS_GetWeaponDef CG_GS_GetWeaponDef
#define GS_InitWeapons CG_GS_InitWeapons
#define GS_SelectBestWeapon CG_GS_SelectBestWeapon
#define GS_CheckAmmoInWeapon CG_GS_CheckAmmoInWeapon
#define GS_FiredefForPlayerState CG_GS_FiredefForPlayerState
#define GS_ThinkPlayerWeapon CG_GS_ThinkPlayerWeapon
#define GS_TraceBullet CG_GS_TraceBullet
#define GS_TraceLaserBeam CG_GS_TraceLaserBeam
#define GS_TraceCurveLaserBeam CG_GS_TraceCurveLaserBeam
#define GS_AddLaserbeamPoint CG_GS_AddLaserbeamPoint
#define G_GetLaserbeamPoint CG_G_GetLaserbeamPoint
#define GS_FindItemByTag CG_GS_FindItemByTag
#define GS_FindItemByClassname CG_GS_FindItemByClassname
#define GS_FindItemByName CG_GS_FindItemByName
#define GS_Cmd_UseItem CG_GS_Cmd_UseItem
#define GS_Cmd_NextWeapon_f CG_GS_Cmd_NextWeapon_f
#define GS_Cmd_PrevWeapon_f CG_GS_Cmd_PrevWeapon_f
#define playerbox_stand_mins cg_playerbox_stand_mins
#define playerbox_stand_maxs cg_playerbox_stand_maxs
#define playerbox_stand_viewheight cg_playerbox_stand_viewheight
#define playerbox_crouch_mins cg_playerbox_crouch_mins
#define playerbox_crouch_maxs cg_playerbox_crouch_maxs
#define playerbox_crouch_viewheight cg_playerbox_crouch_viewheight
#define playerbox_gib_mins cg_playerbox_gib_mins
#define playerbox_gib_maxs cg_playerbox_gib_maxs
#define playerbox_gib_viewheight cg_playerbox_gib_viewheight
#define item_box_mins cg_item_box_mins
#define item_box_maxs cg_item_box_maxs
#define gs_weaponStateNames cg_gs_weaponStateNames
#define gs_weaponDefs cg_gs_weaponDefs
#define itemdefs cg_itemdefs

#endif

#endif
