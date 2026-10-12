#include <SDL.h>
#include "../client/client.h"
#ifdef __WASM__
#include <emscripten.h>
#include "../wasm/wasm_net.h"
#include "../wasm/wasm_trace.h"
#endif

#if defined( _WIN32 )
#include "../win32/resource.h"
#endif

#if defined( __APPLE__ )
#include <CoreFoundation/CoreFoundation.h>
#include <sys/param.h>
#endif

unsigned sys_frame_time;

void Sys_InitTime( void );

void Sys_Sleep( unsigned int millis )
{
	SDL_Delay( millis );
}

void Sys_Error( const char *format, ... )
{
	va_list argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	printf( "Sys_Error: %s\n", msg );
#ifdef __WASM__
	EM_ASM({
		var text = UTF8ToString($0);
		if (typeof Module !== 'undefined' && Module.printErr) {
			Module.printErr(text);
		}
		if (typeof window !== 'undefined') {
			window.dispatchEvent(new CustomEvent('warfork-error', { detail: text }));
		}
	}, msg);
#endif
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, APPLICATION, msg, NULL );

	exit( 1 );
}

/*
* Sys_Init
*/
void Sys_Init( void )
{
	Sys_InitTime();
#ifdef __WASM__
	Sys_WASM_Init();
#else
	/* native platform init stays in the existing Sys_* helpers */
#endif
}

/*
* Sys_InitDynvars
*/
void Sys_InitDynvars( void )
{
}

/*
* Sys_Quit
*/
void Sys_Quit( void )
{
	Qcommon_Shutdown();

	exit( 0 );
}

/*
* Sys_AcquireWakeLock
*/
void *Sys_AcquireWakeLock( void )
{
	return NULL;
}

/*
* Sys_ReleaseWakeLock
*/
void Sys_ReleaseWakeLock( void *wl )
{
}

/*
* Sys_AppActivate
*/
void Sys_AppActivate( void )
{
}

/*
* Sys_SendKeyEvents
*/
void Sys_SendKeyEvents( void )
{
	// grab frame time
	sys_frame_time = Sys_Milliseconds();
}

char *Sys_ConsoleInput( void )
{
	return NULL;
}

void Sys_ConsoleOutput( char *string )
{
}

/*****************************************************************************/

#ifdef __WASM__
static unsigned int wasm_oldtime;

static void Sys_WASM_Frame( void )
{
	unsigned int newtime, time;
	static int wasm_frame_n;

	newtime = Sys_Milliseconds();
	time = newtime - wasm_oldtime;
	if( !time )
		return;
	wasm_oldtime = newtime;
	wasm_frame_n++;
	if( wasm_frame_n <= 3 )
		WasmTrace( "Sys_WASM_Frame begin" );
	Qcommon_Frame( time );
	if( wasm_frame_n <= 3 )
		WasmTrace( "Sys_WASM_Frame end" );
}
#endif

int main( int argc, char **argv )
{
	unsigned int oldtime, newtime, time;

#if defined( __APPLE__ ) && !defined( DEDICATED_ONLY )
	char resourcesPath[MAXPATHLEN];
	CFURLGetFileSystemRepresentation( CFBundleCopyResourcesDirectoryURL( CFBundleGetMainBundle() ), 1, (UInt8 *)resourcesPath, MAXPATHLEN );
	chdir( resourcesPath );
#endif

#if defined( __WIN32__ )
#if defined( _DEBUG )
	SDL_SetHint( SDL_HINT_ALLOW_TOPMOST, "0" );
#endif
#endif

#ifdef __WASM__
	WasmTrace( "main: SDL_Init" );
#endif
	SDL_Init( SDL_INIT_VIDEO );

#ifdef __WASM__
	WasmTrace( "main: Qcommon_Init" );
#endif
	Qcommon_Init( argc, argv );

#ifdef __WASM__
	(void)oldtime;
	(void)newtime;
	(void)time;
	wasm_oldtime = Sys_Milliseconds();
	/* simulate_infinite_loop=0: callMain can return; EXIT_RUNTIME=0 keeps the loop alive. */
	emscripten_set_main_loop( Sys_WASM_Frame, 0, 0 );
#else
	oldtime = Sys_Milliseconds();
	while( true ) {
		// find time spent rendering last frame
		do {
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
			if( time > 0 )
				break;
			Sys_Sleep( 0 );
		} while( 1 );
		oldtime = newtime;

		Qcommon_Frame( time );
	}

	SDL_Quit();
#endif // __WASM__
}
