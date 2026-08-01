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
// libFuzzer target for snapshot parsing - the path a malicious server (or a
// malicious .wfdemo, which reaches the same code) drives on a connected client.
//
// snap_read.c links against msg.c plus the stubs alone, so this needs no client
// state. Build with -fsanitize=fuzzer,address.
#include "../qcommon.h"
#include "../snap_read.h"
#include "net_test_stubs.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// the real client sizes this from the loaded map; any fixed size works here, the
// point is that SNAP_ParseFrame must respect whatever it is told
#define FUZZ_AREABYTES 32

static entity_state_t fuzz_baselines[MAX_EDICTS];
static snapshot_t fuzz_backup[UPDATE_BACKUP];
static uint8_t fuzz_areabits[UPDATE_BACKUP][FUZZ_AREABYTES];

static void fuzz_reset( void )
{
	size_t i;

	memset( fuzz_baselines, 0, sizeof( fuzz_baselines ) );
	memset( fuzz_backup, 0, sizeof( fuzz_backup ) );
	memset( fuzz_areabits, 0, sizeof( fuzz_areabits ) );

	// SNAP_ParseFrame writes areabits into a per-map allocation supplied by the
	// client; mirror that here so the size check has something to check against
	for( i = 0; i < UPDATE_BACKUP; i++ )
	{
		fuzz_backup[i].areabits = fuzz_areabits[i];
		fuzz_backup[i].areabytes = sizeof( fuzz_areabits[i] );
	}
}

int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	uint8_t buf[MAX_MSGLEN];
	msg_t msg;
	uint8_t selector;

	if( size < 2 )
		return 0;

	selector = data[0];
	data++;
	size--;

	if( size > sizeof( buf ) )
		size = sizeof( buf );

	memcpy( buf, data, size );
	MSG_Init( &msg, buf, sizeof( buf ) );
	msg.cursize = size;
	MSG_BeginReading( &msg );

	fuzz_reset();

	// a rejected snapshot unwinds through Com_Error, which is the correct outcome
	if( !NET_TEST_TRY() )
		return 0;

	if( selector & 1 )
	{
		int i;
		for( i = 0; i < 64 && msg.readcount <= msg.cursize; i++ )
			SNAP_ParseBaseline( &msg, fuzz_baselines );
	}
	else
	{
		int suppressCount = 0;
		SNAP_ParseFrame( &msg, NULL, &suppressCount, fuzz_backup, fuzz_baselines, 0 );
	}

	NET_TEST_OK();
	return 0;
}
