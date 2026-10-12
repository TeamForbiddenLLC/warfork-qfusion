#include "../qas_precompiled.h"
#include "as_generic_runtime.h"

#if defined( __EMSCRIPTEN__ ) || defined( __WASM__ )

namespace ASGen
{
	Slot slots[MAX_SLOTS];
	int nslots = 0;

	void InvokeSlot( int i, asIScriptGeneric *g )
	{
		static int traces;
		if( i < 0 || i >= nslots || !slots[i].invoke ) {
			if( traces < 8 ) {
				Com_Printf( "[wasm] AS InvokeSlot skip i=%d nslots=%d invoke=%p\n",
					i, nslots, ( i >= 0 && i < nslots ) ? (void *)slots[i].invoke : (void *)0 );
				traces++;
			}
			return;
		}
		if( traces < 8 ) {
			Com_Printf( "[wasm] AS InvokeSlot i=%d\n", i );
			traces++;
		}
		slots[i].invoke( g, &slots[i] );
	}
}

#include "as_generic_thunks.inc"

#endif
