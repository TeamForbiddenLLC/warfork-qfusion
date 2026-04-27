#ifndef _STEAM_MODULE_H
#define _STEAM_MODULE_H

#include "./steamshim_types.h"

struct steam_workshop_mod_s {
	uint64_t workshop_id;
	bool is_remote;
	bool is_local;  // discovered from the local mods folder, may be unpublished (workshop_id == 0)
	const char *path;
	const char *name;        // folder name for local mods, NULL for remote-only mods
	const char *title;
	const char *description;
	const char *tags;
	const char *preview_url;
};

#ifdef __cplusplus
#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
	extern "C" {                                 \
	ret name( __VA_ARGS__ );                     \
	}
#else
#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
	ret name( __VA_ARGS__ );
#endif

DECLARE_TYPEDEF_METHOD( int, STEAMSHIM_dispatch );
DECLARE_TYPEDEF_METHOD( int, STEAMSHIM_sendRPC, void *req, uint32_t size, void *self, STEAMSHIM_rpc_handle rpc, uint32_t *syncIndex );
DECLARE_TYPEDEF_METHOD( int, STEAMSHIM_sendEVT, void *packet, uint32_t size );
DECLARE_TYPEDEF_METHOD( int, STEAMSHIM_waitDispatchSync, uint32_t syncIndex ); // wait on the dispatch loop does not trigger steam callbacks
DECLARE_TYPEDEF_METHOD( void, STEAMSHIM_subscribeEvent, uint32_t id, void *self, STEAMSHIM_evt_handle evt );
DECLARE_TYPEDEF_METHOD( void, STEAMSHIM_unsubscribeEvent, uint32_t id, STEAMSHIM_evt_handle evt );
DECLARE_TYPEDEF_METHOD( bool, STEAMSHIM_active );
typedef enum {
	STEAM_PUBLISH_OK = 0,
	STEAM_PUBLISH_ERR_STEAM_UNAVAILABLE,
	STEAM_PUBLISH_ERR_TITLE_REQUIRED,
	STEAM_PUBLISH_ERR_INVALID_MOD,
	STEAM_PUBLISH_ERR_TOO_MANY_TAGS,
	STEAM_PUBLISH_ERR_CREATE_FAILED,
	STEAM_PUBLISH_ERR_UPDATE_FAILED,
	STEAM_PUBLISH_ERR_SET_TITLE,
	STEAM_PUBLISH_ERR_SET_DESCRIPTION,
	STEAM_PUBLISH_ERR_SET_CONTENT,
	STEAM_PUBLISH_ERR_SET_PREVIEW,
	STEAM_PUBLISH_ERR_SET_VISIBILITY,
	STEAM_PUBLISH_ERR_SET_TAGS,
	STEAM_PUBLISH_ERR_SUBMIT_FAILED,
} steam_publish_error_e;

// Input parameters for Steam_PublishLocalMod.
// All string fields are borrowed — they must remain valid for the duration of the call.
struct steam_workshop_publish_s {
	const char *title;
	const char *description;
	char* tags;        // comma-separated
	const char *visibility;  // "public" | "friends" | "private"
	const char *change_note;
};

// Returned by Steam_PublishLocalMod.
struct steam_workshop_publish_result_s {
	steam_publish_error_e res;       // STEAM_PUBLISH_OK on success
	uint32_t              steam_result; // raw Steam EResult, valid on create/submit failures
	uint64_t              file_id;      // assigned Workshop item ID
	bool                  is_new_item;  // true if a new item was created (vs updated)
};

DECLARE_TYPEDEF_METHOD( const struct steam_workshop_mod_s *, Steam_GetWorkshopMods, void );
DECLARE_TYPEDEF_METHOD( size_t, Steam_GetWorkshopModCount, void );
DECLARE_TYPEDEF_METHOD( void, Steam_RefreshWorkshopMods, void );
DECLARE_TYPEDEF_METHOD( struct steam_workshop_publish_result_s, Steam_PublishLocalMod, int modIndex, const struct steam_workshop_publish_s *params );

#undef DECLARE_TYPEDEF_METHOD

struct steam_import_s {
	STEAMSHIM_dispatchFn STEAMSHIM_dispatch;
	STEAMSHIM_sendRPCFn STEAMSHIM_sendRPC;
	STEAMSHIM_sendEVTFn STEAMSHIM_sendEVT;
	STEAMSHIM_waitDispatchSyncFn STEAMSHIM_waitDispatchSync;
	STEAMSHIM_subscribeEventFn STEAMSHIM_subscribeEvent;
	STEAMSHIM_unsubscribeEventFn STEAMSHIM_unsubscribeEvent;
	STEAMSHIM_activeFn STEAMSHIM_active;
	Steam_GetWorkshopModsFn Steam_GetWorkshopMods;
	Steam_GetWorkshopModCountFn Steam_GetWorkshopModCount;
	Steam_RefreshWorkshopModsFn Steam_RefreshWorkshopMods;
	Steam_PublishLocalModFn Steam_PublishLocalMod;
};
#define DECLARE_STEAM_STRUCT()                                                                                                    \
	{ STEAMSHIM_dispatch,		  STEAMSHIM_sendRPC, STEAMSHIM_sendEVT,		STEAMSHIM_waitDispatchSync, STEAMSHIM_subscribeEvent, \
	  STEAMSHIM_unsubscribeEvent, STEAMSHIM_active,	 Steam_GetWorkshopMods, Steam_GetWorkshopModCount, Steam_RefreshWorkshopMods, Steam_PublishLocalMod };

#ifdef STEAM_DEFINE_INTERFACE_IMPL
static struct steam_import_s steam_import;
static inline void Q_ImportSteamModule( const struct steam_import_s *imp )
{
	steam_import = *imp;
}
int STEAMSHIM_dispatch()
{
	return steam_import.STEAMSHIM_dispatch();
}
int STEAMSHIM_sendRPC( void *req, uint32_t size, void *self, STEAMSHIM_rpc_handle rpc, uint32_t *syncIndex )
{
	return steam_import.STEAMSHIM_sendRPC( req, size, self, rpc, syncIndex );
}
int STEAMSHIM_sendEVT( void *packet, uint32_t size )
{
	return steam_import.STEAMSHIM_sendEVT( packet, size );
}
int STEAMSHIM_waitDispatchSync( uint32_t syncIndex )
{
	return steam_import.STEAMSHIM_waitDispatchSync( syncIndex );
} // wait on the dispatch loop
void STEAMSHIM_subscribeEvent( uint32_t id, void *self, STEAMSHIM_evt_handle evt )
{
	return steam_import.STEAMSHIM_subscribeEvent( id, self, evt );
} // wait on the dispatch loop
void STEAMSHIM_unsubscribeEvent( uint32_t id, STEAMSHIM_evt_handle evt )
{
	return steam_import.STEAMSHIM_unsubscribeEvent( id, evt );
} // wait on the dispatch loop
bool STEAMSHIM_active()
{
	return steam_import.STEAMSHIM_active();
} // wait on the dispatch loop
const struct steam_workshop_mod_s *Steam_GetWorkshopMods()
{
	return steam_import.Steam_GetWorkshopMods();
}
size_t Steam_GetWorkshopModCount()
{
	return steam_import.Steam_GetWorkshopModCount();
}
void Steam_RefreshWorkshopMods()
{
	steam_import.Steam_RefreshWorkshopMods();
}
struct steam_workshop_publish_result_s Steam_PublishLocalMod( int modIndex, const struct steam_workshop_publish_s *params )
{
	return steam_import.Steam_PublishLocalMod( modIndex, params );
}
#endif
#endif
