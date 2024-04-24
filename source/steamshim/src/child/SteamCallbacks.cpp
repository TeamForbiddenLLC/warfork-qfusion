/*
Copyright (C) 2023 coolelectronics

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#include "steam/isteamfriends.h"
#include "steam/isteamuser.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

#include "../os.h"
#include "./child_private.h"
#include "../steamshim_private.h"
#include "../steamshim.h"
#include <cstdint>
#include "write_utils.h"


SteamCallbacks::SteamCallbacks()
    : m_CallbackCreateBeacon( this, &SteamCallbacks::OnCreateBeacon ),
    m_CallbackPersonaStateChange( this, &SteamCallbacks::OnPersonaStateChange),
    m_CallbackGameRichPresenceJoinRequested( this, &SteamCallbacks::OnGameJoinRequested)
{
}


void SteamCallbacks::OnCreateBeacon(UserStatsReceived_t *pCallback)
{
}
void SteamCallbacks::OnGameJoinRequested(GameRichPresenceJoinRequested_t *pCallback)
{

    join_request_evt_s evt;
    evt.cmd = EVT_GAME_JOIN;
	evt.steamID = pCallback->m_steamIDFriend.ConvertToUint64();
	memcpy(evt.rgchConnect, pCallback->m_rgchConnect, k_cchMaxRichPresenceValueLength);
    write_packet(GPipeWrite, &evt, sizeof(struct join_request_evt_s ));
}

void SteamCallbacks::OnPersonaStateChange(PersonaStateChange_t *pCallback)
{
    persona_changes_evt_s evt;
    evt.cmd = EVT_PERSONA_CHANGED;
	evt.avatar_changed = pCallback->m_nChangeFlags & k_EPersonaChangeAvatar;
    evt.steamID = pCallback->m_ulSteamID; 
    write_packet(GPipeWrite, &evt, sizeof(struct persona_changes_evt_s));
} 

