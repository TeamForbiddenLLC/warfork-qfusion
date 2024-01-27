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

}

/*
* Steam_Shutdown
*/
void Steam_Shutdown( void )
{
	STEAMSHIM_deinit();
}

/*
* Steam_Active
*/
int Steam_Active(){
	return STEAMSHIM_alive();
}

