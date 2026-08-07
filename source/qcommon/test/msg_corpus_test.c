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
// Regression corpus for the network memory-safety audit.
//
// Each test replays a packet that used to corrupt memory. The assertion is not
// "the right value came out" - it is "the parser refused, and nothing outside the
// destination buffer was touched". Run under ASan; the guard bytes around each
// destination catch anything ASan would not (a write that lands inside an
// adjacent live object).
#include "../qcommon.h"
#include "net_test_stubs.h"
#include "utest.h"

#include <string.h>

// A destination buffer with poisoned margins on both sides, so an overrun in
// either direction is visible even without a sanitizer.
#define GUARD_BYTE 0xA5
#define GUARD_SIZE 64
#define PAYLOAD_SIZE 4096

typedef struct
{
	uint8_t front[GUARD_SIZE];
	uint8_t payload[PAYLOAD_SIZE];
	uint8_t back[GUARD_SIZE];
} guarded_buf_t;

static void guarded_init( guarded_buf_t *g )
{
	memset( g, GUARD_BYTE, sizeof( *g ) );
}

static bool guarded_intact( const guarded_buf_t *g )
{
	size_t i;

	for( i = 0; i < GUARD_SIZE; i++ )
	{
		if( g->front[i] != GUARD_BYTE || g->back[i] != GUARD_BYTE )
			return false;
	}
	return true;
}

// Builder for a message the parsers will read back.
typedef struct
{
	uint8_t data[MAX_MSGLEN];
	msg_t msg;
} packet_t;

static void packet_begin( packet_t *p )
{
	MSG_Init( &p->msg, p->data, sizeof( p->data ) );
	MSG_Clear( &p->msg );
}

static void packet_finish( packet_t *p )
{
	MSG_BeginReading( &p->msg );
}

//
// MSG_ReadData
//

UTEST( msg, read_data_rejects_oversized_length )
{
	packet_t p;
	guarded_buf_t dest;
	uint8_t small[16];

	packet_begin( &p );
	MSG_WriteData( &p.msg, "0123456789ABCDEF", 16 );
	packet_finish( &p );

	guarded_init( &dest );
	memset( small, 0, sizeof( small ) );

	// asking for more than the destination holds must copy nothing
	EXPECT_FALSE( MSG_ReadData( &p.msg, dest.payload, sizeof( small ), 4096 ) );
	EXPECT_TRUE( guarded_intact( &dest ) );
	EXPECT_EQ( dest.payload[0], (uint8_t)GUARD_BYTE );

	// and the message is poisoned so the caller's bad-read guard trips
	EXPECT_GT( p.msg.readcount, p.msg.cursize );
}

UTEST( msg, read_data_rejects_read_past_end_of_message )
{
	packet_t p;
	guarded_buf_t dest;

	packet_begin( &p );
	MSG_WriteData( &p.msg, "short", 5 );
	packet_finish( &p );

	guarded_init( &dest );

	// fits the destination, but the message only has 5 bytes left
	EXPECT_FALSE( MSG_ReadData( &p.msg, dest.payload, sizeof( dest.payload ), 512 ) );
	EXPECT_TRUE( guarded_intact( &dest ) );
	EXPECT_EQ( dest.payload[0], (uint8_t)GUARD_BYTE );
	EXPECT_GT( p.msg.readcount, p.msg.cursize );
}

UTEST( msg, read_data_rejects_negative_length_widened_to_size_t )
{
	packet_t p;
	guarded_buf_t dest;
	int negative = -1;

	packet_begin( &p );
	MSG_WriteData( &p.msg, "short", 5 );
	packet_finish( &p );

	guarded_init( &dest );

	// this is the shape of the clc_steamauth / clc_voice bugs: a signed length
	// read off the wire, widened to size_t at the call
	EXPECT_FALSE( MSG_ReadData( &p.msg, dest.payload, sizeof( dest.payload ), (size_t)negative ) );
	EXPECT_TRUE( guarded_intact( &dest ) );
	EXPECT_EQ( dest.payload[0], (uint8_t)GUARD_BYTE );
}

UTEST( msg, read_data_accepts_exact_fit )
{
	packet_t p;
	guarded_buf_t dest;

	packet_begin( &p );
	MSG_WriteData( &p.msg, "0123456789ABCDEF", 16 );
	packet_finish( &p );

	guarded_init( &dest );

	EXPECT_TRUE( MSG_ReadData( &p.msg, dest.payload, 16, 16 ) );
	EXPECT_TRUE( guarded_intact( &dest ) );
	EXPECT_EQ( 0, memcmp( dest.payload, "0123456789ABCDEF", 16 ) );
	EXPECT_EQ( p.msg.readcount, p.msg.cursize );
	// the byte after the read must be untouched
	EXPECT_EQ( dest.payload[16], (uint8_t)GUARD_BYTE );
}

//
// Read primitives at the end-of-message boundary
//

UTEST( msg, read_char_does_not_read_past_cursize )
{
	packet_t p;

	packet_begin( &p );
	MSG_WriteByte( &p.msg, 0x7F );
	packet_finish( &p );

	EXPECT_EQ( 0x7F, MSG_ReadChar( &p.msg ) );
	// the second read is past the end; it must report -1 without dereferencing
	EXPECT_EQ( -1, MSG_ReadChar( &p.msg ) );
}

UTEST( msg, read_ushort_does_not_sign_extend )
{
	packet_t p;

	packet_begin( &p );
	MSG_WriteShort( &p.msg, 0xFFFF );
	MSG_WriteShort( &p.msg, 0x8000 );
	packet_finish( &p );

	// MSG_ReadShort would hand these back as -1 and -32768, which is what let
	// negative lengths past the "> MAX" checks in the clc handlers
	EXPECT_EQ( 0xFFFF, MSG_ReadUShort( &p.msg ) );
	EXPECT_EQ( 0x8000, MSG_ReadUShort( &p.msg ) );

	// -1 is still reserved for end-of-message
	EXPECT_EQ( -1, MSG_ReadUShort( &p.msg ) );
}

UTEST( msg, skip_data_does_not_wrap )
{
	packet_t p;

	packet_begin( &p );
	MSG_WriteData( &p.msg, "short", 5 );
	packet_finish( &p );

	// readcount + length would wrap past cursize and silently "succeed"
	EXPECT_EQ( 0, MSG_SkipData( &p.msg, (size_t)-1 ) );
	EXPECT_EQ( (size_t)0, p.msg.readcount );

	EXPECT_EQ( 1, MSG_SkipData( &p.msg, 5 ) );
	EXPECT_EQ( p.msg.cursize, p.msg.readcount );
}

//
// Snapshot parsing
//

// Write an entity header the way MSG_ReadEntityBits reads it back, forcing the
// 16-bit number encoding so the number can exceed a byte.
static void write_entity_bits16( msg_t *msg, unsigned bits, int number )
{
	unsigned total = bits | U_NUMBER16;

	MSG_WriteByte( msg, total & 0xFF );
	if( total & U_MOREBITS1 )
		MSG_WriteByte( msg, ( total >> 8 ) & 0xFF );
	if( total & U_MOREBITS2 )
		MSG_WriteByte( msg, ( total >> 16 ) & 0xFF );
	if( total & U_MOREBITS3 )
		MSG_WriteByte( msg, ( total >> 24 ) & 0xFF );

	MSG_WriteShort( msg, number );
}

UTEST( snap, baseline_rejects_out_of_range_entity_number )
{
	packet_t p;
	static entity_state_t baselines[MAX_EDICTS];

	memset( baselines, 0, sizeof( baselines ) );

	packet_begin( &p );
	// U_NUMBER16 makes the number a sign-extended short; 0xFFFF reads back as -1
	// and used to index baselines[-1], writing a whole entity_state_t there
	write_entity_bits16( &p.msg, U_MOREBITS1, 0xFFFF );
	packet_finish( &p );

	if( NET_TEST_TRY() )
	{
		SNAP_ParseBaseline( &p.msg, baselines );
		NET_TEST_OK();
	}

	EXPECT_TRUE( NET_TEST_ERRORED() );
}

UTEST( snap, baseline_rejects_entity_number_at_max_edicts )
{
	packet_t p;
	static entity_state_t baselines[MAX_EDICTS];

	memset( baselines, 0, sizeof( baselines ) );

	packet_begin( &p );
	write_entity_bits16( &p.msg, U_MOREBITS1, MAX_EDICTS );
	packet_finish( &p );

	if( NET_TEST_TRY() )
	{
		SNAP_ParseBaseline( &p.msg, baselines );
		NET_TEST_OK();
	}

	EXPECT_TRUE( NET_TEST_ERRORED() );
}

UTEST( snap, baseline_accepts_valid_entity_number )
{
	packet_t p;
	static entity_state_t baselines[MAX_EDICTS];

	memset( baselines, 0, sizeof( baselines ) );

	packet_begin( &p );
	write_entity_bits16( &p.msg, U_MOREBITS1, 42 );
	packet_finish( &p );

	if( NET_TEST_TRY() )
	{
		SNAP_ParseBaseline( &p.msg, baselines );
		NET_TEST_OK();
	}

	EXPECT_FALSE( NET_TEST_ERRORED() );
	EXPECT_EQ( 42, baselines[42].number );
}

//
// Delta entity field ranges
//

UTEST( msg, delta_entity_clamps_index_fields )
{
	packet_t p;
	entity_state_t from, to;

	memset( &from, 0, sizeof( from ) );
	memset( &to, 0, sizeof( to ) );

	packet_begin( &p );
	MSG_WriteShort( &p.msg, 0xFFFF );	// U_MODEL  -> modelindex
	MSG_WriteShort( &p.msg, 0xFFFF );	// U_SOUND  -> sound
	packet_finish( &p );

	MSG_ReadDeltaEntity( &p.msg, &from, &to, 1, U_MODEL | U_SOUND );

	// both are used directly as subscripts by the cgame
	EXPECT_LT( to.modelindex, (unsigned)MAX_MODELS );
	EXPECT_GE( to.sound, 0 );
	EXPECT_LT( to.sound, MAX_SOUNDS );
}

UTEST_MAIN();
