/*
Copyright (C) 2026 Warfork contributors

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
// libFuzzer target for the netchan receive path: fragment reassembly and zlib
// decompression, both of which run before any authentication.
//
// Fragments only reassemble across several packets, so the input is split into
// length-prefixed chunks fed through one netchan in sequence - a single-packet
// corpus would never reach the reassembly branch.
//
// Build with -fsanitize=fuzzer,address.
#include "../qcommon.h"
#include "net_test_stubs.h"

#include <stdint.h>
#include <string.h>

static socket_t fuzz_socket;
static netchan_t fuzz_chan;
static bool fuzz_initialized = false;

int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	uint8_t buf[MAX_MSGLEN];
	msg_t msg;
	netadr_t addr;
	size_t pos = 0;
	int packets = 0;

	if( size < 4 )
		return 0;

	// registers the showpackets / showdrop / net_showfragments cvars that
	// Netchan_Process dereferences unconditionally
	if( !fuzz_initialized )
	{
		Netchan_Init();
		fuzz_initialized = true;
	}

	memset( &fuzz_socket, 0, sizeof( fuzz_socket ) );
	memset( &addr, 0, sizeof( addr ) );
	addr.type = NA_LOOPBACK;
	fuzz_socket.open = true;
	fuzz_socket.type = SOCKET_LOOPBACK;
	// the server variant reads an extra game_port short from the header
	fuzz_socket.server = ( data[0] & 1 ) ? true : false;
	pos = 1;

	Netchan_Setup( &fuzz_chan, &fuzz_socket, &addr, 0 );

	if( !NET_TEST_TRY() )
		return 0;

	// feed a sequence of packets so multi-fragment messages can actually assemble
	while( pos + 2 <= size && packets < 64 )
	{
		size_t chunk = ( (size_t)data[pos] << 8 ) | data[pos + 1];
		pos += 2;

		if( chunk > size - pos )
			chunk = size - pos;
		if( chunk > sizeof( buf ) )
			chunk = sizeof( buf );
		if( chunk == 0 )
			break;

		memcpy( buf, data + pos, chunk );
		pos += chunk;
		packets++;

		MSG_Init( &msg, buf, sizeof( buf ) );
		msg.cursize = chunk;
		MSG_BeginReading( &msg );

		if( Netchan_Process( &fuzz_chan, &msg ) )
			Netchan_DecompressMessage( &msg );
	}

	NET_TEST_OK();
	return 0;
}
