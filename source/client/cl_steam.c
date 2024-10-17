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
void Steam_AdvertiseGame( netadr_t *addr, uint32_t* syncToken)
{
	char rpc_connect_packet[sizeof(struct buffer_rpc_s) + 256];
	struct buffer_rpc_s* req = (struct buffer_rpc_s*)rpc_connect_packet;
	req->cmd = RPC_SET_RICH_PRESENCE; 

	const char* pairs[] = {
		"connect",
		addr != NULL ? NET_AddressToString(addr) : "",
	};
	const size_t bufferLen = pack_cstr_null_terminated( (char *)req->buf, 256, pairs, 2 );
	STEAMSHIM_sendRPC(req, bufferLen + sizeof(struct buffer_rpc_s), NULL, NULL, syncToken);
}
