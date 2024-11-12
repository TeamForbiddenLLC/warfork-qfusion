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
#include "steam/isteamgameserver.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/isteamuser.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

#include "../os.h"
#include "./child_private.h"
#include "../steamshim_private.h"
#include "../steamshim.h"
#include <cstdint>
#include "steam/steamnetworkingtypes.h"
#include "write_utils.h"


SteamCallbacks::SteamCallbacks()
    : m_CallbackCreateBeacon( this, &SteamCallbacks::OnCreateBeacon ),
    m_CallbackPersonaStateChange( this, &SteamCallbacks::OnPersonaStateChange),
    m_CallbackGameRichPresenceJoinRequested( this, &SteamCallbacks::OnGameJoinRequested),
    m_CallbackSteamNetConnectionStatusChanged( this, &SteamCallbacks::OnSteamNetConnectionStatusChanged),
    m_CallbackSteamNetConnectionStatusChanged_SV( this, &SteamCallbacks::OnSteamNetConnectionStatusChanged_SV),
    m_CallbackSteamServersConnectedSV( this, &SteamCallbacks::OnSteamServersConnected_SV),
    m_CallbackPolicyResponse( this, &SteamCallbacks::OnPolicyResponse_SV)
{
}

void SteamCallbacks::OnSteamServersConnected_SV(SteamServersConnected_t *pCallback)
{
    printf("SteamServersConnected_t\n");
}

void SteamCallbacks::OnPolicyResponse_SV(GSPolicyResponse_t *pCallback)
{
    printf("VAC enabled: %d\n", pCallback->m_bSecure);

    uint64_t steamID = SteamGameServer()->GetSteamID().ConvertToUint64();

    struct p2p_listen_recv_s recv;
    prepared_rpc_packet( &GCurrent_p2p_listen_request.common, &recv );
    recv.steamID = steamID;
    recv.success = true;
    write_packet( GPipeWrite, &recv, sizeof recv );
}

void SteamCallbacks::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pCallback)
{
    printf("SteamNetConnectionStatusChangedCallback_t\n");
    printf("status: %llu\n", pCallback->m_info.m_eState);
    if (pCallback->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected) {

		struct p2p_connect_recv_s recv;
		prepared_rpc_packet( &GCurrent_p2p_connect_request.common, &recv );
		recv.success = true;
		recv.steamID = pCallback->m_info.m_identityRemote.GetSteamID64();
		write_packet( GPipeWrite, &recv, sizeof( steam_id_rpc_s ) );
    }
}

void SteamCallbacks::OnSteamNetConnectionStatusChanged_SV(SteamNetConnectionStatusChangedCallback_t *pCallback)
{
    HSteamNetConnection conn = pCallback->m_hConn;
    SteamNetConnectionInfo_t info = pCallback->m_info;

    ESteamNetworkingConnectionState old = pCallback->m_eOldState;


    if (info.m_hListenSocket && old == k_ESteamNetworkingConnectionState_None && info.m_eState == k_ESteamNetworkingConnectionState_Connecting)
    {
        printf("New connection\n");
        printf("id: %llu\n", info.m_identityRemote.GetSteamID64());
        EResult res = SteamGameServerNetworkingSockets()->AcceptConnection(pCallback->m_hConn);
        if (res != k_EResultOK)
        {
            printf("Failed to accept connection\n");
            SteamGameServerNetworkingSockets()->CloseConnection(conn, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection", false);
            return;
        }

        struct p2p_new_connection_evt_s evt;
        evt.cmd = EVT_P2P_NEW_CONNECTION;
        evt.steamID = info.m_identityRemote.GetSteamID64();
        evt.handle = conn;
        write_packet(GPipeWrite, &evt, sizeof(struct p2p_new_connection_evt_s));
    }

    printf("connection status: %llu\n", pCallback->m_info.m_eState);
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

