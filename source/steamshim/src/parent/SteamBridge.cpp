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


//#define GAME_CLIENT_LAUNCH_NAME "testapp"
#include "steam/isteamuser.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

#include "../os.h"

// class SteamBridge;
//
// class SteamBridge
// {
// public:
//     SteamBridge(PipeType _fd);
// 	STEAM_CALLBACK(SteamBridge, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived);
// 	STEAM_CALLBACK(SteamBridge, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored);
// 	STEAM_CALLBACK(SteamBridge, OnValidateAuthTicket, ValidateAuthTicketResponse_t, m_CallbackValidateAuthTicket);
//
// private:
//     PipeType fd;
// };
//
//
// SteamBridge::SteamBridge(PipeType _fd)
//     : m_CallbackUserStatsReceived( this, &SteamBridge::OnUserStatsReceived )
// 	, m_CallbackUserStatsStored( this, &SteamBridge::OnUserStatsStored )
//     , m_CallbackValidateAuthTicket( this, &SteamBridge::OnValidateAuthTicket )
// 	, fd(_fd)
// {
// } 
//
// void SteamBridge::OnUserStatsReceived(UserStatsReceived_t *pCallback)
// {
// 	if (GAppID != pCallback->m_nGameID) return;
// 	if (GUserID != pCallback->m_steamIDUser.ConvertToUint64()) return;
//     writeStatsReceived(fd, pCallback->m_eResult == k_EResultOK);
// } 
//
// void SteamBridge::OnUserStatsStored(UserStatsStored_t *pCallback)
// {
// 	if (GAppID != pCallback->m_nGameID) return;
//     writeStatsStored(fd, pCallback->m_eResult == k_EResultOK);
// } 
//
// void SteamBridge::OnValidateAuthTicket(ValidateAuthTicketResponse_t *pCallback)
// {
//     PIPE_Init();
//     PIPE_WriteLong(pCallback->m_SteamID.ConvertToUint64());
//     PIPE_WriteInt(pCallback->m_eAuthSessionResponse);
//     PIPE_SendEvt(fd, SHIMEVENT_AUTHSESSIONVALIDATED, true);
// }
