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

#include <cassert>
#include <chrono>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stdlib.h>
#include <thread>
#include <vector>

#include "../os.h"
#include "../steamshim_private.h"
#include "ServerBrowser.h"
#include "child_private.h"
#include "steam/isteamfriends.h"
#include "steam/isteamgameserver.h"
#include "steam/isteammatchmaking.h"
#include "steam/isteamnetworking.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/isteamnetworkingutils.h"
#include "steam/isteamremotestorage.h"
#include "steam/isteamugc.h"
#include "steam/isteamuser.h"
#include "steam/isteamutils.h"
#include "steam/steam_api.h"
#include "steam/steam_api_common.h"
#include "steam/steam_gameserver.h"
#include "steam/steamclientpublic.h"

#include "../packet_utils.h"
#include "rpc_async_handles.h"
#include "steam/steamnetworkingtypes.h"
#include "write_utils.h"

static bool GRunServer = false;
static bool GRunClient = false;

static ISteamUserStats *GSteamStats = NULL;
static ISteamUtils *GSteamUtils = NULL;
static ISteamUser *GSteamUser = NULL;
static AppId_t GAppID = 0;
static uint64 GUserID = 0;

static ISteamGameServer *GSteamGameServer = NULL;
static ISteamUGC *GSteamUGC = NULL;
ServerBrowser *GServerBrowser = NULL;
static time_t time_since_last_pump = 0;

static void handleSteamConnectionStatusChanged( steam_cmd_s cmd, SteamNetConnectionStatusChangedCallback_t *pCallback )
{
	struct p2p_net_connection_changed_evt_s evt;
	evt.cmd = cmd;
	evt.hConn = pCallback->m_hConn;
	evt.listenSocket = pCallback->m_info.m_hListenSocket;
	evt.oldState = pCallback->m_eOldState;
	evt.state = pCallback->m_info.m_eState;
	evt.identityRemoteSteamID = pCallback->m_info.m_identityRemote.GetSteamID64();
	write_packet( GPipeWrite, &evt, sizeof( p2p_net_connection_changed_evt_s ) );
}

static void handleRecvMessageRPC( const steam_rpc_shim_common_s *req, SteamNetworkingMessage_t **msgs, int num_messages, uint64_t steamID, uint32_t handle )
{
	if( num_messages == -1 ) {
		recv_messages_recv_s recv;
		recv.steamID = steamID;
		recv.handle = handle;
		recv.count = 0;
		prepared_rpc_packet( req, &recv );
		write_packet( GPipeWrite, &recv, sizeof( struct recv_messages_recv_s ) );
		printf( "invalid handle for req: %d\n", req->cmd );
		return;
	}

	size_t total = 0;
	for( int i = 0; i < num_messages; i++ ) {
		total += msgs[i]->GetSize();
	}
	auto recv = (struct recv_messages_recv_s *)malloc( sizeof( struct recv_messages_recv_s ) + total );
	recv->steamID = steamID;
	recv->handle = handle;
	recv->count = num_messages;
	size_t offset = 0;

	for( int i = 0; i < num_messages; i++ ) {
		recv->messageinfo[i].count = msgs[i]->GetSize();
		memcpy( recv->buffer + offset, msgs[i]->GetData(), msgs[i]->GetSize() );
		offset += msgs[i]->GetSize();
	}
	prepared_rpc_packet( req, recv );
	write_packet( GPipeWrite, recv, sizeof( struct recv_messages_recv_s ) + total );
	free( recv );
}

static void processEVT( steam_evt_pkt_s *req, size_t size )
{
	switch( req->common.cmd ) {
		case EVT_HEART_BEAT: {
			time( &time_since_last_pump );
			break;
			default:
				assert( 0 ); // unhandled packet
				break;
		}
	}
}

static void processRPC( steam_rpc_pkt_s *req, size_t size )
{
	switch( req->common.cmd ) {
		case RPC_PUMP: {
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_WORKSHOP_SUBMIT_ITEM: {
			dbgprintf( "RPC_WORKSHOP_SUBMIT_ITEM handle=%" PRIu64 " note=\"%s\"\n",
				(uint64_t)req->workshop_submit_item.handle, (const char *)req->workshop_submit_item.buf );
			SteamAPICall_t call = GSteamUGC->SubmitItemUpdate( req->workshop_submit_item.handle, (const char *)req->workshop_submit_item.buf );
			dbgprintf( "  -> api_call=%" PRIu64 "\n", (uint64_t)call );
			steam_async_push_rpc_shim( call, &req->common );
			break;
		}
		case RPC_WORKSHOP_BEGIN_ITEM_UPDATE: {
			dbgprintf( "RPC_WORKSHOP_BEGIN_ITEM_UPDATE file_id=%" PRIu64 "\n",
				(uint64_t)req->workshop_start_item_update.publish_item_file );
			STEAM_UGCUpdateHandle_t handle = GSteamUGC->StartItemUpdate( GAppID, req->workshop_start_item_update.publish_item_file );
			dbgprintf( "  -> handle=%" PRIu64 " valid=%d\n", (uint64_t)handle, handle != k_UGCUpdateHandleInvalid );
			struct start_item_update_recv_s recv;
			recv.handle = handle;
			recv.success = handle != k_UGCUpdateHandleInvalid;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( start_item_update_recv_s ) );
			break;
		}
		case RPC_WORKSHOP_CREATE_ITEM: {
			dbgprintf( "RPC_WORKSHOP_CREATE_ITEM app_id=%u\n", (unsigned)GAppID );
			SteamAPICall_t call = GSteamUGC->CreateItem( GAppID, k_EWorkshopFileTypeGameManagedItem );
			dbgprintf( "  -> api_call=%" PRIu64 "\n", (uint64_t)call );
			steam_async_push_rpc_shim( call, &req->common );
			break;
		}
		case RPC_WORKSHOP_ITEM_SET_TITLE: {
			dbgprintf( "RPC_WORKSHOP_ITEM_SET_TITLE handle=%" PRIu64 " title=\"%s\"\n",
				(uint64_t)req->workshop_set_title.handle, (const char *)req->workshop_set_title.buf );
			struct success_recv_s recv;
			recv.success = GSteamUGC->SetItemTitle( req->workshop_set_title.handle, (const char *)req->workshop_set_title.buf );
			dbgprintf( "  -> success=%d\n", recv.success );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( success_recv_s ) );
			break;
		}
		case RPC_WORKSHOP_ITEM_SET_DESCRIPTION: {
			dbgprintf( "RPC_WORKSHOP_ITEM_SET_DESCRIPTION handle=%" PRIu64 "\n",
				(uint64_t)req->workshop_set_description.handle );
			struct success_recv_s recv;
			recv.success = GSteamUGC->SetItemDescription( req->workshop_set_description.handle, (const char *)req->workshop_set_description.buf );
			dbgprintf( "  -> success=%d\n", recv.success );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( success_recv_s ) );
			break;
		}
		case RPC_WORKSHOP_ITEM_SET_ITEM_CONTENT: {
			dbgprintf( "RPC_WORKSHOP_ITEM_SET_ITEM_CONTENT handle=%" PRIu64 " path=\"%s\"\n",
				(uint64_t)req->workshop_set_item_content.handle, (const char *)req->workshop_set_item_content.buf );
			struct success_recv_s recv;
			recv.success = GSteamUGC->SetItemContent( req->workshop_set_item_content.handle, (const char *)req->workshop_set_item_content.buf );
			dbgprintf( "  -> success=%d\n", recv.success );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( success_recv_s ) );
			break;
		}
		case RPC_WORKSHOP_ITEM_SET_VISIBILITY: {
			dbgprintf( "RPC_WORKSHOP_ITEM_SET_VISIBILITY item_id=%" PRIu64 " visibility=%d\n",
				(uint64_t)req->workshop_set_visibility.handle, (int)req->workshop_set_visibility.visibility );
			struct success_recv_s recv;
			recv.success = GSteamUGC->SetItemVisibility( req->workshop_set_visibility.handle, (ERemoteStoragePublishedFileVisibility)req->workshop_set_visibility.visibility );
			dbgprintf( "  -> success=%d\n", recv.success );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( success_recv_s ) );
			break;
		}
		case RPC_WORKSHOP_ITEM_SET_TAGS: {
			dbgprintf( "RPC_WORKSHOP_ITEM_SET_TAGS handle=%" PRIu64 " num_tags=%u\n",
				(uint64_t)req->workshop_set_tags.handle, (unsigned)req->workshop_set_tags.num_tags );
			struct SteamParamStringArray_t tags;
			tags.m_ppStrings = (const char **)malloc( sizeof( char * ) * req->workshop_set_tags.num_tags );
			tags.m_nNumStrings = 0;
			const char *c = req->workshop_set_tags.buffer;
			for( size_t i = 0; i < req->workshop_set_tags.num_tags; i++ ) {
				dbgprintf( "  tag[%zu]=\"%s\"\n", i, c );
				tags.m_ppStrings[tags.m_nNumStrings++] = c;
				while( *c != '\0' ) {
					c++;
				}
			}
			struct success_recv_s recv;
			recv.success = GSteamUGC->SetItemTags( req->workshop_set_tags.handle, &tags );
			dbgprintf( "  -> success=%d\n", recv.success );
			free( tags.m_ppStrings );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( success_recv_s ) );
			break;
		}
		case RPC_WORKSHOP_ITEM_SET_PREVIEW: {
			dbgprintf( "RPC_WORKSHOP_ITEM_SET_PREVIEW handle=%" PRIu64 " path=\"%s\"\n",
				(uint64_t)req->workshop_set_preview.handle, (const char *)req->workshop_set_preview.buf );
			struct success_recv_s recv;
			recv.success = GSteamUGC->SetItemPreview( req->workshop_set_preview.handle, (const char *)req->workshop_set_preview.buf );
			dbgprintf( "  -> success=%d\n", recv.success );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( success_recv_s ) );
			break;
		}
		case RPC_WORKSHOP_INSTALLED_INFO: {
			dbgprintf( "RPC_WORKSHOP_INSTALLED_INFO workshop_id=%" PRIu64 "\n",
				(uint64_t)req->steam_workshop_item.workshop_id );
			alignas( steam_workshop_install_info_s ) char recv_storage[sizeof( steam_workshop_install_info_s )] = {};
			steam_workshop_install_info_s &recv = *reinterpret_cast<steam_workshop_install_info_s *>( recv_storage );
			char folder[1024] = { 0 };

			prepared_rpc_packet( &req->common, &recv );
			recv.workshop_id = req->steam_workshop_item.workshop_id;
			if( GSteamUGC != NULL ) {
				uint64 size_on_disk;
				recv.success = GSteamUGC->GetItemInstallInfo( req->steam_workshop_item.workshop_id, &size_on_disk, folder, sizeof( folder ), &recv.time_stamp );
				recv.size_on_disk = size_on_disk;
			}

			dbgprintf( "  -> success=%d folder=\"%s\"\n", recv.success, folder );
			const size_t folderLen = recv.success ? strlen( folder ) : 0;
			const uint32_t bufferSize = sizeof( recv ) + folderLen + 1;
			writePipe( GPipeWrite, &bufferSize, sizeof( uint32_t ) );
			writePipe( GPipeWrite, &recv, sizeof( recv ) );
			writePipe( GPipeWrite, folder, folderLen + 1 );
			break;
		}
		case RPC_WORKSHOP_QUERY_ITEM_DETAILS: {
			dbgprintf( "RPC_WORKSHOP_QUERY_ITEM_DETAILS num_ids=%u\n", (unsigned)req->steam_workshop_items.num_ids );
			UGCQueryHandle_t query = GSteamUGC->CreateQueryUGCDetailsRequest( (uint64 *)req->steam_workshop_items.workshop_ids, req->steam_workshop_items.num_ids );
			SteamAPICall_t call = GSteamUGC->SendQueryUGCRequest( query );
			dbgprintf( "  -> api_call=%" PRIu64 " valid=%d\n", (uint64_t)call, call != k_uAPICallInvalid );
			if( call == k_uAPICallInvalid ) {
				GSteamUGC->ReleaseQueryUGCRequest( query );
			} else {
				steam_async_push_rpc_shim( call, &req->common );
			}
			break;
		}
		case RPC_WORKSHOP_REFRESH_SUBSCRIBED_ITEMS: {
			std::vector<PublishedFileId_t> items( GSteamUGC->GetNumSubscribedItems() );
			const uint32 numItems = GSteamUGC->GetSubscribedItems( items.data(), items.size() );
			dbgprintf( "RPC_WORKSHOP_REFRESH_SUBSCRIBED_ITEMS num_subscribed=%u\n", (unsigned)numItems );

			const uint32_t pkt_size = sizeof( workshop_subscribed_items_recv_s ) + numItems * sizeof( uint64_t );
			std::vector<char> pkt( pkt_size );
			workshop_subscribed_items_recv_s *recv = reinterpret_cast<workshop_subscribed_items_recv_s *>( pkt.data() );
			prepared_rpc_packet( &req->common, recv );
			recv->num_ids = numItems;
			for( uint32 i = 0; i < numItems; i++ ) {
				recv->ids[i] = items[i];
			}
			write_packet( GPipeWrite, pkt.data(), pkt_size );
			break;
		}
		case RPC_AUTHSESSION_TICKET: {
			struct auth_session_ticket_recv_s recv;
			prepared_rpc_packet( &req->common, &recv );
			GSteamUser->GetAuthSessionTicket( recv.ticket, AUTH_TICKET_MAXSIZE, &recv.pcbTicket );
			write_packet( GPipeWrite, &recv, sizeof( struct auth_session_ticket_recv_s ) );
			break;
		}
		case RPC_PERSONA_NAME: {
			const char *name = SteamFriends()->GetPersonaName();
			struct buffer_rpc_s recv;
			prepared_rpc_packet( &req->common, &recv );

			const uint32_t bufferSize = strlen( name ) + 1 + sizeof( struct buffer_rpc_s );
			writePipe( GPipeWrite, &bufferSize, sizeof( uint32_t ) );
			writePipe( GPipeWrite, &recv, sizeof( struct buffer_rpc_s ) );
			writePipe( GPipeWrite, name, strlen( name ) + 1 );
			break;
		}
		case RPC_SRV_P2P_ACCEPT_CONNECTION: {
			struct p2p_accept_connection_recv_s recv;
			EResult res = SteamGameServerNetworkingSockets()->AcceptConnection( req->p2p_accept_connection_req.handle );
			prepared_rpc_packet( &req->common, &recv );
			recv.result = res;
			write_packet( GPipeWrite, &recv, sizeof( struct p2p_accept_connection_recv_s ) );
			break;
		}
		case RPC_REQUEST_LAUNCH_COMMAND: {
			uint8_t buffer[sizeof( struct buffer_rpc_s ) + 1024];
			struct buffer_rpc_s *recv = (struct buffer_rpc_s *)buffer;
			prepared_rpc_packet( &req->common, recv );
			const int numberBytes = SteamApps()->GetLaunchCommandLine( (char *)recv->buf, 1024 );
			assert( numberBytes > 0 );
			write_packet( GPipeWrite, &recv, sizeof( buffer_rpc_s ) + numberBytes );
			break;
		}
		case RPC_BEGIN_AUTH_SESSION: {
			const int result = GSteamGameServer->BeginAuthSession( req->begin_auth_session.authTicket, req->begin_auth_session.cbAuthTicket, (uint64)req->begin_auth_session.steamID );
			struct steam_result_recv_s recv;
			prepared_rpc_packet( &req->common, &recv );
			recv.result = result;
			write_packet( GPipeWrite, &recv, sizeof( steam_result_recv_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO: {
			GSteamGameServer->SetAdvertiseServerActive( req->server_info.advertise );
			GSteamGameServer->SetDedicatedServer( req->server_info.dedicated );
			GSteamGameServer->SetMaxPlayerCount( req->server_info.maxPlayerCount );
			GSteamGameServer->SetBotPlayerCount( req->server_info.botPlayerCount );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_result_recv_s ) );
			break;
		}
		case RPC_ACTIVATE_OVERLAY: {
			SteamFriends()->ActivateGameOverlayToUser( "steamid", (uint64)req->open_overlay.id );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_GAME_DATA: {
			GSteamGameServer->SetGameData( (const char *)req->server_game_data.buf );

			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_GAME_DESCRIPTION: {
			GSteamGameServer->SetGameDescription( (const char *)req->server_description.buf );

			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_GAME_TAGS: {
			GSteamGameServer->SetGameDescription( (const char *)req->server_tags.buf );

			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_MAP_NAME: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameDescription( (const char *)req->server_map_name.buf );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_MOD_DIR: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameDescription( (const char *)req->server_mod_dir.buf );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_PRODUCT: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameDescription( (const char *)req->server_product.buf );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_REGION: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameDescription( (const char *)req->server_region.buf );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_SERVERNAME: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameDescription( (const char *)req->server_name.buf );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_SET_RICH_PRESENCE: {
			char *str[2] = { 0 };
			if( unpack_string_array_null( (char *)req->rich_presence.buf, size - sizeof( struct buffer_rpc_s ), str, 2 ) ) {
				SteamFriends()->SetRichPresence( str[0], str[1] );
			}
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_END_AUTH_SESSION: {
			GSteamGameServer->EndAuthSession( (uint64)req->end_auth_session.id );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_SRV_P2P_LISTEN: {
			ISteamGameServer *gameServer = SteamGameServer();
			ISteamNetworkingSockets *networkSocket = SteamGameServerNetworkingSockets();
			gameServer->LogOnAnonymous();
			struct p2p_connect_recv_s recv;
			prepared_rpc_packet( &req->common, &recv );
			recv.handle = networkSocket->CreateListenSocketP2P( 0, 0, nullptr );
			write_packet( GPipeWrite, &recv, sizeof( p2p_connect_recv_s ) );
			break;
		}
		case RPC_SRV_P2P_CLOSE_LISTEN: {
			SteamGameServerNetworkingSockets()->CloseListenSocket( req->p2p_disconnect.handle );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_P2P_CONNECT: {
			SteamNetworkingIdentity id;
			CSteamID steamID = CSteamID( (uint64)req->p2p_connect.steamID );
			id.SetSteamID( steamID );
			struct p2p_connect_recv_s recv;
			prepared_rpc_packet( &req->common, &recv );
			recv.handle = SteamNetworkingSockets()->ConnectP2P( id, 0, 0, nullptr );
			write_packet( GPipeWrite, &recv, sizeof( p2p_connect_recv_s ) );
			break;
		}
		case RPC_SRV_P2P_DISCONNECT: {
			SteamGameServerNetworkingSockets()->CloseConnection( req->p2p_disconnect.handle, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection", false );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_P2P_DISCONNECT: {
			SteamNetworkingSockets()->CloseConnection( req->p2p_disconnect.handle, 0, nullptr, false );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_SRV_P2P_SEND_MESSAGE: {
			assert( req->send_message.count < SDR_MAX_MESSAGE_SIZE );
			SteamGameServerNetworkingSockets()->SendMessageToConnection( req->send_message.handle, req->send_message.buffer, req->send_message.count, req->send_message.messageReliability, nullptr );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_P2P_SEND_MESSAGE: {
			assert( req->send_message.count < SDR_MAX_MESSAGE_SIZE );
			SteamNetworkingSockets()->SendMessageToConnection( req->send_message.handle, req->send_message.buffer, req->send_message.count, req->send_message.messageReliability, nullptr );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_SRV_P2P_RECV_MESSAGES: {
			SteamNetworkingMessage_t *msgs[32];
			SteamNetConnectionInfo_t info;
			SteamGameServerNetworkingSockets()->GetConnectionInfo( req->recv_messages.handle, &info );
			const uint64_t steamid = info.m_identityRemote.GetSteamID().ConvertToUint64();
			const int n = SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection( req->recv_messages.handle, msgs, 1 );
			handleRecvMessageRPC( &req->common, msgs, n, steamid, req->recv_messages.handle );
			break;
		}
		case RPC_P2P_RECV_MESSAGES: {
			SteamNetworkingMessage_t *msgs[32];
			SteamNetConnectionInfo_t info;
			SteamNetworkingSockets()->GetConnectionInfo( req->recv_messages.handle, &info );
			const uint64_t steamid = info.m_identityRemote.GetSteamID().ConvertToUint64();

			const int n = SteamNetworkingSockets()->ReceiveMessagesOnConnection( req->recv_messages.handle, msgs, 1 );
			handleRecvMessageRPC( &req->common, msgs, n, steamid, req->recv_messages.handle );
			break;
		}
		case RPC_GETVOICE: {
			uint32 size;

			SteamUser()->GetAvailableVoice( &size );
			if( size == 0 ) {
				break;
			}

			uint8_t buffer[1024];

			SteamUser()->GetVoice( true, buffer, sizeof buffer, &size );

			auto recv = (struct getvoice_recv_s *)malloc( sizeof( struct getvoice_recv_s ) + size );
			recv->count = size;
			memcpy( recv->buffer, buffer, size );

			prepared_rpc_packet( &req->common, recv );
			write_packet( GPipeWrite, recv, sizeof( struct getvoice_recv_s ) + size );
			break;
		}
		case RPC_DECOMPRESS_VOICE: {
			static uint8 decompressed[VOICE_BUFFER_MAX];

			auto recv = (struct decompress_voice_recv_s *)malloc( sizeof( struct decompress_voice_recv_s ) + sizeof( decompressed ) );

			uint32 size = 0;
			uint32 optimalSampleRate = 11025;
			EVoiceResult e = SteamUser()->DecompressVoice( req->decompress_voice.buffer, req->decompress_voice.count, decompressed, sizeof decompressed, &size, optimalSampleRate );
			if( e != 0 )
				printf( "DecompressVoice failed: %d\n", e );
			memcpy( recv->buffer, decompressed, size );
			recv->count = size;

			prepared_rpc_packet( &req->common, recv );
			write_packet( GPipeWrite, recv, sizeof( struct decompress_voice_recv_s ) + size );
			break;
		}
		case RPC_START_VOICE_RECORDING: {
			SteamUser()->StartVoiceRecording();
			SteamFriends()->SetInGameVoiceSpeaking( SteamUser()->GetSteamID(), true );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_STOP_VOICE_RECORDING: {
			SteamUser()->StopVoiceRecording();
			SteamFriends()->SetInGameVoiceSpeaking( SteamUser()->GetSteamID(), false );
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_REQUEST_STEAM_ID: {
			struct steam_id_rpc_s recv;
			prepared_rpc_packet( &req->common, &recv );
			recv.id = SteamUser()->GetSteamID().ConvertToUint64();
			write_packet( GPipeWrite, &recv, sizeof( steam_id_rpc_s ) );
			break;
		}
		case RPC_REQUEST_AVATAR: {
			uint8_t buffer[sizeof( struct steam_avatar_recv_s ) + STEAM_MAX_AVATAR_SIZE];
			struct steam_avatar_recv_s *recv = (struct steam_avatar_recv_s *)buffer;
			prepared_rpc_packet( &req->common, recv );
			int handle;

			// will recieve the avatar in a persona request cb
			if( SteamFriends()->RequestUserInformation( (uint64)req->avatar_req.steamID, false ) ) {
				goto fail_avatar_image;
			}
			switch( req->avatar_req.size ) {
				case STEAM_AVATAR_SMALL:
					handle = SteamFriends()->GetSmallFriendAvatar( (uint64)req->avatar_req.steamID );
					break;
				case STEAM_AVATAR_MED:
					handle = SteamFriends()->GetMediumFriendAvatar( (uint64)req->avatar_req.steamID );
					break;
				case STEAM_AVATAR_LARGE:
					handle = SteamFriends()->GetLargeFriendAvatar( (uint64)req->avatar_req.steamID );
					break;
				default:
					recv->width = 0;
					recv->height = 0;
					goto fail_avatar_image;
			}
			SteamUtils()->GetImageSize( handle, &recv->width, &recv->height );
			SteamUtils()->GetImageRGBA( handle, recv->buf, STEAM_MAX_AVATAR_SIZE );
		fail_avatar_image:
			const uint32_t size = ( recv->width * recv->height * 4 ) + sizeof( steam_avatar_recv_s );
			assert( size <= sizeof( buffer ) );
			writePipe( GPipeWrite, &size, sizeof( uint32_t ) );
			writePipe( GPipeWrite, buffer, size );
			break;
		}
		default:
			assert( 0 ); // unhandled packet
			break;
	}
}

static void processSteamServerDispatch()
{
	if( GRunServer ) {
		HSteamPipe steamPipe = SteamGameServer_GetHSteamPipe();

		// perform period actions
		SteamAPI_ManualDispatch_RunFrame( steamPipe );

		// poll for callbacks
		CallbackMsg_t callback;
		while( SteamAPI_ManualDispatch_GetNextCallback( steamPipe, &callback ) ) {
			int id = callback.m_iCallback;
			void *data = callback.m_pubParam;
			void *callResultData = 0;
			steam_rpc_shim_common_s rpc_callback;

			if( callback.m_iCallback == SteamAPICallCompleted_t::k_iCallback ) {
				SteamAPICallCompleted_t *callCompleted = (SteamAPICallCompleted_t *)callback.m_pubParam;
				void *callResultData = malloc( callCompleted->m_cubParam );
				bool failed;
				if( SteamAPI_ManualDispatch_GetAPICallResult( steamPipe, callCompleted->m_hAsyncCall, callResultData, callCompleted->m_cubParam, callCompleted->m_iCallback, &failed ) ) {
					if( !steam_async_pop_rpc_shim( callCompleted->m_hAsyncCall, &rpc_callback ) ) {
						free( callResultData );
						SteamAPI_ManualDispatch_FreeLastCallback( steamPipe );
						continue;
					}

					id = callCompleted->m_iCallback;
					data = callResultData;
					// dmLogInfo( "SteamAPICallCompleted_t %llu result %d", callCompleted->m_hAsyncCall, id );
				} else {
					free( callResultData );
					SteamAPI_ManualDispatch_FreeLastCallback( steamPipe );
					// dmLogInfo("SteamAPICallCompleted_t failed to get call result");
					return;
				}
			}

			switch( id ) {
				case SteamServersConnected_t::k_iCallback: {
					printf( "SteamServersConnected_t\n" );
					break;
				}
				case SteamNetConnectionStatusChangedCallback_t::k_iCallback:
					handleSteamConnectionStatusChanged( EVT_SRV_P2P_CONNECTION_CHANGED, (SteamNetConnectionStatusChangedCallback_t *)data );
					break;
				case GSPolicyResponse_t::k_iCallback: {
					GSPolicyResponse_t *pCallback = (GSPolicyResponse_t *)data;
					printf( "VAC enabled: %d\n", pCallback->m_bSecure );
					uint64_t steamID = SteamGameServer()->GetSteamID().ConvertToUint64();
					policy_response_evt_s evt;
					evt.cmd = EVT_SRV_P2P_POLICY_RESPONSE;
					evt.steamID = steamID;
					evt.secure = pCallback->m_bSecure;
					write_packet( GPipeWrite, &evt, sizeof( struct policy_response_evt_s ) );
					break;
				}
			}
			free( callResultData );
			SteamAPI_ManualDispatch_FreeLastCallback( steamPipe );
		}
	}
}

static void processSteamDispatch()
{
	if( GRunClient ) {
		// use a manual callback loop
		// see steam_api.h for details
		HSteamPipe steamPipe = SteamAPI_GetHSteamPipe();

		// perform period actions
		SteamAPI_ManualDispatch_RunFrame( steamPipe );

		// poll for callbacks
		CallbackMsg_t callback;
		while( SteamAPI_ManualDispatch_GetNextCallback( steamPipe, &callback ) ) {
			int id = callback.m_iCallback;
			void *data = callback.m_pubParam;
			void *call_result_data = 0;
			steam_rpc_shim_common_s rpc_callback;

			if( callback.m_iCallback == SteamAPICallCompleted_t::k_iCallback ) {
				SteamAPICallCompleted_t *callCompleted = (SteamAPICallCompleted_t *)callback.m_pubParam;
				void *callResultData = malloc( callCompleted->m_cubParam );
				bool failed;
				if( SteamAPI_ManualDispatch_GetAPICallResult( steamPipe, callCompleted->m_hAsyncCall, callResultData, callCompleted->m_cubParam, callCompleted->m_iCallback, &failed ) ) {
					if( !steam_async_pop_rpc_shim( callCompleted->m_hAsyncCall, &rpc_callback ) ) {
						free( callResultData );
						SteamAPI_ManualDispatch_FreeLastCallback( steamPipe );
						continue;
					}
					id = callCompleted->m_iCallback;
					data = callResultData;
					// dmLogInfo( "SteamAPICallCompleted_t %llu result %d", callCompleted->m_hAsyncCall, id );
				} else {
					free( callResultData );
					SteamAPI_ManualDispatch_FreeLastCallback( steamPipe );
					// dmLogInfo("SteamAPICallCompleted_t failed to get call result");
					return;
				}
			}

			switch( id ) {
				case UserStatsReceived_t::k_iCallback: {
					break;
				}
				case SteamNetConnectionStatusChangedCallback_t::k_iCallback: {
					handleSteamConnectionStatusChanged( EVT_P2P_CONNECTION_CHANGED, (SteamNetConnectionStatusChangedCallback_t *)data );
					break;
				}
				case SubmitItemUpdateResult_t::k_iCallback: {
					SubmitItemUpdateResult_t *result = (SubmitItemUpdateResult_t *)data;
					struct submit_item_result_recv_s recv;
					recv.file_id = result->m_nPublishedFileId;
					recv.result = result->m_eResult;
					prepared_rpc_packet( &rpc_callback, &recv );
					write_packet( GPipeWrite, &recv, sizeof( struct submit_item_result_recv_s ) );
					break;
				}
				case CreateItemResult_t::k_iCallback: {
					CreateItemResult_t *result = (CreateItemResult_t *)data;
					struct create_item_recv_s recv;
					recv.file_id = result->m_nPublishedFileId;
					recv.result = result->m_eResult;
					prepared_rpc_packet( &rpc_callback, &recv );
					write_packet( GPipeWrite, &recv, sizeof( struct create_item_recv_s ) );
					break;
				}
				case SteamUGCQueryCompleted_t::k_iCallback: {
					SteamUGCQueryCompleted_t *result = (SteamUGCQueryCompleted_t *)data;
					dbgprintf( "SteamUGCQueryCompleted handle=%" PRIu64 " result=%d num_results=%u\n",
						(uint64_t)result->m_handle, (int)result->m_eResult, (unsigned)result->m_unNumResultsReturned );

					if( result->m_eResult == k_EResultOK ) {
						SteamUGCDetails_t details = {};
						char preview_url[4096];
						for( uint32_t i = 0; i < result->m_unNumResultsReturned; i++ ) {
							const char *title = "";
							const char *description = "";
							const char *tags = "";
							const char *preview = "";

							alignas( workshop_item_details_evt_s ) char recv_storage[sizeof( workshop_item_details_evt_s )] = {};
							workshop_item_details_evt_s &recv = *reinterpret_cast<workshop_item_details_evt_s *>( recv_storage );
							recv.cmd = EVT_WORKSHOP_DETAIL;
							recv.result = result->m_eResult;

							if( GSteamUGC->GetQueryUGCResult( result->m_handle, i, &details ) ) {
								preview_url[0] = '\0';
								GSteamUGC->GetQueryUGCPreviewURL( result->m_handle, i, preview_url, sizeof( preview_url ) );
								title = details.m_rgchTitle;
								description = details.m_rgchDescription;
								tags = details.m_rgchTags;
								preview = preview_url;

								recv.workshop_id = details.m_nPublishedFileId;
								recv.success = true;
								recv.owner_id = details.m_ulSteamIDOwner;
								recv.creator_app_id = details.m_nCreatorAppID;
								recv.consumer_app_id = details.m_nConsumerAppID;
								recv.file_type = details.m_eFileType;
								recv.visibility = details.m_eVisibility;
								recv.time_created = details.m_rtimeCreated;
								recv.time_updated = details.m_rtimeUpdated;
								recv.votes_up = details.m_unVotesUp;
								recv.votes_down = details.m_unVotesDown;
								recv.num_children = details.m_unNumChildren;
								recv.item_state = GSteamUGC->GetItemState( details.m_nPublishedFileId );
								recv.score = details.m_flScore;
								dbgprintf( "  -> [%u] workshop_id=%" PRIu64 " title=\"%s\" tags=\"%s\" preview=\"%s\"\n",
									i, (uint64_t)recv.workshop_id, title, tags, preview );
							}

							recv.title_len = strlen( title );
							recv.description_len = strlen( description );
							recv.tags_len = strlen( tags );
							recv.preview_url_len = strlen( preview );

							const uint32_t buffer_size = sizeof( recv ) + recv.title_len + 1 + recv.description_len + 1 + recv.tags_len + 1 + recv.preview_url_len + 1;
							std::vector<char> pkt( buffer_size );
							char *cursor = pkt.data();
							memcpy( cursor, &recv, sizeof( recv ) ); cursor += sizeof( recv );
							memcpy( cursor, title, recv.title_len + 1 ); cursor += recv.title_len + 1;
							memcpy( cursor, description, recv.description_len + 1 ); cursor += recv.description_len + 1;
							memcpy( cursor, tags, recv.tags_len + 1 ); cursor += recv.tags_len + 1;
							memcpy( cursor, preview, recv.preview_url_len + 1 );
							write_packet( GPipeWrite, pkt.data(), buffer_size );
						}
					}

					GSteamUGC->ReleaseQueryUGCRequest( result->m_handle );

					struct success_recv_s recv;
					prepared_rpc_packet( &rpc_callback, &recv );
					write_packet( GPipeWrite, &recv, sizeof( struct success_recv_s ) );
					break;
				}
				case GameRichPresenceJoinRequested_t::k_iCallback: {
					GameRichPresenceJoinRequested_t *pCallback = (GameRichPresenceJoinRequested_t *)data;
					join_request_evt_s evt;
					evt.cmd = EVT_GAME_JOIN;
					evt.steamID = pCallback->m_steamIDFriend.ConvertToUint64();
					memcpy( evt.rgchConnect, pCallback->m_rgchConnect, k_cchMaxRichPresenceValueLength );
					write_packet( GPipeWrite, &evt, sizeof( struct join_request_evt_s ) );
					break;
				}
				case PersonaStateChange_t::k_iCallback: {
					PersonaStateChange_t *pCallback = (PersonaStateChange_t *)data;
					persona_changes_evt_s evt;
					evt.cmd = EVT_PERSONA_CHANGED;
					evt.avatar_changed = pCallback->m_nChangeFlags & k_EPersonaChangeAvatar;
					evt.steamID = pCallback->m_ulSteamID;
					write_packet( GPipeWrite, &evt, sizeof( struct persona_changes_evt_s ) );
					break;
				}
				case RemoteStoragePublishedFileSubscribed_t::k_iCallback: {
					RemoteStoragePublishedFileSubscribed_t *pCallback = (RemoteStoragePublishedFileSubscribed_t *)data;
					struct workshop_item_evt_s evt;
					evt.cmd = EVT_WORKSHOP_ITEM_SUBSCRIBED;
					evt.app_id = pCallback->m_nAppID;
					evt.workshop_id = pCallback->m_nPublishedFileId;
					write_packet( GPipeWrite, &evt, sizeof( workshop_item_evt_s ) );
					break;
				}
				case RemoteStoragePublishedFileUnsubscribed_t::k_iCallback: {
					RemoteStoragePublishedFileUnsubscribed_t *pCallback = (RemoteStoragePublishedFileUnsubscribed_t *)data;
					struct workshop_item_evt_s evt;
					evt.cmd = EVT_WORKSHOP_ITEM_UNSUBSCRIBED;
					evt.app_id = pCallback->m_nAppID;
					evt.workshop_id = pCallback->m_nPublishedFileId;
					write_packet( GPipeWrite, &evt, sizeof( workshop_item_evt_s ) );
					break;
				}
				case ItemInstalled_t::k_iCallback: {
					ItemInstalled_t *pCallback = (ItemInstalled_t *)data;
					struct workshop_item_evt_s evt;
					evt.cmd = EVT_WORKSHOP_ITEM_INSTALLED;
					evt.app_id = pCallback->m_unAppID;
					evt.workshop_id = pCallback->m_nPublishedFileId;
					write_packet( GPipeWrite, &evt, sizeof( workshop_item_evt_s ) );
					break;
				}
			}
			free( call_result_data );
			SteamAPI_ManualDispatch_FreeLastCallback( steamPipe );
		}
	}
}

static void processCommands()
{
	static std::vector<uint8_t> packet_buf;
	static size_t cursor = 0;
	while( 1 ) {
		if( time_since_last_pump != 0 ) {
			time_t delta = time( NULL ) - time_since_last_pump;
			if( delta > 5 ) // we haven't gotten a pump in 5 seconds, safe to assume the main process is either dead or unresponsive and we can terminate
				return;
		}

		if( !pipeReady( GPipeRead ) ) {
			std::this_thread::sleep_for( std::chrono::microseconds( 1000 ) );
			continue;
		}

		if( packet_buf.empty() ) {
			packet_buf.resize( STEAM_PACKED_RESERVE_SIZE );
		}

		const int bytesRead = readPipe( GPipeRead, packet_buf.data() + cursor, packet_buf.size() - cursor );
		if( bytesRead > 0 ) {
			cursor += bytesRead;
		} else {
			std::this_thread::sleep_for( std::chrono::microseconds( 1000 ) );
			continue;
		}

		processSteamServerDispatch();
		processSteamDispatch();
	continue_processing:

		if( cursor < sizeof( uint32_t ) ) {
			continue;
		}

		struct steam_packet_buf *packet = reinterpret_cast<struct steam_packet_buf *>( packet_buf.data() );
		const size_t packetlen = packet->size + sizeof( uint32_t );
		if( packetlen < sizeof( uint32_t ) ) {
			return;
		}

		if( packetlen > packet_buf.size() ) {
			packet_buf.resize( packetlen );
			packet = reinterpret_cast<struct steam_packet_buf *>( packet_buf.data() );
		}

		if( cursor < packetlen ) {
			continue;
		}

		if( packet->common.cmd >= RPC_BEGIN && packet->common.cmd < RPC_END ) {
			processRPC( &packet->rpc_payload, packet->size );
		} else if( packet->common.cmd >= EVT_BEGIN && packet->common.cmd < EVT_END ) {
			processEVT( &packet->evt_payload, packet->size );
		}

		if( cursor > packetlen ) {
			const size_t remainingLen = cursor - packetlen;
			memmove( packet_buf.data(), packet_buf.data() + packetlen, remainingLen );
			cursor = remainingLen;
			goto continue_processing;
		} else {
			cursor = 0;
		}
	}
}

static bool initSteamworks( PipeType fd )
{
	if( GRunClient ) {
		// this can fail for many reasons:
		//  - you forgot a steam_appid.txt in the current working directory.
		//  - you don't have Steam running
		//  - you don't own the game listed in steam_appid.txt
		if( !SteamAPI_Init() )
			return 0;

		GSteamStats = SteamUserStats();
		GSteamUtils = SteamUtils();
		GSteamUser = SteamUser();
		GSteamUGC = SteamUGC();

		GAppID = GSteamUtils ? GSteamUtils->GetAppID() : 0;
		GUserID = GSteamUser ? GSteamUser->GetSteamID().ConvertToUint64() : 0;

		GServerBrowser = new ServerBrowser();
		GServerBrowser->RefreshInternetServers();

		SteamNetworkingUtils()->InitRelayNetworkAccess();
		SteamUser()->StopVoiceRecording();
	}

	if( GRunServer ) {
		// AFAIK you have to do a full server init if you want to use any of the steam utils, even though we never use this port
		if( !SteamGameServer_Init( 0, 0, 0, eServerModeAuthenticationAndSecure, "1.0.0.0" ) )
			printf( "port in use, ignoring\n" );

		GSteamGameServer = SteamGameServer();
		if( !GSteamGameServer )
			return 0;

		// GSLT can be used but seems to do nothing currently?
		// SteamGameServer()->LogOnAnonymous();

		GSteamGameServer->SetAdvertiseServerActive( true );
	}

	SteamAPI_ManualDispatch_Init();

	return 1;
}
static void deinitSteamworks()
{
	if( GRunServer ) {
		SteamGameServer_Shutdown();
	}

	if( GRunClient ) {
		SteamAPI_Shutdown();
	}
}

static int initPipes( void )
{
	char buf[64];
	unsigned long long val;

	if( !getEnvVar( "STEAMSHIM_READHANDLE", buf, sizeof( buf ) ) )
		return 0;
	else if( sscanf( buf, "%llu", &val ) != 1 )
		return 0;
	else
		GPipeRead = (PipeType)val;

	if( !getEnvVar( "STEAMSHIM_WRITEHANDLE", buf, sizeof( buf ) ) )
		return 0;
	else if( sscanf( buf, "%llu", &val ) != 1 )
		return 0;
	else
		GPipeWrite = (PipeType)val;

	return ( ( GPipeRead != NULLPIPE ) && ( GPipeWrite != NULLPIPE ) );
} /* initPipes */

int main( int argc, char **argv )
{
#ifndef _WIN32
	signal( SIGPIPE, SIG_IGN );

	if( argc > 1 && !strcmp( argv[1], "steamdebug" ) ) {
		debug = true;
	}
#endif

	dbgprintf( "Child starting mainline.\n" );

	char buf[64];
	if( !initPipes() )
		fail( "Child init failed.\n" );

	if( getEnvVar( "STEAMSHIM_RUNCLIENT", buf, sizeof( buf ) ) )
		GRunClient = true;
	if( getEnvVar( "STEAMSHIM_RUNSERVER", buf, sizeof( buf ) ) )
		GRunServer = true;

	if( !initSteamworks( GPipeWrite ) ) {
		char failure = 0;
		writePipe( GPipeWrite, &failure, sizeof failure );
		fail( "Failed to initialize Steamworks" );
	}
	char success = 1;
	writePipe( GPipeWrite, &success, sizeof success );

	dbgprintf( "Child in command processing loop.\n" );

	// Now, we block for instructions until the pipe fails (child closed it or
	//  terminated/crashed).
	processCommands();

	dbgprintf( "Child shutting down.\n" );

	// Close our ends of the pipes.
	closePipe( GPipeRead );
	closePipe( GPipeWrite );

	deinitSteamworks();

	return 0;
}

#ifdef _WIN32

// from win_sys
#define MAX_NUM_ARGVS 128
int argc;
char *argv[MAX_NUM_ARGVS];

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	if( strstr( GetCommandLineA(), "steamdebug" ) ) {
		debug = true;
		FreeConsole();
		AllocConsole();
		freopen( "CONOUT$", "w", stdout );
	}
	return main( argc, argv );
} // WinMain
#endif
