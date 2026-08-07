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
// Host-side scaffolding so the network parsers can be driven outside the engine.
//
// The parsers reject malformed input by calling Com_Error( ERR_DROP, ... ), which
// in the real engine longjmps out to the main loop. The stub does the same thing
// into a buffer owned by the test, so "the parser rejected this" is an outcome a
// test can assert on rather than a process exit.
#ifndef NET_TEST_STUBS_H
#define NET_TEST_STUBS_H

#include <setjmp.h>
#include <stdbool.h>

extern jmp_buf net_test_abort_jmp;
extern bool net_test_abort_armed;
extern int net_test_abort_code;
extern char net_test_abort_msg[1024];

// Run 'body' with Com_Error trapped. Evaluates to true if the parser completed
// without erroring, false if it bailed out.
//
//   if( NET_TEST_TRY() ) { SNAP_ParseBaseline( &msg, baselines ); NET_TEST_OK(); }
//
#define NET_TEST_TRY() \
	( net_test_abort_msg[0] = '\0', net_test_abort_code = 0, \
	  net_test_abort_armed = true, setjmp( net_test_abort_jmp ) == 0 )

#define NET_TEST_OK() ( net_test_abort_armed = false )

// true when the last NET_TEST_TRY block ended in Com_Error
#define NET_TEST_ERRORED() ( net_test_abort_msg[0] != '\0' )

#endif // NET_TEST_STUBS_H
