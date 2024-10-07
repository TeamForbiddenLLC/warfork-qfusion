#include "../qcommon/qcommon.h"
#include "../qcommon/steam.h"
#include "../steamshim/src/parent/parent.h"
#include <string.h>

static const SteamshimEvent* blockOnEvent(SteamshimEventType type){

	//while( 1 ) {
	//	const SteamshimEvent *evt = STEAMSHIM_pump();
	//	if (!evt) continue;

	//	if (evt->type == type){
	//		return evt;
	//	} else {
	//		printf("warning: ignoring event %i\n",evt->type);
	//	}
	//}
	return NULL;
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
	return 0;
	//STEAMSHIM_beginAuthSession(steamid,ticket);
	//const SteamshimEvent *evt = blockOnEvent(EVT_SV_AUTHSESSIONVALIDATED);

	//return evt->sv_authsessionvalidated;
}

void Steam_EndAuthSession(uint64_t steamid){
	//STEAMSHIM_endAuthSession(steamid);
}
