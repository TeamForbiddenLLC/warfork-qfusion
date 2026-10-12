#ifdef __WASM__

#include "wasm_net.h"
#include "../qcommon/sys_net.h"

#include <emscripten.h>
#include <emscripten/websocket.h>
#include <string.h>
#include <unistd.h>

#define WASM_NET_QUEUE 256
#define WASM_FRAME_HDR 4
#define WASM_ADDR_HDR 6

typedef struct {
	int length;
	netadr_t address;
	uint8_t data[MAX_MSGLEN];
} wasm_packet_t;

static EMSCRIPTEN_WEBSOCKET_T wasm_ws = 0;
static bool wasm_ws_open = false;
static bool wasm_ws_connecting = false;
static wasm_packet_t wasm_queue[WASM_NET_QUEUE];
static int wasm_qhead = 0;
static int wasm_qtail = 0;
static uint8_t wasm_rxbuf[MAX_MSGLEN + WASM_FRAME_HDR];
static size_t wasm_rxlen = 0;
static socket_t *wasm_udp_socket = NULL;

EM_JS( char *, wasm_net_default_url, ( void ), {
	var url = ( typeof Module !== 'undefined' && Module.websocketUrl ) ? Module.websocketUrl : 'ws://localhost:8080/ws';
	var len = lengthBytesUTF8( url ) + 1;
	var ptr = _malloc( len );
	stringToUTF8( url, ptr, len );
	return ptr;
} );

static void Wasm_NetLog( const char *fmt, ... )
{
	va_list argptr;
	char msg[1024];

	va_start( argptr, fmt );
	Q_vsnprintfz( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );
	Com_Printf( "%s\n", msg );
}

static bool Wasm_QueuePush( const uint8_t *data, int length, const netadr_t *address )
{
	int next;

	if( length <= 0 || length > MAX_MSGLEN )
		return false;

	next = ( wasm_qtail + 1 ) % WASM_NET_QUEUE;
	if( next == wasm_qhead )
		return false;

	wasm_queue[wasm_qtail].length = length;
	wasm_queue[wasm_qtail].address = *address;
	memcpy( wasm_queue[wasm_qtail].data, data, length );
	wasm_qtail = next;
	return true;
}

static int Wasm_QueuePop( uint8_t *data, int maxsize, netadr_t *address )
{
	int length;

	if( wasm_qhead == wasm_qtail )
		return 0;

	length = wasm_queue[wasm_qhead].length;
	if( length > maxsize )
		length = maxsize;
	memcpy( data, wasm_queue[wasm_qhead].data, length );
	if( address )
		*address = wasm_queue[wasm_qhead].address;
	wasm_qhead = ( wasm_qhead + 1 ) % WASM_NET_QUEUE;
	return length;
}

static void Wasm_FeedBytes( const uint8_t *data, size_t length )
{
	netadr_t address;
	size_t packetLen;

	while( length ) {
		size_t space = sizeof( wasm_rxbuf ) - wasm_rxlen;
		size_t copy = length < space ? length : space;
		memcpy( wasm_rxbuf + wasm_rxlen, data, copy );
		wasm_rxlen += copy;
		data += copy;
		length -= copy;

		while( wasm_rxlen >= WASM_FRAME_HDR ) {
			packetLen = ( (size_t)wasm_rxbuf[0] << 24 ) | ( (size_t)wasm_rxbuf[1] << 16 )
				| ( (size_t)wasm_rxbuf[2] << 8 ) | (size_t)wasm_rxbuf[3];
			if( packetLen < WASM_ADDR_HDR || packetLen > MAX_MSGLEN + WASM_ADDR_HDR ) {
				Wasm_NetLog( "[WS] Invalid framed packet length %zu", packetLen );
				wasm_rxlen = 0;
				break;
			}
			if( wasm_rxlen < WASM_FRAME_HDR + packetLen )
				break;

			memset( &address, 0, sizeof( address ) );
			address.type = NA_IP;
			address.address.ipv4.ip[0] = wasm_rxbuf[WASM_FRAME_HDR + 0];
			address.address.ipv4.ip[1] = wasm_rxbuf[WASM_FRAME_HDR + 1];
			address.address.ipv4.ip[2] = wasm_rxbuf[WASM_FRAME_HDR + 2];
			address.address.ipv4.ip[3] = wasm_rxbuf[WASM_FRAME_HDR + 3];
			NET_SetAddressPort( &address, (unsigned short)( ( wasm_rxbuf[WASM_FRAME_HDR + 4] << 8 )
				| wasm_rxbuf[WASM_FRAME_HDR + 5] ) );
			if( !Wasm_QueuePush( wasm_rxbuf + WASM_FRAME_HDR + WASM_ADDR_HDR,
				(int)( packetLen - WASM_ADDR_HDR ), &address ) )
				Wasm_NetLog( "[WS] Receive queue full, dropped %zu bytes", packetLen );

			wasm_rxlen -= WASM_FRAME_HDR + packetLen;
			if( wasm_rxlen )
				memmove( wasm_rxbuf, wasm_rxbuf + WASM_FRAME_HDR + packetLen, wasm_rxlen );
		}

		if( copy == space && length )
			break;
	}
}

static EM_BOOL Wasm_OnOpen( int type, const EmscriptenWebSocketOpenEvent *event, void *userData )
{
	(void)type;
	(void)event;
	(void)userData;
	wasm_ws_open = true;
	wasm_ws_connecting = false;
	Wasm_NetLog( "[WS] Client connected" );
	return EM_TRUE;
}

static EM_BOOL Wasm_OnClose( int type, const EmscriptenWebSocketCloseEvent *event, void *userData )
{
	(void)type;
	(void)userData;
	wasm_ws_open = false;
	wasm_ws_connecting = false;
	Wasm_NetLog( "[WS] Client disconnected (code %d)", event ? event->code : 0 );
	return EM_TRUE;
}

static EM_BOOL Wasm_OnError( int type, const EmscriptenWebSocketErrorEvent *event, void *userData )
{
	(void)type;
	(void)event;
	(void)userData;
	wasm_ws_connecting = false;
	Wasm_NetLog( "[WS] Error" );
	return EM_TRUE;
}

static EM_BOOL Wasm_OnMessage( int type, const EmscriptenWebSocketMessageEvent *event, void *userData )
{
	(void)type;
	(void)userData;
	if( !event || event->isText || !event->data || event->numBytes <= 0 )
		return EM_TRUE;
	Wasm_FeedBytes( event->data, (size_t)event->numBytes );
	return EM_TRUE;
}

static void Wasm_Connect( void )
{
	EmscriptenWebSocketCreateAttributes attr;
	char *url;

	if( wasm_ws_open || wasm_ws_connecting )
		return;
	if( !emscripten_websocket_is_supported() ) {
		Wasm_NetLog( "[WS] WebSocket is not supported in this browser" );
		return;
	}

	url = wasm_net_default_url();
	emscripten_websocket_init_create_attributes( &attr );
	attr.url = url;
	attr.protocols = "binary";
	attr.createOnMainThread = true;

	wasm_ws = emscripten_websocket_new( &attr );
	free( url );
	if( wasm_ws <= 0 ) {
		Wasm_NetLog( "[WS] emscripten_websocket_new failed" );
		wasm_ws = 0;
		return;
	}

	wasm_ws_connecting = true;
	emscripten_websocket_set_onopen_callback( wasm_ws, NULL, Wasm_OnOpen );
	emscripten_websocket_set_onclose_callback( wasm_ws, NULL, Wasm_OnClose );
	emscripten_websocket_set_onerror_callback( wasm_ws, NULL, Wasm_OnError );
	emscripten_websocket_set_onmessage_callback( wasm_ws, NULL, Wasm_OnMessage );
	Wasm_NetLog( "[WS] Connecting" );
}

void Sys_WASM_Init( void )
{
	if( chdir( "/" ) != 0 ) {
		Com_Printf( "Sys_WASM_Init: chdir(\"/\") failed\n" );
	}
	Com_Printf( "Sys_WASM_Init: browser / Emscripten platform\n" );
}

void Sys_WASM_Shutdown( void )
{
	Sys_WASM_NetShutdown();
}

void Sys_WASM_NetInit( void )
{
	wasm_qhead = wasm_qtail = 0;
	wasm_rxlen = 0;
	Wasm_Connect();
}

void Sys_WASM_NetShutdown( void )
{
	if( wasm_ws > 0 ) {
		emscripten_websocket_close( wasm_ws, 1000, "shutdown" );
		emscripten_websocket_delete( wasm_ws );
		wasm_ws = 0;
	}
	wasm_ws_open = false;
	wasm_ws_connecting = false;
	wasm_udp_socket = NULL;
	wasm_qhead = wasm_qtail = 0;
	wasm_rxlen = 0;
}

bool Sys_WASM_UDP_OpenSocket( socket_t *sock, const netadr_t *address, bool server )
{
	static int fakeHandle = 1;

	assert( sock && !sock->open );
	assert( address );

	Wasm_Connect();

	sock->open = true;
	sock->type = SOCKET_UDP;
	sock->address = *address;
	sock->server = server;
	sock->handle = fakeHandle++;
	if( !wasm_udp_socket )
		wasm_udp_socket = sock;
	return true;
}

void Sys_WASM_UDP_CloseSocket( socket_t *socket )
{
	if( !socket->open )
		return;
	if( wasm_udp_socket == socket )
		wasm_udp_socket = NULL;
	socket->handle = 0;
	socket->open = false;
}

int Sys_WASM_UDP_GetPacket( const socket_t *socket, netadr_t *address, msg_t *message )
{
	int ret;

	(void)socket;
	ret = Wasm_QueuePop( message->data, message->maxsize, address );
	if( !ret )
		return 0;
	message->readcount = 0;
	message->cursize = ret;
	return 1;
}

bool Sys_WASM_UDP_SendPacket( const socket_t *socket, const void *data, size_t length, const netadr_t *address )
{
	uint8_t frame[MAX_MSGLEN + WASM_FRAME_HDR + WASM_ADDR_HDR];
	size_t packetLen;
	unsigned short port;
	EMSCRIPTEN_RESULT result;

	(void)socket;

	if( !wasm_ws_open || wasm_ws <= 0 ) {
		Wasm_Connect();
		NET_SetErrorString( "WebSocket not connected" );
		return false;
	}
	if( length == 0 || length > MAX_MSGLEN ) {
		NET_SetErrorString( "Oversized packet" );
		return false;
	}
	if( !address || address->type != NA_IP ) {
		NET_SetErrorString( "Unsupported address type" );
		return false;
	}

	packetLen = length + WASM_ADDR_HDR;
	port = NET_GetAddressPort( address );
	frame[0] = (uint8_t)( ( packetLen >> 24 ) & 0xff );
	frame[1] = (uint8_t)( ( packetLen >> 16 ) & 0xff );
	frame[2] = (uint8_t)( ( packetLen >> 8 ) & 0xff );
	frame[3] = (uint8_t)( packetLen & 0xff );
	frame[4] = address->address.ipv4.ip[0];
	frame[5] = address->address.ipv4.ip[1];
	frame[6] = address->address.ipv4.ip[2];
	frame[7] = address->address.ipv4.ip[3];
	frame[8] = (uint8_t)( ( port >> 8 ) & 0xff );
	frame[9] = (uint8_t)( port & 0xff );
	memcpy( frame + WASM_FRAME_HDR + WASM_ADDR_HDR, data, length );

	result = emscripten_websocket_send_binary( wasm_ws, frame, (uint32_t)( packetLen + WASM_FRAME_HDR ) );
	if( result != EMSCRIPTEN_RESULT_SUCCESS ) {
		NET_SetErrorString( "WebSocket send failed" );
		return false;
	}
	return true;
}

void Sys_WASM_NetSleep( int msec )
{
	(void)msec;
}

EM_JS( int, wasm_dns_lookup, ( const char *host, char *ip, int ipSize ), {
	var name = UTF8ToString(host);
	if( !Module._dnsCache ) {
		Module._dnsCache = {};
	}
	if( Module._dnsCache[name] ) {
		stringToUTF8(Module._dnsCache[name], ip, ipSize);
		return 1;
	}
	try {
		var xhr = new XMLHttpRequest();
		xhr.open("GET", "/dns-lookup?host=" + encodeURIComponent(name), false);
		xhr.send(null);
		if( xhr.status === 200 && xhr.responseText ) {
			var addr = String(xhr.responseText).trim();
			if( addr && addr.indexOf(".") > 0 ) {
				Module._dnsCache[name] = addr;
				stringToUTF8(addr, ip, ipSize);
				return 1;
			}
		}
	} catch (err) {
		console.warn("dns-lookup failed", name, err);
	}
	return 0;
} );

bool Sys_WASM_ResolveHost( const char *host, char *ip, size_t ipSize )
{
	if( !host || !host[0] || !ip || ipSize < 8 )
		return false;

	ip[0] = '\0';
	return wasm_dns_lookup( host, ip, (int)ipSize ) != 0;
}

void Sys_NET_Init( void )
{
}

void Sys_NET_Shutdown( void )
{
}

net_error_t Sys_NET_GetLastError( void )
{
	return NET_ERR_WOULDBLOCK;
}

void Sys_NET_SocketClose( socket_handle_t handle )
{
	(void)handle;
}

int Sys_NET_SocketIoctl( socket_handle_t handle, long request, ioctl_param_t *param )
{
	(void)handle;
	(void)request;
	(void)param;
	return 0;
}

int64_t Sys_NET_SendFile( socket_handle_t handle, int fileno, size_t offset, size_t count )
{
	(void)handle;
	(void)fileno;
	(void)offset;
	(void)count;
	return -1;
}

#endif // __WASM__
