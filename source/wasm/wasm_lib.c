#ifdef __WASM__

#include "../qcommon/qcommon.h"
#include "../qcommon/sys_library.h"

#include <string.h>

/* Addresses of the statically linked module entry points. */
extern void *GetRefAPI();
extern void *GetSoundAPI();
extern void *GetUIAPI();
extern void *GetFTLibAPI();
extern void *GetCinematicsAPI();
extern void *GetAngelwrapAPI();
extern void *GetGameAPI();
extern void *GetCGameAPI();

typedef struct {
	const char *token;
	const char *proc;
	void *fn;
} wasm_export_t;

static const wasm_export_t wasm_exports[] = {
	{ "ref_gl", "GetRefAPI", (void *)GetRefAPI },
	{ "snd_openal", "GetSoundAPI", (void *)GetSoundAPI },
	{ "ui_", "GetUIAPI", (void *)GetUIAPI },
	{ "ftlib", "GetFTLibAPI", (void *)GetFTLibAPI },
	{ "cin", "GetCinematicsAPI", (void *)GetCinematicsAPI },
	{ "angelwrap", "GetAngelwrapAPI", (void *)GetAngelwrapAPI },
	{ "cgame", "GetCGameAPI", (void *)GetCGameAPI },
	{ "game", "GetGameAPI", (void *)GetGameAPI },
	{ NULL, NULL, NULL }
};

static const char *wasm_lib_error = "";

static const wasm_export_t *Wasm_Match( const char *name )
{
	const wasm_export_t *it;

	if( !name )
		return NULL;
	for( it = wasm_exports; it->token; it++ ) {
		if( strstr( name, it->token ) )
			return it;
	}
	return NULL;
}

bool Sys_Library_Close( void *lib )
{
	(void)lib;
	return true;
}

const char *Sys_Library_GetFullName( const char *name )
{
	return name;
}

const char *Sys_Library_GetGameLibPath( const char *name, int64_t time, int randomizer )
{
	(void)time;
	(void)randomizer;
	return name;
}

void *Sys_Library_Open( const char *name )
{
	const wasm_export_t *exp = Wasm_Match( name );

	wasm_lib_error = "";
	if( !exp ) {
		wasm_lib_error = "statically linked module not found";
		return NULL;
	}
	return (void *)exp;
}

void *Sys_Library_ProcAddress( void *lib, const char *apifuncname )
{
	const wasm_export_t *exp = (const wasm_export_t *)lib;

	if( !exp || !apifuncname )
		return NULL;
	if( !strcmp( exp->proc, apifuncname ) )
		return exp->fn;
	return NULL;
}

const char *Sys_Library_ErrorString( void )
{
	return wasm_lib_error;
}

#endif // __WASM__
