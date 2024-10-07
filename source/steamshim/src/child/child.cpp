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
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stdlib.h>
#include <thread>
#include <stdlib.h>

#include "../os.h"
#include "../steamshim.h"
#include "../steamshim_private.h"
#include "ServerBrowser.h"
#include "child_private.h"
#include "steam/isteamfriends.h"
#include "steam/isteamgameserver.h"
#include "steam/isteammatchmaking.h"
#include "steam/isteamnetworking.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/isteamnetworkingutils.h"
#include "steam/isteamuser.h"
#include "steam/isteamutils.h"
#include "steam/steam_api.h"
#include "steam/steam_api_common.h"
#include "steam/steam_gameserver.h"
#include "steam/steamclientpublic.h"

#include "../packet_utils.h"
#include "steam/steamnetworkingtypes.h"
#include "write_utils.h"


struct steam_rpc_pkt_s GCurrent_p2p_connect_request;
struct steam_rpc_pkt_s GCurrent_p2p_listen_request;

HSteamNetConnection GClientToServerConn = 0;
HSteamNetConnection GListenSocket = 0;

static bool GRunServer = false;
static bool GRunClient = false;

static ISteamUserStats *GSteamStats = NULL;
static ISteamUtils *GSteamUtils = NULL;
static ISteamUser *GSteamUser = NULL;
static AppId_t GAppID = 0;
static uint64 GUserID = 0;
static ISteamGameServer *GSteamGameServer = NULL;
ServerBrowser *GServerBrowser = NULL;
static time_t time_since_last_pump = 0;

static SteamCallbacks *GSteamCallbacks;

static void processRPC( steam_rpc_pkt_s *req, size_t size )
{
	switch( req->common.cmd ) {
		case RPC_PUMP: {
			time( &time_since_last_pump );
			if( GRunServer ) {
				SteamGameServer_RunCallbacks();
			}
			if( GRunClient )
				SteamAPI_RunCallbacks();
			struct steam_rpc_shim_common_s recv;
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
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
			break;
		}
		case RPC_UPDATE_SERVERINFO_GAME_DATA: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameData( (const char *)req->server_game_data.buf );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_GAME_DESCRIPTION: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameDescription( (const char *)req->server_description.buf );
			prepared_rpc_packet( &req->common, &recv );
			write_packet( GPipeWrite, &recv, sizeof( steam_rpc_shim_common_s ) );
			break;
		}
		case RPC_UPDATE_SERVERINFO_GAME_TAGS: {
			struct steam_rpc_shim_common_s recv;
			GSteamGameServer->SetGameDescription( (const char *)req->server_tags.buf );
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
			break;
		}
		case RPC_P2P_LISTEN: {
			ISteamGameServer *gameServer = SteamGameServer();
			ISteamNetworkingSockets *networkSocket = SteamGameServerNetworkingSockets();
			if( gameServer && networkSocket ) {
				gameServer->LogOnAnonymous();
				GListenSocket = networkSocket->CreateListenSocketP2P( 0, 0, nullptr );
				memcpy( &GCurrent_p2p_listen_request, req, sizeof( steam_rpc_pkt_s ) );
			} else {
				struct p2p_listen_recv_s recv;
				prepared_rpc_packet( &req->common, &recv );
				recv.success = false;
				recv.steamID = 0;
				write_packet( GPipeWrite, &recv, sizeof( p2p_listen_recv_s ) );
			}
			break;
		}
		case RPC_P2P_CLOSE_LISTEN: {
			SteamGameServerNetworkingSockets()->CloseListenSocket(GListenSocket); 
	 	}
		case RPC_P2P_CONNECT: {
			SteamNetworkingIdentity id;
			CSteamID steamID = CSteamID((uint64)req->p2p_connect.steamID);
			id.SetSteamID(steamID);
			GClientToServerConn = SteamNetworkingSockets()->ConnectP2P(id, 0, 0, nullptr);
			memcpy(&GCurrent_p2p_connect_request, req, sizeof(steam_rpc_pkt_s));
			break;
		}
		case RPC_P2P_DISCONNECT: {
			if (req->p2p_disconnect.handle == 0) {
				SteamNetworkingSockets()->CloseConnection(GClientToServerConn, 0, nullptr, false);
			} else {
				// connection close causes a hang inside of steamworks dll. haven't figured out why yet
				SteamGameServerNetworkingSockets()->CloseConnection(req->p2p_disconnect.handle, 0, nullptr, false);
			}
		 	break;
	 	}
		case RPC_P2P_SEND_MESSAGE: {
		 	 assert(req->send_message.count < SDR_MAX_MESSAGE_SIZE);
			if (req->send_message.handle == 0) {
				// 0 is the c2s connection, and we must be on the client
				SteamNetworkingSockets()->SendMessageToConnection(GClientToServerConn, req->send_message.buffer, req->send_message.count, req->send_message.messageReliability, nullptr);
				break;
			}

			// .. otherwise we must be on the server, sending to a client
			SteamGameServerNetworkingSockets()->SendMessageToConnection(req->send_message.handle, req->send_message.buffer, req->send_message.count, req->send_message.messageReliability, nullptr);
			break;
		}
		case RPC_P2P_RECV_MESSAGES: {
			SteamNetworkingMessage_t* msgs[32];
			int n;

			uint64_t steamid;
			if (req->recv_messages.handle == 0) {
				SteamNetConnectionInfo_t info;
				SteamNetworkingSockets()->GetConnectionInfo(GClientToServerConn, &info);
				steamid = info.m_identityRemote.GetSteamID().ConvertToUint64();

				n = SteamNetworkingSockets()->ReceiveMessagesOnConnection(GClientToServerConn, msgs, 1);
			} else {
				SteamNetConnectionInfo_t info;
				SteamGameServerNetworkingSockets()->GetConnectionInfo(req->recv_messages.handle, &info);
				steamid = info.m_identityRemote.GetSteamID().ConvertToUint64();

				n = SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(req->recv_messages.handle, msgs, 1);
			}

			size_t total = 0;
			for (int i = 0; i < n; i++) {
				total += msgs[i]->GetSize();
			}

			auto recv = (struct recv_messages_recv_s *)malloc(sizeof(struct recv_messages_recv_s) + total);
			recv->steamID = steamid;
			recv->handle = req->recv_messages.handle;
			recv->count = n;

			size_t offset = 0;

			for (int i = 0; i < n; i++) {
				recv->messageinfo[i].count = msgs[i]->GetSize();
				memcpy(recv->buffer + offset, msgs[i]->GetData(), msgs[i]->GetSize());
				offset += msgs[i]->GetSize();
			}


			prepared_rpc_packet(&req->common, recv);
			write_packet(GPipeWrite, recv, sizeof(struct recv_messages_recv_s) + total);
			break;
		}
		case RPC_GETVOICE: {
			uint32 size;
			SteamUser()->GetAvailableVoice(&size);
			if (size == 0) {
				break;
			}

			uint8_t buffer[1024];

			SteamUser()->GetVoice(true, buffer, sizeof buffer, &size);

			auto recv = (struct getvoice_recv_s *)malloc(sizeof(struct getvoice_recv_s) + size);
			recv->count = size;
			memcpy(recv->buffer, buffer, size);

			prepared_rpc_packet(&req->common, recv);
			write_packet(GPipeWrite, recv, sizeof(struct getvoice_recv_s) + size);

			break;
	 	}
	 	case RPC_DECOMPRESS_VOICE: {
			static uint8 decompressed[ VOICE_BUFFER_MAX ];

	 	 	auto recv = (struct decompress_voice_recv_s *)malloc(sizeof(struct decompress_voice_recv_s) + sizeof(decompressed));

	 	 	uint32 size = 0;
	 	 	uint32 optimalSampleRate = 11025;
	 		EVoiceResult e = SteamUser()->DecompressVoice(req->decompress_voice.buffer, req->decompress_voice.count, decompressed, sizeof decompressed, &size, optimalSampleRate);
	 		if (e != 0) printf("DecompressVoice failed: %d\n", e);
	 		memcpy(recv->buffer, decompressed, size);
	 		recv->count = size;

	 		prepared_rpc_packet(&req->common, recv);
	 		write_packet(GPipeWrite, recv, sizeof(struct decompress_voice_recv_s) + size);
	 	 	break;
	 	}
	 	case RPC_START_VOICE_RECORDING: {
			SteamUser()->StartVoiceRecording();
			SteamFriends()->SetInGameVoiceSpeaking(SteamUser()->GetSteamID(), true);
			break;
	 	}
	 	case RPC_STOP_VOICE_RECORDING: {
			SteamUser()->StopVoiceRecording();
			SteamFriends()->SetInGameVoiceSpeaking(SteamUser()->GetSteamID(), false);
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
			break;
	}
}
static void processCommands()
{
	static struct steam_packet_buf packet;
	static size_t cursor = 0;
	bool hasMore = false;
	while( 1 ) {
		if( time_since_last_pump != 0 ) {
			time_t delta = time( NULL ) - time_since_last_pump;
			if( delta > 5 ) // we haven't gotten a pump in 5 seconds, safe to assume the main process is either dead or unresponsive and we can terminate
				return;
		}

		assert( sizeof( struct steam_packet_buf ) == STEAM_PACKED_RESERVE_SIZE );

		const int bytesRead = readPipe( GPipeRead, packet.buffer + cursor, STEAM_PACKED_RESERVE_SIZE - cursor );
		if( bytesRead > 0 ) {
			cursor += bytesRead;
		} else {
			std::this_thread::sleep_for( std::chrono::microseconds( 1000 ) );
			continue;
		}
	continue_processing:

		if( packet.size > STEAM_PACKED_RESERVE_SIZE - sizeof( uint32_t ) ) {
			// the packet is larger then the reserved size
			return;
		}

		if( cursor < packet.size + sizeof( uint32_t ) ) {
			continue;
		}

		// readPipe(GPipeRead, &packet, size );
		if( packet.common.cmd >= RPC_BEGIN && packet.common.cmd < RPC_END ) {
			// dbgprintf( "process packet: %lu %lu\n", packet.rpc_payload.common.cmd, packet.rpc_payload.common.sync );
			processRPC( &packet.rpc_payload, packet.size );
		}

		if( cursor > packet.size + sizeof( uint32_t ) ) {
			const size_t packetlen = packet.size + sizeof( uint32_t );
			const size_t remainingLen = cursor - packetlen;
			memmove( packet.buffer, packet.buffer + packet.size + sizeof( uint32_t ), remainingLen );
			cursor = remainingLen;
			goto continue_processing;
		} else {
			cursor = 0;
		}
	}
}

void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {
	printf("SteamNetConnectionStatusChangedCallback_t\n");
}

HSteamNetConnection gconn = k_HSteamNetConnection_Invalid;
static bool initSteamworks( PipeType fd )
{
	GSteamCallbacks = new SteamCallbacks();

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

		GAppID = GSteamUtils ? GSteamUtils->GetAppID() : 0;
		GUserID = GSteamUser ? GSteamUser->GetSteamID().ConvertToUint64() : 0;

		GServerBrowser = new ServerBrowser();

		GServerBrowser->RefreshInternetServers();

		SteamNetworkingUtils()->InitRelayNetworkAccess();
	}

	if( GRunServer ) {

		// AFAIK you have to do a full server init if you want to use any of the steam utils, even though we never use this port
		if( !SteamGameServer_Init( 0, 0, 0, eServerModeAuthenticationAndSecure, "1.0.0.0" ) )
			printf("port in use, ignoring\n");

		GSteamGameServer = SteamGameServer();
		if( !GSteamGameServer )
			return 0;

		// GSLT can be used but seems to do nothing currently?
		// SteamGameServer()->LogOnAnonymous();

		GSteamGameServer->SetAdvertiseServerActive( true );
	}

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
	// if (!gcansend) continue;
	// 		SteamNetworkingMessage_t* msgs[32];
	// 		int n = SteamNetworkingSockets()->ReceiveMessagesOnConnection(gconn, msgs, 32);
	//
	// 		for (int i = 0; i < n; i++) {
	// 			uint32 size = msgs[i]->GetSize();
	// 			printf("Received message %i\n", size);
	// 			uint8* data = new uint8[size];
	// 			memcpy(data, msgs[i]->GetData(), size);
	//
	// 			printf("Data: %s\n", data);
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
