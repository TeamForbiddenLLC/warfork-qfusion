/*
Copyright (C) 2014 Victor Luchits

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
#include "../qcommon/qcommon.h"
#include "../steamshim/src/child/child.h"
#include <string.h>

#define UNIMPLEMENTED_DBGBREAK()                                         \
	do {                                                                 \
		Com_Printf( S_COLOR_RED "%s is UNIMPLEMENTED\n", __FUNCTION__ ); \
		assert( 0 );                                                     \
	} while( 0 )

static void printEvent( const STEAMSHIM_Event *e )
{
	if( !e )
		return;

	Com_Printf( "%sokay, ival=%d, fval=%f, lval=%llu, name='%s').\n", e->okay ? "" : "!", e->ivalue, e->fvalue, e->lvalue, e->name );
} /* printEvent */

static const STEAMSHIM_Event* blockOnEvent(STEAMSHIM_EventType type){

	while( 1 ) {
		const STEAMSHIM_Event *evt = STEAMSHIM_pump();
		if (!evt) continue;

		if (evt->type == type){
			printEvent( evt );
			return evt;
		} else {
			printf("warning: ignoring event %i\n",evt->type);
			// event gets ignored!
			printEvent( evt );
		}
	}
}

/*
* Steam_Init
*/
void Steam_Init( void )
{
#if DEDICATED_ONLY 
	int r = STEAMSHIM_init(false);
#else
	int r = STEAMSHIM_init(true);
#endif
	if( !r ) {
		Com_Printf( "Steam initialization failed.\n" );
		return;
	}

	// STEAMSHIM_requestStats();
}

/*
* Steam_RunFrame
*/
void Steam_RunFrame( void )
{
	const STEAMSHIM_Event *evt = STEAMSHIM_pump();
	if( evt ) {
		printEvent( evt );
	}
}

/*
* Steam_Shutdown
*/
void Steam_Shutdown( void )
{
	STEAMSHIM_deinit();
}



/*
* Steam_GetAuthSessionTicket
*/
int Steam_GetAuthSessionTicket( void ( *callback )( void *, size_t ) )
{
	// coolelectronics: not implementing anything here until i have a better understanding of cl_mm.c
	return 0;
}

/*
* Steam_GetAuthSessionTicketBlocking
*/
const SteamAuthTicket_t* Steam_GetAuthSessionTicketBlocking(){
	static SteamAuthTicket_t ticket;

	STEAMSHIM_getAuthSessionTicket();
	const STEAMSHIM_Event *evt = blockOnEvent(SHIMEVENT_AUTHSESSIONTICKETRECIEVED);

	ticket.pcbTicket = evt->lvalue;
	memcpy(ticket.pTicket, evt->name, AUTH_TICKET_MAXSIZE);

	return &ticket;
}

int Steam_BeginAuthSession(uint64_t steamid, SteamAuthTicket_t *ticket){

	STEAMSHIM_beginAuthSession(steamid,ticket);
	const STEAMSHIM_Event *evt = blockOnEvent(SHIMEVENT_AUTHSESSIONVALIDATED);

	return evt->ivalue;
}

void Steam_EndAuthSession(uint64_t steamid){
	STEAMSHIM_endAuthSession(steamid);
}

/*
* Steam_AdvertiseGame
*/
void Steam_AdvertiseGame( const uint8_t *ip, unsigned short port )
{
	// UNIMPLEMENTED_DBGBREAK();
}

/*
* Steam_GetPersonaName
*/
void Steam_GetPersonaName( char *name, size_t namesize )
{
	if( !namesize ) {
		return;
	}
	STEAMSHIM_getPersonaName();
	const STEAMSHIM_Event *evt = blockOnEvent(SHIMEVENT_PERSONANAMERECIEVED);
	strncpy(name, evt->name, namesize);


	printf("making beacon\n");
	STEAMSHIM_createBeacon(1, "a", "b");
}

/*
* Steam_SetRichPresence
*/
void Steam_SetRichPresence( int num, const char **key, const char **val )
{
	STEAMSHIM_setRichPresence(num, key, val);
}
/*
* Steam_GetSteamID
*/
uint64_t Steam_GetSteamID( void )
{
	STEAMSHIM_getSteamID();
	const STEAMSHIM_Event *evt = blockOnEvent(SHIMEVENT_STEAMIDRECIEVED);
	return evt->lvalue;
}

/*
* Steam_Active
*/
int Steam_Active(){
	return STEAMSHIM_alive();
}

