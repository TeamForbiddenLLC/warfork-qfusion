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
#include "../gameshared/q_arch.h"
#define STEAM_DEFINE_INTERFACE_IMPL 
#include "../steamshim/src/mod_steam.h"
#define REF_DEFINE_INTERFACE_IMPL 1
#include "../ref_base/ref_mod.h"
#define FS_DEFINE_INTERFACE_IMPL 1
#include "../qcommon/mod_fs.h"

#include "cg_local.h"


cgame_import_t CGAME_IMPORT;

/*
* GetCGameAPI
* 
* Returns a pointer to the structure with all entry points
*/
extern "C" QF_DLL_EXPORT cgame_export_t *GetCGameAPI( cgame_import_t *import )
{
	static cgame_export_t globals;


	Q_ImportRefModule(&import->ref_import);
	CGAME_IMPORT = *import;
	Q_ImportSteamModule( &import->steam_import );

	fs_import = *import->fsImport;
	globals.API = CG_API;

	globals.Init = CG_Init;
	globals.Reset = CG_Reset;
	globals.Shutdown = CG_Shutdown;

	globals.ConfigString = CG_ConfigString;

	globals.EscapeKey = CG_EscapeKey;

	globals.GetEntitySpatilization = CG_GetEntitySpatilization;

	globals.GetSensitivityScale = CG_GetSensitivityScale;

	globals.Trace = CG_Trace;
	globals.RenderView = CG_RenderView;

	globals.NewFrameSnapshot = CG_NewFrameSnap;

	globals.UpdateInput = CG_UpdateInput;
	globals.ClearInputState = CG_ClearInputState;

	globals.GetButtonBits = CG_GetButtonBits;
	globals.AddViewAngles = CG_AddViewAngles;
	globals.AddMovement = CG_AddMovement;

	globals.TouchEvent = CG_TouchEvent;
	globals.IsTouchDown = CG_IsTouchDown;

	globals.GetBlocklistItem = CG_GetBlocklistItem;
	globals.PlayVoice = CG_PlayVoice;

	return &globals;
}

#if defined ( HAVE_DLLMAIN ) && !defined ( CGAME_HARD_LINKED )
int WINAPI DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
