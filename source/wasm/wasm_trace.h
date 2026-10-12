#ifndef WASM_TRACE_H
#define WASM_TRACE_H

#if defined( __EMSCRIPTEN__ ) || defined( __WASM__ )
#include <emscripten.h>
static inline void WasmTrace( const char *msg )
{
	EM_ASM( { console.log( '[wasm] ' + UTF8ToString( $0 ) ); }, msg );
}
#else
static inline void WasmTrace( const char *msg )
{
	(void)msg;
}
#endif

#endif
