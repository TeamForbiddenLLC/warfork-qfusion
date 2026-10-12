#ifdef __WASM__

#include "../qcommon/qcommon.h"
#include "../qcommon/wswcurl.h"

#include <emscripten.h>
#include <emscripten/fetch.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

struct wswcurl_req_s {
	char url[4096];
	char proxied[4608];
	char content_type[128];
	wswcurl_read_cb read_cb;
	wswcurl_done_cb done_cb;
	wswcurl_header_cb header_cb;
	void *customp;
	emscripten_fetch_t *fetch;
	int http_status;
	int timeout_ms;
	bool started;
	bool finished;
	bool done_pending;
	struct wswcurl_req_s *next;
};

static wswcurl_req *wasm_curl_list = NULL;

EM_JS( void, wasm_http_origin, ( char *dst, int dstSize ), {
	var origin = ( typeof location !== 'undefined' && location.origin ) ? location.origin : "";
	stringToUTF8( origin, dst, dstSize );
} );

static void Wasm_CurlUnlink( wswcurl_req *req )
{
	wswcurl_req **walk;

	for( walk = &wasm_curl_list; *walk; walk = &( *walk )->next ) {
		if( *walk == req ) {
			*walk = req->next;
			req->next = NULL;
			return;
		}
	}
}

static void Wasm_CurlProxyUrl( const char *url, char *out, size_t outSize )
{
	char origin[256];
	const char *hostpath;

	origin[0] = '\0';
	wasm_http_origin( origin, sizeof( origin ) );

	hostpath = url;
	if( !Q_strnicmp( hostpath, "http://", 7 ) )
		hostpath += 7;
	else if( !Q_strnicmp( hostpath, "https://", 8 ) )
		hostpath += 8;

	(void)origin;
	Q_snprintfz( out, outSize, "/http-proxy/%s", hostpath );
}

static void Wasm_CurlOnSuccess( emscripten_fetch_t *fetch )
{
	wswcurl_req *req = fetch ? (wswcurl_req *)fetch->userData : NULL;

	if( !req )
		return;
	req->http_status = fetch->status ? (int)fetch->status : 200;
	Q_strncpyz( req->content_type, "application/octet-stream", sizeof( req->content_type ) );
	Com_Printf( "[wasm] HTTP %d %u bytes %s\n", req->http_status,
		(unsigned)( fetch->numBytes ), req->url );
	if( req->read_cb && fetch->data && fetch->numBytes )
		req->read_cb( req, fetch->data, (size_t)fetch->numBytes, 100.0f, req->customp );
	req->finished = true;
	req->done_pending = true;
}

static void Wasm_CurlOnError( emscripten_fetch_t *fetch )
{
	wswcurl_req *req = fetch ? (wswcurl_req *)fetch->userData : NULL;

	if( !req )
		return;
	req->http_status = fetch->status ? (int)fetch->status : -1;
	Com_Printf( "[wasm] HTTP error status=%d url=%s\n", req->http_status, req ? req->url : "" );
	req->finished = true;
	req->done_pending = true;
}

void wswcurl_init( void )
{
}

wswcurl_req *wswcurl_create( const char *iface, const char *furl, ... )
{
	wswcurl_req *req;
	va_list argptr;

	(void)iface;
	if( !furl )
		return NULL;

	req = (wswcurl_req *)calloc( 1, sizeof( *req ) );
	if( !req )
		return NULL;

	va_start( argptr, furl );
	Q_vsnprintfz( req->url, sizeof( req->url ), furl, argptr );
	va_end( argptr );

	Wasm_CurlProxyUrl( req->url, req->proxied, sizeof( req->proxied ) );
	req->timeout_ms = 10000;
	req->next = wasm_curl_list;
	wasm_curl_list = req;
	return req;
}

void wswcurl_start( wswcurl_req *req )
{
	emscripten_fetch_attr_t attr;

	if( !req || req->started )
		return;

	emscripten_fetch_attr_init( &attr );
	strcpy( attr.requestMethod, "GET" );
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.timeoutMSecs = req->timeout_ms > 0 ? (uint32_t)req->timeout_ms : 10000;
	attr.onsuccess = Wasm_CurlOnSuccess;
	attr.onerror = Wasm_CurlOnError;
	attr.userData = req;

	req->started = true;
	req->fetch = emscripten_fetch( &attr, req->proxied );
}

size_t wswcurl_getsize( wswcurl_req *req, size_t *rxreceived )
{
	if( rxreceived )
		*rxreceived = 0;
	(void)req;
	return 0;
}

void wswcurl_stream_callbacks( wswcurl_req *req, wswcurl_read_cb read_cb, wswcurl_done_cb done_cb,
	wswcurl_header_cb header_cb, void *customp )
{
	if( !req )
		return;
	req->read_cb = read_cb;
	req->done_cb = done_cb;
	req->header_cb = header_cb;
	req->customp = customp;
}

size_t wswcurl_read( wswcurl_req *req, void *buffer, size_t size )
{
	(void)req;
	(void)buffer;
	(void)size;
	return 0;
}

int wswcurl_perform( void )
{
	wswcurl_req *req, *next;
	int active = 0;

	for( req = wasm_curl_list; req; req = next ) {
		next = req->next;
		if( req->done_pending ) {
			wswcurl_done_cb done = req->done_cb;
			void *customp = req->customp;
			int status = req->http_status;

			req->done_pending = false;
			if( done )
				done( req, status, customp );
			continue;
		}
		if( req->started && !req->finished )
			active++;
	}
	return active;
}

void wswcurl_delete( wswcurl_req *req )
{
	if( !req )
		return;
	Wasm_CurlUnlink( req );
	if( req->fetch ) {
		emscripten_fetch_close( req->fetch );
		req->fetch = NULL;
	}
	free( req );
}

void wswcurl_cleanup( void )
{
	while( wasm_curl_list )
		wswcurl_delete( wasm_curl_list );
}

void wswcurl_set_timeout( wswcurl_req *req, int timeout )
{
	if( req )
		if( !req )
		return;
	if( timeout >= 1000 )
		req->timeout_ms = timeout;
	else if( timeout > 0 )
		req->timeout_ms = timeout * 1000;
	else
		req->timeout_ms = 10000;
}

void wswcurl_set_resume_from( wswcurl_req *req, long resume )
{
	(void)req;
	(void)resume;
}

int wswcurl_header( wswcurl_req *req, const char *key, const char *value, ... )
{
	(void)req;
	(void)key;
	(void)value;
	return 0;
}

int wswcurl_formadd( wswcurl_req *req, const char *field, const char *value, ... )
{
	(void)req;
	(void)field;
	(void)value;
	return 0;
}

int wswcurl_formadd_raw( wswcurl_req *req, const char *field, void *data, size_t size )
{
	(void)req;
	(void)field;
	(void)data;
	(void)size;
	return 0;
}

int wswcurl_set_postfields( wswcurl_req *req, const char *fields, size_t size )
{
	(void)req;
	(void)fields;
	(void)size;
	return 0;
}

void wswcurl_urlencode( const char *src, char *dst, size_t size )
{
	if( !dst || !size )
		return;
	if( !src ) {
		dst[0] = '\0';
		return;
	}
	Q_strncpyz( dst, src, size );
}

int wswcurl_isvalidhandle( wswcurl_req *req )
{
	wswcurl_req *walk;

	for( walk = wasm_curl_list; walk; walk = walk->next ) {
		if( walk == req )
			return 1;
	}
	return 0;
}

int wswcurl_tell( wswcurl_req *req )
{
	(void)req;
	return 0;
}

int wswcurl_eof( wswcurl_req *req )
{
	return req ? ( req->finished ? 1 : 0 ) : 1;
}

void wswcurl_ignore_bytes( wswcurl_req *req, size_t nbytes )
{
	(void)req;
	(void)nbytes;
}

const char *wswcurl_get_content_type( wswcurl_req *req )
{
	return req ? req->content_type : "";
}

const char *wswcurl_getip( wswcurl_req *req )
{
	(void)req;
	return "0.0.0.0";
}

const char *wswcurl_errorstr( int status )
{
	(void)status;
	return "WASM HTTP error";
}

const char *wswcurl_get_url( const wswcurl_req *req )
{
	return req ? req->url : "";
}

const char *wswcurl_get_effective_url( wswcurl_req *req )
{
	return req ? req->proxied : "";
}

int wswcurl_get_status( const wswcurl_req *req )
{
	return req ? req->http_status : 0;
}

#endif // __WASM__
