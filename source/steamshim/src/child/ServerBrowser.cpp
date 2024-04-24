#include "ServerBrowser.h"

#include "../steamshim_types.h"
#include "../steamshim.h"
#include "../steamshim_private.h"
#include "steam/isteamfriends.h"
#include "steam/isteamuser.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"
#include <cstdlib>
#include <stdio.h>


ServerBrowser::ServerBrowser()
{
	requestingServers = false;
	serverListRequest = NULL;
}


ServerBrowser::~ServerBrowser()
{
	if ( serverListRequest )
	{
		SteamMatchmakingServers()->ReleaseRequest( serverListRequest );
		serverListRequest = NULL;
	}
}


void ServerBrowser::RefreshInternetServers()
{
	if ( requestingServers )
		return;

	if ( serverListRequest )
	{
		SteamMatchmakingServers()->ReleaseRequest( serverListRequest );
		serverListRequest = NULL;
	}
	requestingServers = true;


	MatchMakingKeyValuePair_t pFilters[1];
	MatchMakingKeyValuePair_t *pFilter = pFilters;
	
	strncpy( pFilters[ 0 ].m_szKey, "gamedir", sizeof(pFilters[ 0 ].m_szKey) );
	strncpy( pFilters[ 0 ].m_szValue, "warfork", sizeof(pFilters[ 0 ].m_szValue) );

	serverListRequest = SteamMatchmakingServers()->RequestLANServerList( SteamUtils()->GetAppID(), this );
}


void ServerBrowser::ServerResponded( HServerListRequest hReq, int iServer )
{
	gameserveritem_t *server = SteamMatchmakingServers()->GetServerDetails( hReq, iServer );
	if ( server )
	{
		if ( server->m_nAppID == SteamUtils()->GetAppID() )
		{
			PipeBuffer msg;
			msg.WriteByte(EVT_CL_SERVERRECIEVED);

			msg.Transmit();
		}
	}

}

void ServerBrowser::RunFrame() {

}

void ServerBrowser::ServerFailedToRespond( HServerListRequest hReq, int iServer )
{

}

void ServerBrowser::RefreshComplete( HServerListRequest hReq, EMatchMakingServerResponse response ) 
{
	requestingServers = false; 
}

