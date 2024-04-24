#include "../qcommon/qcommon.h"
#include "../qcommon/steam.h"
#include "client.h"
#include "../steamshim/src/parent/parent.h"
#include "../steamshim/src/packet_utils.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>


static const SteamshimEvent* blockOnEvent(SteamshimEventType type){
	return NULL;
}

/*
* Steam_AdvertiseGame
*/
void Steam_AdvertiseGame( const uint8_t *ip, unsigned short port, uint32_t* syncToken)
{
	char rpc_connect_packet[sizeof(struct buffer_rpc_s) + 256];
	struct buffer_rpc_s* req = (struct buffer_rpc_s*)rpc_connect_packet;
	req->cmd = RPC_SET_RICH_PRESENCE; 
	char connectstr[64] = {0};

	if (port) {
		snprintf( connectstr,sizeof connectstr, "%d.%d.%d.%d:%i",ip[0],ip[1],ip[2],ip[3],port);
	} 	
	const char* pairs[] = {
		"connect",
		connectstr
	};
	const size_t bufferLen = pack_cstr_null_terminated( (char *)req->buf, 256, pairs, 2 );
	STEAMSHIM_sendRPC(req, bufferLen + sizeof(struct buffer_rpc_s), NULL, NULL, syncToken);
}
