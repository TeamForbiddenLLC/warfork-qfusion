#ifdef __WASM__

#include "../steamshim/src/parent/parent.h"
#include "../steamshim/src/steamshim_types.h"

int STEAMSHIM_init( SteamshimOptions *options )
{
	(void)options;
	return 0;
}

void STEAMSHIM_deinit( void )
{
}

int STEAMSHIM_pollFd( void )
{
	return -1;
}

int STEAMSHIM_dispatch( void )
{
	return 0;
}

int STEAMSHIM_sendRPC( void *req, uint32_t size, void *self, STEAMSHIM_rpc_handle rpc, uint32_t *syncIndex )
{
	(void)req;
	(void)size;
	(void)self;
	(void)rpc;
	if( syncIndex )
		*syncIndex = 0;
	return -1;
}

int STEAMSHIM_sendEVT( void *packet, uint32_t size )
{
	(void)packet;
	(void)size;
	return -1;
}

int STEAMSHIM_waitDispatchSync( uint32_t syncIndex )
{
	(void)syncIndex;
	return 0;
}

uint32_t STEAMSHIM_currentSync( void )
{
	return 0;
}

void STEAMSHIM_subscribeEvent( uint32_t id, void *self, STEAMSHIM_evt_handle evt )
{
	(void)id;
	(void)self;
	(void)evt;
}

void STEAMSHIM_unsubscribeEvent( uint32_t id, STEAMSHIM_evt_handle evt )
{
	(void)id;
	(void)evt;
}

bool STEAMSHIM_active( void )
{
	return false;
}

#endif // __WASM__
