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
// libFuzzer target for the message read primitives.
//
// The first input byte selects which reader to drive, so one corpus covers all of
// them. Build with -fsanitize=fuzzer,address - the fuzzer only supplies bytes;
// ASan is what turns an overrun into a failure.
#include "../qcommon.h"
#include "net_test_stubs.h"

#include <stdint.h>
#include <string.h>

#define GUARD_SIZE 64

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

	// Com_Error unwinds back to here rather than aborting, so a rejected input is
	// not reported as a crash
	if( !NET_TEST_TRY() )
		return 0;

	switch( selector % 5 )
	{
	case 0:
		{
			// scalar readers, run until the message is exhausted
			int i;
			for( i = 0; i < 64 && msg.readcount <= msg.cursize; i++ )
			{
				MSG_ReadChar( &msg );
				MSG_ReadByte( &msg );
				MSG_ReadShort( &msg );
				MSG_ReadUShort( &msg );
				MSG_ReadInt3( &msg );
				MSG_ReadLong( &msg );
				MSG_ReadFloat( &msg );
			}
		}
		break;

	case 1:
		{
			// string readers: bounded by a shared static, so the interesting part is
			// that they terminate and stay inside it
			int i;
			for( i = 0; i < 16 && msg.readcount <= msg.cursize; i++ )
			{
				MSG_ReadString( &msg );
				MSG_ReadStringLine( &msg );
			}
		}
		break;

	case 2:
		{
			// MSG_ReadData with a length taken from the input itself
			uint8_t dest[512 + GUARD_SIZE];
			size_t length;

			memset( dest, 0, sizeof( dest ) );
			length = (size_t)MSG_ReadLong( &msg );
			MSG_ReadData( &msg, dest, 512, length );
			MSG_SkipData( &msg, length );
		}
		break;

	case 3:
		{
			// delta entity decode from a zeroed baseline
			entity_state_t from, to;
			unsigned bits;
			int number;
			int i;

			memset( &from, 0, sizeof( from ) );
			for( i = 0; i < 32 && msg.readcount <= msg.cursize; i++ )
			{
				number = MSG_ReadEntityBits( &msg, &bits );
				memset( &to, 0, sizeof( to ) );
				MSG_ReadDeltaEntity( &msg, &from, &to, number, bits );
			}
		}
		break;

	case 4:
		{
			usercmd_t from, cmd;
			int i;

			memset( &from, 0, sizeof( from ) );
			for( i = 0; i < 32 && msg.readcount <= msg.cursize; i++ )
			{
				memset( &cmd, 0, sizeof( cmd ) );
				MSG_ReadDeltaUsercmd( &msg, &from, &cmd );
			}
		}
		break;
	}

	NET_TEST_OK();
	return 0;
}
