#ifndef _STEAM_MODULE_H
#define _STEAM_MODULE_H

#include "./steamshim_types.h"

#ifdef __cplusplus
#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
  extern "C" { \
	  ret name( __VA_ARGS__ ); \
  }
#else
#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
	  ret name( __VA_ARGS__ );
#endif

DECLARE_TYPEDEF_METHOD( int, STEAMSHIM_dispatch );
DECLARE_TYPEDEF_METHOD( int, STEAMSHIM_sendRPC, void *req, uint32_t size, void *self, STEAMSHIM_rpc_handle rpc, uint32_t *syncIndex );
DECLARE_TYPEDEF_METHOD( struct steam_rpc_pkt_s *, STEAMSHIM_sendRPCSync, void *req, uint32_t size );
DECLARE_TYPEDEF_METHOD( int, STEAMSHIM_waitDispatchSync, uint32_t syncIndex ); // wait on the dispatch loop does not trigger steam callbacks
DECLARE_TYPEDEF_METHOD( void, STEAMSHIM_subscribeEvent, uint32_t id, void *self, STEAMSHIM_evt_handle evt );
DECLARE_TYPEDEF_METHOD( void, STEAMSHIM_unsubscribeEvent, uint32_t id, STEAMSHIM_evt_handle evt );
DECLARE_TYPEDEF_METHOD( bool, STEAMSHIM_active );

#undef DECLARE_TYPEDEF_METHOD

struct steam_import_s {
	STEAMSHIM_dispatchFn STEAMSHIM_dispatch;
	STEAMSHIM_sendRPCFn STEAMSHIM_sendRPC;
	STEAMSHIM_sendRPCSyncFn STEAMSHIM_sendRPCSync;
	STEAMSHIM_waitDispatchSyncFn STEAMSHIM_waitDispatchSync;
	STEAMSHIM_subscribeEventFn STEAMSHIM_subscribeEvent;
	STEAMSHIM_unsubscribeEventFn STEAMSHIM_unsubscribeEvent;
	STEAMSHIM_activeFn STEAMSHIM_active;
};
#define DECLARE_STEAM_STRUCT() { \
	STEAMSHIM_dispatch, \
	STEAMSHIM_sendRPC, \
	STEAMSHIM_sendRPCSync, \
	STEAMSHIM_waitDispatchSync, \
	STEAMSHIM_subscribeEvent, \
	STEAMSHIM_unsubscribeEvent, \
	STEAMSHIM_active \
};

#ifdef STEAM_DEFINE_INTERFACE_IMPL
static struct steam_import_s steam_import;
static inline void Q_ImportSteamModule( const struct steam_import_s *imp )
{
	steam_import = *imp;
}
int STEAMSHIM_dispatch() { return steam_import.STEAMSHIM_dispatch();}
int STEAMSHIM_sendRPC( void *req, uint32_t size, void *self, STEAMSHIM_rpc_handle rpc, uint32_t *syncIndex ) { return steam_import.STEAMSHIM_sendRPC(req, size, self, rpc, syncIndex);}
struct steam_rpc_pkt_s *STEAMSHIM_sendRPCSync( void *req, uint32_t size ) { return steam_import.STEAMSHIM_sendRPCSync(req, size); }
int STEAMSHIM_waitDispatchSync( uint32_t syncIndex ){ return steam_import.STEAMSHIM_waitDispatchSync(syncIndex);} // wait on the dispatch loop
void STEAMSHIM_subscribeEvent( uint32_t id, void *self, STEAMSHIM_evt_handle evt ){ return steam_import.STEAMSHIM_subscribeEvent(id, self, evt);} // wait on the dispatch loop
void STEAMSHIM_unsubscribeEvent( uint32_t id, STEAMSHIM_evt_handle evt){ return steam_import.STEAMSHIM_unsubscribeEvent(id, evt);} // wait on the dispatch loop
bool STEAMSHIM_active(){ return steam_import.STEAMSHIM_active();} // wait on the dispatch loop
#endif
#endif
