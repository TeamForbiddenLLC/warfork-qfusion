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
// libFuzzer target for userinfo string handling.
//
// Threat model: a connecting client supplies the whole info string, and the
// server then reads and rewrites keys on it (SVC_DirectConnect ->
// SV_UserinfoChanged). Info_SetValueForKey hardcodes MAX_INFO_STRING as the
// destination capacity, so the validators are load-bearing.
//
// Info_ValueForKey / Info_SetValueForKey / Info_RemoveKey all assert that their
// info string already passed Info_Validate, and every engine caller honours
// that. So does this harness: an input that fails validation is discarded
// rather than pushed through, which keeps the fuzzer on the reachable path
// (adversarial strings that validate) instead of reporting the precondition.
//
// Build with -fsanitize=fuzzer,address.
#include "../../gameshared/q_shared.h"

#include <stdint.h>
#include <string.h>

// keys the server actually sets or reads on client-supplied userinfo
static const char * const fuzz_keys[] = {
	"name", "ip", "socket", "steam_id", "rate", "msg", "clan", "skin", "color"
};
#define NUM_FUZZ_KEYS ( sizeof( fuzz_keys ) / sizeof( fuzz_keys[0] ) )

int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	char info[MAX_INFO_STRING];
	char value[MAX_INFO_VALUE];
	const char *key;
	size_t split, vlen;

	if( size < 4 )
		return 0;

	key = fuzz_keys[data[0] % NUM_FUZZ_KEYS];
	split = data[1] % ( size - 2 ) + 1;
	data += 2;
	size -= 2;

	// the info string as the client sent it
	memset( info, 0, sizeof( info ) );
	memcpy( info, data, split < sizeof( info ) - 1 ? split : sizeof( info ) - 1 );

	// discard anything the server would have rejected outright
	if( !Info_Validate( info ) )
		return 0;

	Info_ValueForKey( info, key );

	// Info_SetValueForKey also asserts on its value. Info_ValidateValue is
	// internal to q_shared.c, so mirror its rules here: shorter than
	// MAX_INFO_VALUE and none of the separator characters.
	memset( value, 0, sizeof( value ) );
	vlen = size - split;
	if( split < size )
		memcpy( value, data + split, vlen < sizeof( value ) - 1 ? vlen : sizeof( value ) - 1 );

	if( !strchr( value, '\\' ) && !strchr( value, ';' ) && !strchr( value, '"' ) )
		Info_SetValueForKey( info, key, value );

	Info_RemoveKey( info, key );

	// Whatever happened, the result must still be a NUL-terminated string that
	// fits MAX_INFO_STRING - every caller passes a buffer of exactly that size
	// and Info_SetValueForKey assumes it.
	if( strlen( info ) >= sizeof( info ) )
		__builtin_trap();

	// and it must still be something the validators accept, otherwise the next
	// Info_* call on it trips its own precondition
	if( !Info_Validate( info ) )
		__builtin_trap();

	return 0;
}
