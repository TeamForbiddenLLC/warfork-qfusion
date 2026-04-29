/*
Copyright (C) 2014 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "steam.h"
#include "../extern/stb/stb_ds.h"
#include "../qcommon/qcommon.h"
#include "../steamshim/src/parent/parent.h"
#include "../steamshim/src/mod_steam.h"
#include "cvar.h"
#include "sys_fs.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

cvar_t *steam_debug;
static struct steam_workshop_mod_s *steam_workshop_mods;
static char *Steam_CopyString( const char *in )
{
	int size;
	char *out;

	size = sizeof( char ) * ( strlen( in ) + 1 );
	out = (char *)malloc( size );
	Q_strncpyz( out, in, size );

	return out;
}

static struct steam_workshop_mod_s *__UpsertWorkshopMod( uint64_t id )
{
	for( size_t i = 0; i < arrlen( steam_workshop_mods ); i++ ) {
		if( steam_workshop_mods[i].workshop_id == id ) {
			return &steam_workshop_mods[i];
		}
	}
	struct steam_workshop_mod_s mod = { 0 };
	mod.workshop_id = id;
	arrput( steam_workshop_mods, mod );
	return &steam_workshop_mods[arrlen( steam_workshop_mods ) - 1];
}

static void __RemoveWorkshopMod( uint64_t id )
{
	for( size_t i = 0; i < arrlen( steam_workshop_mods ); i++ ) {
		if( steam_workshop_mods[i].workshop_id != id )
			continue;
		if( steam_workshop_mods[i].fs_search_path ) {
			FS_UnRegisterModPath( steam_workshop_mods[i].fs_search_path );
			steam_workshop_mods[i].fs_search_path = NULL;
		}
		free( (void *)steam_workshop_mods[i].local_path );
		free( (void *)steam_workshop_mods[i].path );
		free( (void *)steam_workshop_mods[i].title );
		free( (void *)steam_workshop_mods[i].description );
		free( (void *)steam_workshop_mods[i].tags );
		free( (void *)steam_workshop_mods[i].preview_url );
		arrdel( steam_workshop_mods, i );
		return;
	}
}

static void __RefreshModPath( struct steam_workshop_mod_s *mod ) {
	if( mod->fs_search_path ) {
		FS_UnRegisterModPath( mod->fs_search_path );
		mod->fs_search_path = NULL;
	}
	const char *active_path = mod->local_path ? mod->local_path : mod->path;
	if( active_path ) {
		mod->fs_search_path = FS_RegisterModPath( active_path );
	}
}

static void Steam_WorkshopInstallInfoCallback( void *self, struct steam_rpc_pkt_s *pkt )
{
	struct steam_workshop_install_info_s *info = &pkt->workshop_install_info;
	struct steam_workshop_mod_s *mod = __UpsertWorkshopMod( info->workshop_id );
	if( !info->success ) {
		return;
	}

	if( mod->path ) {
		free( (void *)mod->path );
	}
	mod->path = Steam_CopyString( pkt->workshop_install_info.folder );
	mod->is_remote = true;
	__RefreshModPath( mod );
}

static void Steam_WorkshopRefreshCallback( void *self, struct steam_rpc_pkt_s *pkt )
{
	struct workshop_subscribed_items_recv_s *recv = &pkt->workshop_subscribed_items;

	for( uint32_t i = 0; i < recv->num_ids; i++ ) {
		__UpsertWorkshopMod( recv->ids[i] );
	}

	if( recv->num_ids > 0 ) {
		size_t req_size = sizeof( struct steam_workshop_req_rpcs_s ) + sizeof( uint64_t ) * recv->num_ids;
		struct steam_workshop_req_rpcs_s *request = (struct steam_workshop_req_rpcs_s *)malloc( req_size );
		request->cmd = RPC_WORKSHOP_QUERY_ITEM_DETAILS;
		request->num_ids = recv->num_ids;
		for( uint32_t i = 0; i < recv->num_ids; i++ ) {
			request->workshop_ids[i] = recv->ids[i];
		}
		STEAMSHIM_sendRPC( request, req_size, NULL, NULL, NULL );
		free( request );
	}
}

static void Steam_EVT_WorkshopItemSubscribed( void *self, struct steam_evt_pkt_s *pkt )
{
	if( pkt->workshop_item.app_id != APP_STEAMID )
		return;

	__UpsertWorkshopMod( pkt->workshop_item.workshop_id );
}

static void Steam_EVT_WorkshopItemUnSubscribed( void *self, struct steam_evt_pkt_s *pkt )
{
	if( pkt->workshop_item.app_id != APP_STEAMID ) {
		return;
	}

	__RemoveWorkshopMod( pkt->workshop_item.workshop_id );
}

static void Steam_EVT_WorkshopItemInstalled( void *self, struct steam_evt_pkt_s *pkt )
{
	if( pkt->workshop_item.app_id != APP_STEAMID ) {
		return;
	}

	struct steam_workshop_mod_s *mod = __UpsertWorkshopMod( pkt->workshop_item.workshop_id );
	struct steam_workshop_req_rpc_s request;
	request.cmd = RPC_WORKSHOP_INSTALLED_INFO;
	request.workshop_id = mod->workshop_id;
	STEAMSHIM_sendRPC( &request, sizeof( request ), NULL, Steam_WorkshopInstallInfoCallback, NULL );

	char details_request_buf[sizeof( struct steam_workshop_req_rpcs_s ) + sizeof( uint64_t )];
	struct steam_workshop_req_rpcs_s *details_req = (struct steam_workshop_req_rpcs_s *)details_request_buf;
	memset( details_request_buf, 0, sizeof( details_request_buf ) );
	details_req->cmd = RPC_WORKSHOP_QUERY_ITEM_DETAILS;
	details_req->num_ids = 1;
	details_req->workshop_ids[0] = mod->workshop_id;
	STEAMSHIM_sendRPC( details_req, sizeof( details_request_buf ), NULL, NULL, NULL );
}

static void Steam_EVT_WorkshopDetail( void *self, struct steam_evt_pkt_s *pkt )
{
	struct workshop_item_details_evt_s *evt = &pkt->workshop_item_details;
	if( !evt->success ) {
		return;
	}

	struct steam_workshop_mod_s *mod = __UpsertWorkshopMod( evt->workshop_id );

	free( (void *)mod->title );
	free( (void *)mod->description );
	free( (void *)mod->tags );
	free( (void *)mod->preview_url );


	const char *buf = evt->buffer;
	mod->title = Steam_CopyString( buf );
	buf += evt->title_len + 1;
	mod->description = Steam_CopyString( buf );
	buf += evt->description_len + 1;
	mod->tags = Steam_CopyString( buf );
	buf += evt->tags_len + 1;
	mod->preview_url = Steam_CopyString( buf );

	mod->votes_up   = evt->votes_up;
	mod->votes_down = evt->votes_down;
	mod->score      = evt->score;
	mod->item_state = evt->item_state;
	mod->visibility = evt->visibility;
	mod->time_updated = evt->time_updated;
}

void Steam_RefreshWorkshopMods( void )
{
	struct steam_workshop_list_rpc_s request;
	request.cmd = RPC_WORKSHOP_REFRESH_SUBSCRIBED_ITEMS;
	STEAMSHIM_sendRPC( &request, sizeof( request ), NULL, Steam_WorkshopRefreshCallback, NULL );
}

/*
 * Steam_ScanLocalMods
 *
 * Scans <writedir>/mods/ for publishable mod folders.  Each immediate
 * subdirectory is treated as one mod.  If the folder contains a
 * .steam/workshop_id file the stored uint64 is used as the existing
 * Workshop item ID (update flow); otherwise workshop_id is 0 (create flow).
 * Results are appended to steam_workshop_mods with is_local = true.
 */
static void Steam_ScanLocalMods( void )
{
	const char *writedir = FS_WriteDirectory();

	if( !writedir )
		return;

	char modsdir[1024];
	Q_snprintfz( modsdir, sizeof( modsdir ), "%s/mods/*", writedir );

	uint64_t* workshop_ids = NULL;
	const char *entry = Sys_FS_FindFirst( modsdir, SFF_SUBDIR, SFF_HIDDEN | SFF_SYSTEM );
	while( entry ) {
		// strip trailing slash added by FindFirst for directories
		size_t len = strlen( entry );
		char modpath[1024];
		if( len > 0 && entry[len - 1] == '/' )
			Q_snprintfz( modpath, sizeof( modpath ), "%.*s", (int)( len - 1 ), entry );
		else
			Q_snprintfz( modpath, sizeof( modpath ), "%s", entry );

		// derive mod name from last path component
		const char *name = strrchr( modpath, '/' );
		name = name ? name + 1 : modpath;

		// try to read an existing Workshop ID from .steam/workshop_id
		uint64_t workshop_id = 0;
		char idpath[1024];
		Q_snprintfz( idpath, sizeof( idpath ), "%s/workshop_id", modpath );
		FILE *f = Sys_FS_fopen( idpath, "r" );
		if( f ) {
			fscanf( f, "%" SCNu64, &workshop_id );
			fclose( f );
		}

		struct steam_workshop_mod_s *mod;
		if( workshop_id > 0 ) {
			stbds_arrpush( workshop_ids, workshop_id );
			mod = __UpsertWorkshopMod( workshop_id );
		} else {
			struct steam_workshop_mod_s new_mod = { 0 };
			arrput( steam_workshop_mods, new_mod );
			mod = &steam_workshop_mods[arrlen( steam_workshop_mods ) - 1];
		}

		mod->is_local = true;
		mod->local_path = Steam_CopyString( modpath );
		if( !mod->name )
			mod->name = Steam_CopyString( name );
		__RefreshModPath( mod );

		entry = Sys_FS_FindNext( SFF_SUBDIR, SFF_HIDDEN | SFF_SYSTEM );
	}
	const size_t pkt_len = sizeof(struct steam_workshop_req_rpcs_s) + (stbds_arrlen(workshop_ids) *  sizeof(uint64_t));
	struct steam_workshop_req_rpcs_s* req = malloc(pkt_len);
	req->cmd = RPC_WORKSHOP_QUERY_ITEM_DETAILS;
	req->num_ids = stbds_arrlen(workshop_ids);
	for(size_t i = 0; i < stbds_arrlen(workshop_ids); i++) {
		req->workshop_ids[i] = workshop_ids[i];
	}
	STEAMSHIM_sendRPC( req, pkt_len, NULL, NULL, NULL );
	free(req);
	stbds_arrfree(workshop_ids);
	Sys_FS_FindClose();
}

// ---------------------------------------------------------------------------
// Callbacks for synchronous RPC round-trips
// ---------------------------------------------------------------------------
typedef struct {
	uint32_t result;
	uint64_t fileId;
} __publish_item_result_t;
typedef struct {
	uint64_t handle;
	bool success;
} __publish_update_t;
typedef struct {
	bool success;
} __publish_ok_t;

static void __on_create_item( __publish_item_result_t *self, struct steam_rpc_pkt_s *rec )
{
	self->result = rec->create_item_recv.result;
	self->fileId = rec->create_item_recv.file_id;
}
static void __on_submit_item( __publish_item_result_t *self, struct steam_rpc_pkt_s *rec )
{
	self->result = rec->submit_item_result.result;
	self->fileId = rec->submit_item_result.file_id;
}
static void __on_begin_update( __publish_update_t *self, struct steam_rpc_pkt_s *rec )
{
	self->handle = rec->workshop_start_item_update_recv.handle;
	self->success = rec->workshop_start_item_update_recv.success;
}
static void __on_success( __publish_ok_t *self, struct steam_rpc_pkt_s *rec )
{
	self->success = rec->success.success;
}

struct steam_workshop_publish_result_s Steam_PublishLocalMod( int modIndex, const struct steam_workshop_publish_s *params )
{
	struct steam_workshop_publish_result_s res = { 0 };

	if( !STEAMSHIM_active() ) {
		res.res = STEAM_PUBLISH_ERR_STEAM_UNAVAILABLE;
		return res;
	}
	if( !params->title || params->title[0] == '\0' ) {
		res.res = STEAM_PUBLISH_ERR_TITLE_REQUIRED;
		return res;
	}

	const struct steam_workshop_mod_s *mods = Steam_GetWorkshopMods();
	size_t count = Steam_GetWorkshopModCount();
	if( modIndex < 0 || (size_t)modIndex >= count || !mods[modIndex].is_local || !mods[modIndex].local_path ) {
		res.res = STEAM_PUBLISH_ERR_INVALID_MOD;
		return res;
	}
	const char *contentPath = mods[modIndex].local_path;

	// Check for preview.png
	char previewPath[1024];
	Q_snprintfz( previewPath, sizeof( previewPath ), "%s/preview.png", contentPath );
	bool hasPreview;
	{
		FILE *f = Sys_FS_fopen( previewPath, "rb" );
		hasPreview = ( f != NULL );
		if( f )
			fclose( f );
	}

	uint64_t fileId = mods[modIndex].workshop_id;
	res.is_new_item = ( fileId == 0 );

	// Create a new Workshop item if this is our first submission
	if( res.is_new_item ) {
		struct steam_rpc_shim_common_s request = { 0 };
		request.cmd = RPC_WORKSHOP_CREATE_ITEM;
		__publish_item_result_t createResult = { 0, 0 };
		uint32_t sync = 0;
		STEAMSHIM_sendRPC( &request, sizeof( request ), &createResult, (STEAMSHIM_rpc_handle)__on_create_item, &sync );
		if( STEAMSHIM_waitDispatchSync( sync ) != 0 ) {
			res.res = STEAM_PUBLISH_ERR_CREATE_FAILED;
			return res;
		}
		if( createResult.result != STEAMSHIM_EResultOK ) {
			res.res = STEAM_PUBLISH_ERR_CREATE_FAILED;
			res.steam_result = createResult.result;
			return res;
		}
		fileId = createResult.fileId;
	}

	// Begin the update transaction to get an UGC handle
	uint64_t updateHandle;
	{
		struct start_item_update_req_s request = { 0 };
		request.cmd = RPC_WORKSHOP_BEGIN_ITEM_UPDATE;
		request.publish_item_file = fileId;
		__publish_update_t updateResult = { 0, false };
		uint32_t sync = 0;
		STEAMSHIM_sendRPC( &request, sizeof( request ), &updateResult, (STEAMSHIM_rpc_handle)__on_begin_update, &sync );
		if( STEAMSHIM_waitDispatchSync( sync ) != 0 || !updateResult.success || updateResult.handle == 0 ) {
			res.res = STEAM_PUBLISH_ERR_UPDATE_FAILED;
			return res;
		}
		updateHandle = updateResult.handle;
	}

// Helper macro: send a buffer-based workshop field RPC; returns on failure
#define SEND_BUFFER_FIELD( _cmd, _handle, _value, _err )                                      \
	{                                                                                         \
		const char *_val = ( _value );                                                        \
		size_t _valLen = strlen( _val ) + 1;                                                  \
		size_t _sz = sizeof( struct buffer_workshop_rpc_s ) + _valLen;                        \
		struct buffer_workshop_rpc_s *_req = (struct buffer_workshop_rpc_s *)malloc( _sz );   \
		if( !_req ) {                                                                         \
			res.res = ( _err );                                                             \
			return res;                                                                       \
		}                                                                                     \
		memset( _req, 0, sizeof( struct buffer_workshop_rpc_s ) );                            \
		_req->cmd = ( _cmd );                                                                 \
		_req->handle = ( _handle );                                                           \
		memcpy( _req->buf, _val, _valLen );                                                   \
		__publish_ok_t _result = { false };                                                   \
		uint32_t _sync = 0;                                                                   \
		STEAMSHIM_sendRPC( _req, _sz, &_result, (STEAMSHIM_rpc_handle)__on_success, &_sync ); \
		bool _ok = STEAMSHIM_waitDispatchSync( _sync ) == 0 && _result.success;               \
		free( _req );                                                                         \
		if( !_ok ) {                                                                          \
			res.res = ( _err );                                                             \
			return res;                                                                       \
		}                                                                                     \
	}

	SEND_BUFFER_FIELD( RPC_WORKSHOP_ITEM_SET_TITLE, updateHandle, params->title, STEAM_PUBLISH_ERR_SET_TITLE );
	SEND_BUFFER_FIELD( RPC_WORKSHOP_ITEM_SET_DESCRIPTION, updateHandle, params->description ? params->description : "", STEAM_PUBLISH_ERR_SET_DESCRIPTION );
	SEND_BUFFER_FIELD( RPC_WORKSHOP_ITEM_SET_ITEM_CONTENT, updateHandle, contentPath, STEAM_PUBLISH_ERR_SET_CONTENT );
	if( hasPreview ) {
		SEND_BUFFER_FIELD( RPC_WORKSHOP_ITEM_SET_PREVIEW, updateHandle, previewPath, STEAM_PUBLISH_ERR_SET_PREVIEW );
	}

#undef SEND_BUFFER_FIELD

	// Set visibility
	{
		struct item_visiblity_req_s request = { 0 };
		request.cmd = RPC_WORKSHOP_ITEM_SET_VISIBILITY;
		request.handle = updateHandle;
		const char *vis = params->visibility ? params->visibility : "private";
		if( Q_stricmp( vis, "public" ) == 0 )
			request.visibility = STEAM_VISIBILITY_PUBLIC;
		else if( Q_stricmp( vis, "friends" ) == 0 )
			request.visibility = STEAM_VISIBILITY_FRIENDS_ONLY;
		else
			request.visibility = STEAM_VISIBILITY_PRIVATE;
		__publish_ok_t result = { false };
		uint32_t sync = 0;
		STEAMSHIM_sendRPC( &request, sizeof( request ), &result, (STEAMSHIM_rpc_handle)__on_success, &sync );
		if( STEAMSHIM_waitDispatchSync( sync ) != 0 || !result.success ) {
			res.res = STEAM_PUBLISH_ERR_SET_VISIBILITY;
			return res;
		}
	}

	// Set tags (comma-separated input → packed NUL-separated buffer)
	if( params->tags != NULL ) {
		
		size_t buf_size = 0;
		buf_size += strlen(params->tags) + 1;
		size_t reqSz = sizeof( struct tags_req_s ) + buf_size;
		struct tags_req_s *req = (struct tags_req_s *)malloc( reqSz );
		memset( req, 0, sizeof( struct tags_req_s ) );
		req->cmd = RPC_WORKSHOP_ITEM_SET_TAGS;
		req->handle = updateHandle;

		size_t cursor = 0;
		for(const char* c = params->tags; *c != '\0'; c++) {
			if (*c == ',') {
				req->buffer[cursor++] = '\0';
				req->num_tags++;
				continue;
			}
			req->buffer[cursor++] = *c;
		}
		__publish_ok_t result = { false };
		uint32_t sync = 0;
		STEAMSHIM_sendRPC( req, reqSz, &result, (STEAMSHIM_rpc_handle)__on_success, &sync );
		bool ok = STEAMSHIM_waitDispatchSync( sync ) == 0 && result.success;
		free( req );
		if( !ok ) {
			res.res = STEAM_PUBLISH_ERR_SET_TAGS;
			return res;
		}
	}

	// Submit the item update
	{
		const char *note = params->change_note ? params->change_note : "";
		size_t noteLen = strlen( note ) + 1;
		size_t reqSz = sizeof( struct buffer_workshop_rpc_s ) + noteLen;
		struct buffer_workshop_rpc_s *req = (struct buffer_workshop_rpc_s *)malloc( reqSz );
		if( !req ) {
			res.res = STEAM_PUBLISH_ERR_SUBMIT_FAILED;
			return res;
		}
		memset( req, 0, sizeof( struct buffer_workshop_rpc_s ) );
		req->cmd = RPC_WORKSHOP_SUBMIT_ITEM;
		req->handle = updateHandle;
		memcpy( req->buf, note, noteLen );
		__publish_item_result_t submitResult = { 0, 0 };
		uint32_t sync = 0;
		STEAMSHIM_sendRPC( req, reqSz, &submitResult, (STEAMSHIM_rpc_handle)__on_submit_item, &sync );
		bool ok = STEAMSHIM_waitDispatchSync( sync ) == 0;
		free( req );
		if( !ok || submitResult.result != STEAMSHIM_EResultOK ) {
			res.res = STEAM_PUBLISH_ERR_SUBMIT_FAILED;
			res.steam_result = submitResult.result;
			return res;
		}
		res.file_id = submitResult.fileId;
	}

	// Persist the new Workshop ID for future update runs
	if( res.is_new_item ) {
		char idpath[1024];
		Q_snprintfz( idpath, sizeof( idpath ), "%s/workshop_id", contentPath );
		FILE *f = Sys_FS_fopen( idpath, "w" );
		if( f ) {
			fprintf( f, "%" PRIu64, res.file_id );
			fclose( f );
		}
	}

	res.res = STEAM_PUBLISH_OK;
	return res;
}

/*
 * Steam_Init
 */
void Steam_Init( void )
{
	steam_debug = Cvar_Get( "steam_debug", "0", 0 );

	SteamshimOptions opts;
	opts.debug = true;//steam_debug->integer;
	opts.runserver = 1;
	opts.runclient = !dedicated->integer;
	int r = STEAMSHIM_init( &opts );
	if( !r ) {
		Com_Printf( "Steam initialization failed.\n" );
		return;
	}

	if( dedicated->integer ) {
		return;
	}

	STEAMSHIM_subscribeEvent( EVT_WORKSHOP_ITEM_SUBSCRIBED, NULL, Steam_EVT_WorkshopItemSubscribed );
	STEAMSHIM_subscribeEvent( EVT_WORKSHOP_ITEM_UNSUBSCRIBED, NULL, Steam_EVT_WorkshopItemUnSubscribed );
	STEAMSHIM_subscribeEvent( EVT_WORKSHOP_ITEM_INSTALLED, NULL, Steam_EVT_WorkshopItemInstalled );
	STEAMSHIM_subscribeEvent( EVT_WORKSHOP_DETAIL, NULL, Steam_EVT_WorkshopDetail );

	Steam_ScanLocalMods();
	Steam_RefreshWorkshopMods();
}

/*
 * Steam_Shutdown
 */
void Steam_Shutdown( void )
{
	STEAMSHIM_unsubscribeEvent( EVT_WORKSHOP_ITEM_SUBSCRIBED, Steam_EVT_WorkshopItemSubscribed );
	STEAMSHIM_unsubscribeEvent( EVT_WORKSHOP_ITEM_UNSUBSCRIBED, Steam_EVT_WorkshopItemUnSubscribed );
	STEAMSHIM_unsubscribeEvent( EVT_WORKSHOP_ITEM_INSTALLED, Steam_EVT_WorkshopItemInstalled );
	STEAMSHIM_unsubscribeEvent( EVT_WORKSHOP_DETAIL, Steam_EVT_WorkshopDetail );

	for( size_t i = 0; i < arrlen( steam_workshop_mods ); i++ ) {
		if( steam_workshop_mods[i].fs_search_path ) {
			FS_UnRegisterModPath( steam_workshop_mods[i].fs_search_path );
		}
		free( (void *)steam_workshop_mods[i].local_path );
		free( (void *)steam_workshop_mods[i].path );
		free( (void *)steam_workshop_mods[i].name );
		free( (void *)steam_workshop_mods[i].title );
		free( (void *)steam_workshop_mods[i].description );
		free( (void *)steam_workshop_mods[i].tags );
		free( (void *)steam_workshop_mods[i].preview_url );
	}
	arrfree( steam_workshop_mods );
	steam_workshop_mods = NULL;

	STEAMSHIM_deinit();
}

const struct steam_workshop_mod_s *Steam_GetWorkshopMods( void )
{
	return steam_workshop_mods;
}

size_t Steam_GetWorkshopModCount( void )
{
	return arrlen( steam_workshop_mods );
}
