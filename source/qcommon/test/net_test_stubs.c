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
// Minimal engine surface needed to link msg.c / snap_read.c / net_chan.c into a
// test or fuzz binary. See net_test_stubs.h for how Com_Error is trapped.
// qcommon.h defines a file-static `default_fs_imports_s` table holding the
// address of every FS_* entry point. Nothing here calls it, but the compiler
// still emits it at -O0, and its relocations then demand the whole filesystem
// layer at link time. Emitting the module's own forwarding wrappers - the same
// mechanism ref_gl uses - satisfies those references without dragging in
// files.c. The wrappers forward to a zeroed import table and would crash if
// called, which is correct: none of the parsers under test touch the
// filesystem, so a call here means the harness grew a dependency it should not
// have.
#define FS_DEFINE_INTERFACE_IMPL 1
#include "../mod_fs.h"

#include "../qcommon.h"
#include "net_test_stubs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

jmp_buf net_test_abort_jmp;
bool net_test_abort_armed = false;
int net_test_abort_code = 0;
char net_test_abort_msg[1024];

// Set to true by a harness that wants parser chatter on stdout. Off by default so
// fuzzing is not bottlenecked on I/O.
static bool net_test_verbose = false;

void Com_Error( com_error_code_t code, const char *format, ... )
{
	va_list argptr;

	va_start( argptr, format );
	vsnprintf( net_test_abort_msg, sizeof( net_test_abort_msg ), format, argptr );
	va_end( argptr );

	// keep the message non-empty even for an empty format, so NET_TEST_ERRORED works
	if( !net_test_abort_msg[0] )
		strcpy( net_test_abort_msg, "Com_Error" );

	net_test_abort_code = code;

	if( net_test_verbose )
		printf( "Com_Error(%i): %s\n", code, net_test_abort_msg );

	if( net_test_abort_armed )
	{
		net_test_abort_armed = false;
		longjmp( net_test_abort_jmp, 1 );
	}

	// Nothing armed the trap, so there is no sane way to continue: the caller's
	// invariants are what Com_Error was reporting as broken.
	abort();
}

void Sys_Error( const char *error, ... )
{
	va_list argptr;

	va_start( argptr, error );
	vsnprintf( net_test_abort_msg, sizeof( net_test_abort_msg ), error, argptr );
	va_end( argptr );

	if( !net_test_abort_msg[0] )
		strcpy( net_test_abort_msg, "Sys_Error" );

	if( net_test_verbose )
		printf( "Sys_Error: %s\n", net_test_abort_msg );

	if( net_test_abort_armed )
	{
		net_test_abort_armed = false;
		longjmp( net_test_abort_jmp, 1 );
	}

	abort();
}

void Com_Printf( const char *format, ... )
{
	va_list argptr;

	if( !net_test_verbose )
		return;

	va_start( argptr, format );
	vprintf( format, argptr );
	va_end( argptr );
}

void Com_DPrintf( const char *format, ... )
{
	va_list argptr;

	if( !net_test_verbose )
		return;

	va_start( argptr, format );
	vprintf( format, argptr );
	va_end( argptr );
}

//
// net.c surface used by net_chan.c
//

static char net_test_addr_string[] = "test:0";

char *NET_AddressToString( const netadr_t *address )
{
	(void)address;
	return net_test_addr_string;
}

const char *NET_SocketToString( const socket_t *socket )
{
	(void)socket;
	return net_test_addr_string;
}

const char *NET_ErrorString( void )
{
	return "no error";
}

bool NET_SendPacket( const socket_t *socket, const void *data, size_t length, const netadr_t *address )
{
	// swallow outgoing traffic; the fuzzers only care about the receive path
	(void)socket; (void)data; (void)length; (void)address;
	return true;
}

unsigned int Sys_Milliseconds( void )
{
	// deterministic: a wall clock would make fuzz findings unreproducible
	static unsigned int fake_ms = 0;
	return fake_ms++;
}

//
// cvar.c surface used by net_chan.c
//
// Netchan_Init only registers the showpackets / showdrop / showfragments
// diagnostics. Every one is handed back as 0 so the parsers take their quiet
// path - fuzzing should not be bottlenecked on printf.
//
cvar_t *Cvar_Get( const char *var_name, const char *var_value, cvar_flag_t flags )
{
	static char empty[] = "0";
	static cvar_t stub_cvars[16];
	static size_t next_cvar = 0;
	cvar_t *var;

	(void)var_value; (void)flags;

	if( next_cvar >= sizeof( stub_cvars ) / sizeof( stub_cvars[0] ) )
		return &stub_cvars[0];

	var = &stub_cvars[next_cvar++];
	var->name = (char *)var_name;
	var->string = empty;
	var->dvalue = empty;
	var->latched_string = NULL;
	var->flags = flags;
	var->modified = false;
	var->value = 0.0f;
	var->integer = 0;

	return var;
}
