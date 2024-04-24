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
#include <cstdint>
#include <cstring>
#include <stdio.h>
#define DEBUGPIPE 1
#include "../os.h"
#include "../steamshim.h"
#include "../steamshim_private.h"
#include "../steamshim_types.h"
#include "parent.h"

#include "../mod_steam.h"

int GArgc = 0;
char **GArgv = NULL;

#define NUM_RPC_ASYNC_HANDLE 2048
#define NUM_EVT_HANDLE 16

struct steam_rpc_async_s {
	uint32_t token;
	void *self;
	STEAMSHIM_rpc_handle cb;
};

struct event_subscriber_s {
	size_t numSubscribers;
    struct {
		void *self;
		STEAMSHIM_evt_handle cb;
	} handles[NUM_EVT_HANDLE];
};

static size_t SyncToken;
static size_t currentSync;
static struct steam_rpc_async_s rpc_handles[NUM_RPC_ASYNC_HANDLE];
static struct event_subscriber_s evt_handles[STEAM_EVT_LEN];

int STEAMSHIM_dispatch()
{
	// struct steam_packet_buf packet;
	uint32_t syncIndex = 0;
	// size_t cursor = 0;
	struct steam_rpc_shim_common_s pkt;
	pkt.cmd = RPC_PUMP;
	if( STEAMSHIM_sendRPC( &pkt, sizeof( steam_rpc_shim_common_s ), NULL, NULL, &syncIndex ) < 0 ) {
		return -1;
	}
	return STEAMSHIM_waitDispatchSync( syncIndex );
}

//static SteamshimEvent *ProcessEvent()
//{
//	static SteamshimEvent event;
//	// make sure this is static, since it needs to persist between pumps
//	static PipeBuffer buf;
//
//	if( !buf.Recieve() )
//		return NULL;
//
//	if( !buf.hasmsg )
//		return NULL;
//
//	volatile unsigned int msglen = buf.ReadInt();
//
//	char type = buf.ReadByte();
//	event.type = (SteamshimEventType)type;
//
//	switch( type ) {
//		case EVT_CL_STEAMIDRECIEVED: {
//			event.cl_steamidrecieved = buf.ReadLong();
//		} break;
//		case EVT_CL_PERSONANAMERECIEVED: {
//			char *string = buf.ReadString();
//			event.cl_personanamerecieved = string;
//		} break;
//		case EVT_CL_AUTHSESSIONTICKETRECIEVED: {
//			long long pcbTicket = buf.ReadLong();
//			event.cl_authsessionticketrecieved.pcbTicket = pcbTicket;
//			void *ticket = buf.ReadData( AUTH_TICKET_MAXSIZE );
//			memcpy( event.cl_authsessionticketrecieved.pTicket, ticket, AUTH_TICKET_MAXSIZE );
//		} break;
//		case EVT_SV_AUTHSESSIONVALIDATED: {
//			int result = buf.ReadInt();
//			event.sv_authsessionvalidated = result;
//		} break;
//		case EVT_CL_AVATARRECIEVED: {
//			event.cl_avatarrecieved.steamid = buf.ReadLong();
//			event.cl_avatarrecieved.avatar = (uint8_t *)buf.ReadData( STEAM_AVATAR_SIZE );
//		} break;
//		case EVT_CL_GAMEJOINREQUESTED: {
//			event.cl_gamejoinrequested.steamIDFriend = buf.ReadLong();
//			char *string = (char *)buf.ReadString();
//			event.cl_gamejoinrequested.connectString = string;
//		} break;
//		case EVT_CL_COMMANDLINERECIEVED: {
//			char *string = (char *)buf.ReadString();
//			event.cl_commandlinerecieved = string;
//		} break;
//	}
//
//	return &event;
//}

static bool setEnvironmentVars( PipeType pipeChildRead, PipeType pipeChildWrite )
{
	char buf[64];
	snprintf( buf, sizeof( buf ), "%llu", (unsigned long long)pipeChildRead );
	if( !setEnvVar( "STEAMSHIM_READHANDLE", buf ) )
		return false;

	snprintf( buf, sizeof( buf ), "%llu", (unsigned long long)pipeChildWrite );
	if( !setEnvVar( "STEAMSHIM_WRITEHANDLE", buf ) )
		return false;

	return true;
}

extern "C" {
int STEAMSHIM_init( SteamshimOptions *options )
{
	debug = options->debug;

	PipeType pipeParentRead = NULLPIPE;
	PipeType pipeParentWrite = NULLPIPE;
	PipeType pipeChildRead = NULLPIPE;
	PipeType pipeChildWrite = NULLPIPE;
	ProcessType childPid;

	if( options->runclient )
		setEnvVar( "STEAMSHIM_RUNCLIENT", "1" );
	if( options->runserver )
		setEnvVar( "STEAMSHIM_RUNSERVER", "1" );

	if( !createPipes( &pipeParentRead, &pipeParentWrite, &pipeChildRead, &pipeChildWrite ) ) {
		printf( "steamshim: Failed to create application pipes\n" );
		return 0;
	} else if( !setEnvironmentVars( pipeChildRead, pipeChildWrite ) ) {
		printf( "steamshim: Failed to set environment variables\n" );
		return 0;
	} else if( !launchChild( &childPid ) ) {
		printf( "steamshim: Failed to launch application\n" );
		return 0;
	}

	GPipeRead = pipeParentRead;
	GPipeWrite = pipeParentWrite;

	char status;

	readPipe( GPipeRead, &status, sizeof status );

	if( !status ) {
		closePipe( GPipeRead );
		closePipe( GPipeWrite );

		GPipeWrite = GPipeRead = pipeChildRead = pipeChildWrite = NULLPIPE;
		return 0;
	}

	dbgprintf( "Parent init start.\n" );

	// Close the ends of the pipes that the child will use; we don't need them.
	closePipe( pipeChildRead );
	closePipe( pipeChildWrite );

	pipeChildRead = pipeChildWrite = NULLPIPE;

#ifndef _WIN32
	signal( SIGPIPE, SIG_IGN );
#endif

	dbgprintf( "Child init success!\n" );
	return 1;
}

void STEAMSHIM_deinit( void )
{
	dbgprintf( "Child deinit.\n" );
	if( GPipeWrite != NULLPIPE ) {
		closePipe( GPipeWrite );
	}

	if( GPipeRead != NULLPIPE )
		closePipe( GPipeRead );

	GPipeRead = GPipeWrite = NULLPIPE;

#ifndef _WIN32
	signal( SIGPIPE, SIG_DFL );
#endif
}

bool STEAMSHIM_active() {
	return ( ( GPipeRead != NULLPIPE ) && ( GPipeWrite != NULLPIPE ) );
} 

int STEAMSHIM_sendRPC( void *packet, uint32_t size, void *self, STEAMSHIM_rpc_handle rpc, uint32_t *sync )
{
	uint32_t syncIndex = ++SyncToken;
	if( sync ) {
		( *sync ) = syncIndex;
	}
	struct steam_rpc_async_s *handle = rpc_handles + ( syncIndex % NUM_RPC_ASYNC_HANDLE );
	struct steam_rpc_shim_common_s *rpc_common = (steam_rpc_shim_common_s *)packet;
	rpc_common->sync = syncIndex;

	handle->token = syncIndex;
	handle->self = self;
	handle->cb = rpc;
	writePipe( GPipeWrite, &size, sizeof( uint32_t ) );
	writePipe( GPipeWrite, (uint8_t *)packet, size );
	return 0;
}

int STEAMSHIM_waitDispatchSync( uint32_t syncIndex )
{
    if(currentSync == syncIndex) {
        return 0; // can't wait on dispatch if there is no RPC's staged
    }
	static struct steam_packet_buf packet;
	static size_t cursor = 0;
	while( 1 ) {
		assert( sizeof( struct steam_packet_buf ) == STEAM_PACKED_RESERVE_SIZE );
		int bytesRead = readPipe( GPipeRead, packet.buffer + cursor, STEAM_PACKED_RESERVE_SIZE - cursor );
		if( bytesRead > 0 ) {
			cursor += bytesRead;
		} else {
			continue;
		}
		continue_processing:

		if( packet.size > STEAM_PACKED_RESERVE_SIZE - sizeof( uint32_t ) ) {
			// the packet is larger then the reserved size
			return -1;
		}
		
		if( cursor < packet.size + sizeof( uint32_t ) ) {
			continue;
		}
		const bool rpcPacket = packet.common.cmd >= RPC_BEGIN && packet.common.cmd < RPC_END;
		if( rpcPacket ) {
		    assert(packet.rpc_payload.common.sync > currentSync); // rpc's are FIFO no out of order 
	        
	        struct steam_rpc_async_s *handle = rpc_handles + ( packet.rpc_payload.common.sync % NUM_RPC_ASYNC_HANDLE );
	        if( handle->cb ) {
		        handle->cb( handle->self, &packet.rpc_payload );
	        }
		    currentSync = packet.rpc_payload.common.sync;
		} else if( packet.common.cmd >= EVT_BEGIN && packet.common.cmd < EVT_END ) {
			struct event_subscriber_s *handle = evt_handles + ( EVT_BEGIN - packet.common.cmd );
			for(size_t i = 0; i < handle->numSubscribers; i++) {
			    handle->handles[i].cb(handle->handles[i].self, &packet.evt_payload);
			}
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
		if( rpcPacket && currentSync == syncIndex ) {
			break;
		}
	}
	return 0;
}
void STEAMSHIM_subscribeEvent( uint32_t id, void *self, STEAMSHIM_evt_handle evt )
{
    assert(evt);
	assert( id >= EVT_BEGIN && id < EVT_END );
	struct event_subscriber_s *handle = evt_handles + ( EVT_BEGIN - id );
	assert( handle->numSubscribers < NUM_EVT_HANDLE );
	size_t subIndex = handle->numSubscribers++;
	handle->handles[subIndex].self = self;
	handle->handles[subIndex].cb = evt;
}
void STEAMSHIM_unsubscribeEvent( uint32_t id, STEAMSHIM_evt_handle cb )
{
	assert( id >= EVT_BEGIN && id < EVT_END );
	struct event_subscriber_s *handle = evt_handles + ( EVT_BEGIN - id );
	size_t ib = 0;
	size_t ic = 0;
	const size_t len = handle->numSubscribers;
	for( ; ic < len; ic++, ib++ ) {
		if( handle->handles[ic].cb == cb ) {
			handle->numSubscribers--;
			ib--;
		    continue;
		}
		if( ic == ib )
			continue;
		handle->handles[ib] = handle->handles[ic];
	}
}
}
