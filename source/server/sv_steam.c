#include "../qcommon/qcommon.h"
#include "../qcommon/steam.h"
#include "../steamshim/src/child/child.h"
#include <string.h>

static void printEvent( const STEAMSHIM_Event *e )
{
	if( !e )
		return;

	Com_Printf( "%sokay, ival=%d, fval=%f, lval=%llu, name='%s').\n", e->okay ? "" : "!", e->ivalue, e->fvalue, e->lvalue, e->name );
} 

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
* Steam_RunFrame
*/
void SV_Steam_RunFrame( void )
{
	const STEAMSHIM_Event *evt = STEAMSHIM_pump();
	if( evt ) {
		printEvent( evt );
		switch (evt->type){
			default: break;
		}
	}
}

/*
* Steam_GetAuthSessionTicket
*/
int Steam_GetAuthSessionTicket( void ( *callback )( void *, size_t ) )
{
	// coolelectronics: not implementing anything here until i have a better understanding of cl_mm.c
	return 0;
}

int Steam_BeginAuthSession(uint64_t steamid, SteamAuthTicket_t *ticket){

	STEAMSHIM_beginAuthSession(steamid,ticket);
	const STEAMSHIM_Event *evt = blockOnEvent(SHIMEVENT_AUTHSESSIONVALIDATED);

	return evt->ivalue;
}

void Steam_EndAuthSession(uint64_t steamid){
	STEAMSHIM_endAuthSession(steamid);
}
