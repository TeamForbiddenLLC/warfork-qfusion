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

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdio.h>
#include <thread>
#include <vector>
#define DEBUGPIPE 1
#include "../os.h"
#include "../steamshim.h"
#include "../steamshim_private.h"
#include "../steamshim_types.h"
#include "parent.h"

#include "../mod_steam.h"

#include "tracy/TracyC.h"


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

static std::atomic<size_t> SyncToken = 1;
static size_t currentSync;
static struct steam_rpc_async_s rpc_handles[NUM_RPC_ASYNC_HANDLE];
static struct event_subscriber_s evt_handles[STEAM_EVT_LEN];

// Accumulation buffer shared by STEAMSHIM_dispatch (non-blocking drain) and
// STEAMSHIM_waitDispatchSync (blocking wait). Grows to fit oversized packets.
// Only the main thread reads from GPipeRead, so no synchronization is needed.
static std::vector<uint8_t> dispatchBuf;
static size_t dispatchCursor = 0;

std::mutex writeGuard;

static int processBufferedPackets( uint32_t waitSync )
{
	while( 1 ) {
		// Drain every complete packet currently in the buffer.
		while( dispatchCursor >= sizeof( uint32_t ) ) {
			struct steam_packet_buf *packet = reinterpret_cast<struct steam_packet_buf *>( dispatchBuf.data() );
			const size_t packetlen = (size_t)packet->size + sizeof( uint32_t );
			if( packetlen < sizeof( uint32_t ) ) {
				// overflow guard — reset and bail
				dispatchCursor = 0;
				return -1;
			}
			if( packetlen > dispatchBuf.size() ) {
				dispatchBuf.resize( packetlen );
				packet = reinterpret_cast<struct steam_packet_buf *>( dispatchBuf.data() );
			}
			if( dispatchCursor < packetlen ) {
				break; // body still arriving — fall through to read more
			}

			const bool rpcPacket = packet->common.cmd >= RPC_BEGIN && packet->common.cmd < RPC_END;
			if( rpcPacket ) {
				struct steam_rpc_async_s *handle = rpc_handles + ( packet->rpc_payload.common.sync % NUM_RPC_ASYNC_HANDLE );
				if( handle->cb ) {
					handle->cb( handle->self, &packet->rpc_payload );
				}
				currentSync = packet->rpc_payload.common.sync;
			} else if( packet->common.cmd >= EVT_BEGIN && packet->common.cmd < EVT_END ) {
				struct event_subscriber_s *handle = evt_handles + ( packet->common.cmd - EVT_BEGIN );
				for( size_t i = 0; i < handle->numSubscribers; i++ ) {
					handle->handles[i].cb( handle->handles[i].self, &packet->evt_payload );
				}
			}

			if( dispatchCursor > packetlen ) {
				const size_t remainingLen = dispatchCursor - packetlen;
				memmove( dispatchBuf.data(), dispatchBuf.data() + packetlen, remainingLen );
				dispatchCursor = remainingLen;
			} else {
				dispatchCursor = 0;
			}

			if( waitSync != WAIT_SYNC_NONBLOCKING && currentSync == waitSync ) {
				return 0;
			}
		}

		// Buffer fully decoded; need more bytes from the pipe.
		if( waitSync == WAIT_SYNC_NONBLOCKING && !pipeReady( GPipeRead ) ) {
			return 0;
		}
		if( dispatchBuf.empty() ) {
			dispatchBuf.resize( STEAM_PACKED_RESERVE_SIZE );
		}
		int bytesRead = readPipe( GPipeRead, dispatchBuf.data() + dispatchCursor, dispatchBuf.size() - dispatchCursor );
		if( bytesRead <= 0 ) {
			return -1;
		}
		dispatchCursor += bytesRead;
	}
}

int STEAMSHIM_dispatch()
{
	TracyCZoneN( ctx, "STEAMSHIM_dispatch", 1 );
	int result = processBufferedPackets( WAIT_SYNC_NONBLOCKING );
	TracyCZoneEnd( ctx );
	return result;
}

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

void taskHeartbeat()
{
	while (STEAMSHIM_active()) {
		struct steam_shim_common_s pkt;
		pkt.cmd = EVT_HEART_BEAT;
		STEAMSHIM_sendEVT( &pkt, sizeof( struct steam_shim_common_s ));
		std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
	}
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

	dispatchBuf.resize( STEAM_PACKED_RESERVE_SIZE );
	dispatchCursor = 0;

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

	std::thread task( taskHeartbeat );
	task.detach();

#ifndef _WIN32
	signal( SIGPIPE, SIG_IGN );
#endif

	dbgprintf( "Child init success!\n" );
	return 1;
}

void STEAMSHIM_deinit( void )
{
	dbgprintf( "Child deinit.\n" );
	if( GPipeWrite != NULLPIPE )
		closePipe( GPipeWrite );

	if( GPipeRead != NULLPIPE )
		closePipe( GPipeRead );

	GPipeRead = GPipeWrite = NULLPIPE;

	dispatchBuf.clear();
	dispatchCursor = 0;

#ifndef _WIN32
	signal( SIGPIPE, SIG_DFL );
#endif
}

bool STEAMSHIM_active()
{
	TracyCZoneN( ctx, "STEAMSHIM_active", 1 );
	bool result = ( ( GPipeRead != NULLPIPE ) && ( GPipeWrite != NULLPIPE ) );
	TracyCZoneEnd( ctx );
	return result;
}

int STEAMSHIM_sendEVT( void *packet, uint32_t size )
{
	TracyCZoneN( ctx, "STEAMSHIM_sendEVT", 1 );
	writeGuard.lock();
	writePipe( GPipeWrite, &size, sizeof( uint32_t ) );
	writePipe( GPipeWrite, (uint8_t *)packet, size );
	writeGuard.unlock();
	TracyCZoneEnd( ctx );
	return 0;
}

int STEAMSHIM_sendRPC( void *packet, uint32_t size, void *self, STEAMSHIM_rpc_handle rpc, uint32_t *sync )
{
	TracyCZoneN( ctx, "STEAMSHIM_sendRPC", 1 );
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

	writeGuard.lock();
	writePipe( GPipeWrite, &size, sizeof( uint32_t ) );
	writePipe( GPipeWrite, (uint8_t *)packet, size );
	writeGuard.unlock();
	TracyCZoneEnd( ctx );
	return 0;
}

int STEAMSHIM_waitDispatchSync( uint32_t syncIndex )
{
	TracyCZoneN( ctx, "STEAMSHIM_waitDispatchSync", 1 );
	if( currentSync == syncIndex ) {
		TracyCZoneEnd( ctx );
		return 0; // can't wait on dispatch if there is no RPC's staged
	}
	int result = processBufferedPackets( syncIndex );
	TracyCZoneEnd( ctx );
	return result;
}
uint32_t STEAMSHIM_currentSync()
{
	return (uint32_t)currentSync;
}
void STEAMSHIM_subscribeEvent( uint32_t id, void *self, STEAMSHIM_evt_handle evt )
{
	TracyCZoneN( ctx, "STEAMSHIM_subscribeEvent", 1 );
	assert( evt );
	assert( id >= EVT_BEGIN && id < EVT_END );
	struct event_subscriber_s *handle = evt_handles + ( id - EVT_BEGIN );
	assert( handle->numSubscribers < NUM_EVT_HANDLE );
	size_t subIndex = handle->numSubscribers++;
	handle->handles[subIndex].self = self;
	handle->handles[subIndex].cb = evt;
	TracyCZoneEnd( ctx );
}
void STEAMSHIM_unsubscribeEvent( uint32_t id, STEAMSHIM_evt_handle cb )
{
	TracyCZoneN( ctx, "STEAMSHIM_unsubscribeEvent", 1 );
	assert( id >= EVT_BEGIN && id < EVT_END );
	struct event_subscriber_s *handle = evt_handles + ( id - EVT_BEGIN );
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
	TracyCZoneEnd( ctx );
}
}
