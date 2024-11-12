#pragma once

#include "steam/isteamfriends.h"
#include "steam/isteamuser.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"
class ServerBrowser : public ISteamMatchmakingServerListResponse
{
public:
	~ServerBrowser();
	ServerBrowser();

	void RefreshInternetServers();

	void RunFrame();
	void ServerResponded( HServerListRequest hReq, int iServer );
	void ServerFailedToRespond( HServerListRequest hReq, int iServer );
	void RefreshComplete( HServerListRequest hReq, EMatchMakingServerResponse response );

private:
	bool requestingServers;
	HServerListRequest serverListRequest;
};
