#ifndef R_SV_MODULE_H
#define R_SV_MODULE_H

#include "../gameshared/q_arch.h"
#include "../gameshared/q_shared.h"

// The subset of server.h that sv_game.c plumbs into game_import_t,
// either as a direct bind or as the substrate of a PF_* shim.

struct client_s;
struct stat_query_s;

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret(*name##Fn )( __VA_ARGS__ ); \
	ret name (__VA_ARGS__);

// asset indices (direct binds)
DECLARE_TYPEDEF_METHOD( int, SV_ModelIndex, const char *name );
DECLARE_TYPEDEF_METHOD( int, SV_SoundIndex, const char *name );
DECLARE_TYPEDEF_METHOD( int, SV_ImageIndex, const char *name );
DECLARE_TYPEDEF_METHOD( int, SV_SkinIndex, const char *name );

// client lifecycle (direct binds)
DECLARE_TYPEDEF_METHOD( int, SVC_FakeConnect, char *fakeUserinfo, char *fakeSocketType, const char *fakeIP );
DECLARE_TYPEDEF_METHOD( void, SV_ExecuteClientThinks, int clientNum );

// game/server reliable command queues (substrate of PF_GameCmd / PF_ServerCmd)
DECLARE_TYPEDEF_METHOD( void, SV_AddGameCommand, struct client_s *client, const char *cmd );
DECLARE_TYPEDEF_METHOD( void, SV_AddServerCommand, struct client_s *client, const char *cmd );

// pure file list (substrate of PF_PureSound / PF_PureModel)
DECLARE_TYPEDEF_METHOD( void, SV_AddPureFile, const char *filename );

// matchmaking
DECLARE_TYPEDEF_METHOD( struct stat_query_s *, SV_MM_CreateQuery, const char *iface, const char *url, bool get );
DECLARE_TYPEDEF_METHOD( void, SV_MM_SendQuery, struct stat_query_s *query );
DECLARE_TYPEDEF_METHOD( void, SV_MM_GameState, bool state );

#undef DECLARE_TYPEDEF_METHOD

struct sv_import_s {
	SV_ModelIndexFn SV_ModelIndex;
	SV_SoundIndexFn SV_SoundIndex;
	SV_ImageIndexFn SV_ImageIndex;
	SV_SkinIndexFn SV_SkinIndex;
	SVC_FakeConnectFn SVC_FakeConnect;
	SV_ExecuteClientThinksFn SV_ExecuteClientThinks;
	SV_AddGameCommandFn SV_AddGameCommand;
	SV_AddServerCommandFn SV_AddServerCommand;
	SV_AddPureFileFn SV_AddPureFile;
	SV_MM_CreateQueryFn SV_MM_CreateQuery;
	SV_MM_SendQueryFn SV_MM_SendQuery;
	SV_MM_GameStateFn SV_MM_GameState;
};

#if SV_DEFINE_INTERFACE_IMPL
static struct sv_import_s sv_import;

int SV_ModelIndex( const char *name ) { return sv_import.SV_ModelIndex( name ); }
int SV_SoundIndex( const char *name ) { return sv_import.SV_SoundIndex( name ); }
int SV_ImageIndex( const char *name ) { return sv_import.SV_ImageIndex( name ); }
int SV_SkinIndex( const char *name ) { return sv_import.SV_SkinIndex( name ); }
int SVC_FakeConnect( char *fakeUserinfo, char *fakeSocketType, const char *fakeIP ) { return sv_import.SVC_FakeConnect( fakeUserinfo, fakeSocketType, fakeIP ); }
void SV_ExecuteClientThinks( int clientNum ) { sv_import.SV_ExecuteClientThinks( clientNum ); }
void SV_AddGameCommand( struct client_s *client, const char *cmd ) { sv_import.SV_AddGameCommand( client, cmd ); }
void SV_AddServerCommand( struct client_s *client, const char *cmd ) { sv_import.SV_AddServerCommand( client, cmd ); }
void SV_AddPureFile( const char *filename ) { sv_import.SV_AddPureFile( filename ); }
struct stat_query_s *SV_MM_CreateQuery( const char *iface, const char *url, bool get ) { return sv_import.SV_MM_CreateQuery( iface, url, get ); }
void SV_MM_SendQuery( struct stat_query_s *query ) { sv_import.SV_MM_SendQuery( query ); }
void SV_MM_GameState( bool state ) { sv_import.SV_MM_GameState( state ); }

static inline void Q_ImportSvModule( struct sv_import_s *mod ) {
	sv_import = *mod;
}

#endif

#endif
