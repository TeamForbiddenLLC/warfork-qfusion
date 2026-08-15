#include "../qcommon/qcommon.h"
#include "../qcommon/steam.h"
#include "../steamshim/src/steamshim_types.h"
#include "./../steamshim/src/mod_steam.h"

struct authsessioncb_self {
	int result;
};

static void BeginAuthSessionCB(struct authsessioncb_self *self, struct steam_result_recv_s *rec) {
	self->result = rec->result;
}

int Steam_BeginAuthSession(uint64_t steamid, SteamAuthTicket_t *ticket){
	struct begin_auth_session_req_s s;

	// pcbTicket originates from the network; the caller bounds it, but this struct is
	// also fed from the shim, so validate here too rather than trusting the field
	if( ticket->pcbTicket < 0 || ticket->pcbTicket > (long long)sizeof( s.authTicket ) ||
		ticket->pcbTicket > (long long)sizeof( ticket->pTicket ) )
		return -1;

	s.steamID = steamid;
	s.cmd = RPC_BEGIN_AUTH_SESSION;
	s.cbAuthTicket = ticket->pcbTicket;
	memcpy(s.authTicket, ticket->pTicket, (size_t)ticket->pcbTicket);

	uint32_t sync;
	struct authsessioncb_self self;
	self.result = -1;

	STEAMSHIM_sendRPC(&s, sizeof s, &self, (STEAMSHIM_rpc_handle)BeginAuthSessionCB, &sync);
	STEAMSHIM_waitDispatchSync(sync);

	return self.result;
}


void Steam_EndAuthSession(uint64_t steamid) {
	struct steam_id_rpc_s s;
	s.id = steamid;
	s.cmd = RPC_END_AUTH_SESSION;

	STEAMSHIM_sendRPC(&s, sizeof s, NULL, NULL, NULL);
}
