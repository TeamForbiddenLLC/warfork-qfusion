#ifndef QFUSION_WASM_NET_H
#define QFUSION_WASM_NET_H

#include "../qcommon/qcommon.h"

void Sys_WASM_Init( void );
void Sys_WASM_Shutdown( void );

void Sys_WASM_NetInit( void );
void Sys_WASM_NetShutdown( void );

bool Sys_WASM_UDP_OpenSocket( socket_t *sock, const netadr_t *address, bool server );
void Sys_WASM_UDP_CloseSocket( socket_t *socket );
int  Sys_WASM_UDP_GetPacket( const socket_t *socket, netadr_t *address, msg_t *message );
bool Sys_WASM_UDP_SendPacket( const socket_t *socket, const void *data, size_t length, const netadr_t *address );
void Sys_WASM_NetSleep( int msec );
bool Sys_WASM_ResolveHost( const char *host, char *ip, size_t ipSize );

#endif
