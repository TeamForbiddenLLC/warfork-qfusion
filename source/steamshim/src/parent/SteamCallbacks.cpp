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
#include "./parent_private.h"
#include "../steamshim_private.h"
#include "../steamshim.h"

SteamCallbacks::SteamCallbacks()
    : m_CallbackCreateBeacon( this, &SteamCallbacks::OnCreateBeacon ),
    m_CallbackPersonaStateChange( this, &SteamCallbacks::OnPersonaStateChange)
{
} 

void SteamCallbacks::OnCreateBeacon(UserStatsReceived_t *pCallback)
{
} 

void SteamCallbacks::OnPersonaStateChange(PersonaStateChange_t *pCallback)
{
    if (pCallback->m_nChangeFlags & k_EPersonaChangeAvatar){
        TransmitAvatar(pCallback->m_ulSteamID);
    }
} 
void TransmitAvatar(uint64 id){
    int handle = SteamFriends()->GetSmallFriendAvatar(id);

    uint8_t image[4096];
    SteamUtils()->GetImageRGBA(handle, image, sizeof image);

    pipebuff_t msg;
    msg.WriteByte(SHIMEVENT_AVATARRECIEVED);
    msg.WriteLong(id);
    msg.WriteData(image, sizeof image);
    msg.Transmit();
}

