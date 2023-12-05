/*
Copyright (c) <2023> <coolelectronics>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <cstddef>
#include <cstdint>

#include "parent_private.h"
#include "../steamshim_private.h"
#include "../os.h"
#include "../steamshim.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

int GArgc = 0;
char **GArgv = NULL;

typedef enum ServerType {
    STEAMGAMESERVER,
    STEAMGAMECLIENT,
} ServerType;
static ServerType GServerType;

static ISteamUserStats *GSteamStats = NULL;
static ISteamUtils *GSteamUtils = NULL;
static ISteamUser *GSteamUser = NULL;
static AppId_t GAppID = 0;
static uint64 GUserID = 0;
// static SteamBridge *GSteamBridge = NULL;
static ISteamGameServer *GSteamGameServer = NULL;


static bool processCommand(pipebuff_t cmd, ShimCmd cmdtype, unsigned int len)
{
  #if 1
    if (false) {}
#define PRINTGOTCMD(x) else if (cmdtype && cmdtype == x) printf("Parent got " #x ".\n")
    PRINTGOTCMD(SHIMCMD_BYE);
    // PRINTGOTCMD(SHIMCMD_PUMP);
    PRINTGOTCMD(SHIMCMD_REQUESTSTATS);
    PRINTGOTCMD(SHIMCMD_STORESTATS);
    PRINTGOTCMD(SHIMCMD_SETACHIEVEMENT);
    PRINTGOTCMD(SHIMCMD_GETACHIEVEMENT);
    PRINTGOTCMD(SHIMCMD_RESETSTATS);
    PRINTGOTCMD(SHIMCMD_SETSTATI);
    PRINTGOTCMD(SHIMCMD_GETSTATI);
    PRINTGOTCMD(SHIMCMD_SETSTATF);
    PRINTGOTCMD(SHIMCMD_GETSTATF);
    PRINTGOTCMD(SHIMCMD_REQUESTSTEAMID);
    PRINTGOTCMD(SHIMCMD_REQUESTPERSONANAME);
    PRINTGOTCMD(SHIMCMD_SETRICHPRESENCE);
    PRINTGOTCMD(SHIMCMD_REQUESTAUTHSESSIONTICKET);
    PRINTGOTCMD(SHIMCMD_BEGINAUTHSESSION);
#undef PRINTGOTCMD
    else if (cmdtype != SHIMCMD_PUMP) printf("Parent got unknown shimcmd %d.\n", (int) cmdtype);
#endif

    pipebuff_t msg;
    switch (cmdtype)
    {
        case SHIMCMD_PUMP:
            if (GServerType == STEAMGAMESERVER)
                SteamGameServer_RunCallbacks();
            else
                SteamAPI_RunCallbacks();
            break;

        case SHIMCMD_BYE:
            // writeBye(fd);
            return false;

        case SHIMCMD_REQUESTSTEAMID:
            {
                unsigned long long id = SteamUser()->GetSteamID().ConvertToUint64();

                msg.WriteByte(SHIMEVENT_STEAMIDRECIEVED);
                msg.WriteLong(id);
                msg.Transmit();
            }
            break;

        case SHIMCMD_REQUESTPERSONANAME:
            {
                const char* name = SteamFriends()->GetPersonaName();

                msg.WriteByte(SHIMEVENT_PERSONANAMERECIEVED);
                msg.WriteData((void*)name, strlen(name));
                msg.Transmit();
            }
            break;

        case SHIMCMD_SETRICHPRESENCE:
            {
                const char *key = cmd.ReadString();
                const char *val = cmd.ReadString();
                SteamFriends()->SetRichPresence(key,val);
            }
            break;
        case SHIMCMD_REQUESTAUTHSESSIONTICKET:
            {
                char pTicket[AUTH_TICKET_MAXSIZE];
                uint32 pcbTicket;
                GSteamUser->GetAuthSessionTicket(pTicket,AUTH_TICKET_MAXSIZE, &pcbTicket);

                msg.WriteByte(SHIMEVENT_AUTHSESSIONTICKETRECIEVED);
                msg.WriteLong(pcbTicket);
                msg.WriteData(pTicket, AUTH_TICKET_MAXSIZE);
                msg.Transmit();
            }
            break;
        case SHIMCMD_BEGINAUTHSESSION:
            {
                uint64 steamID = cmd.ReadLong();
                long long cbAuthTicket = cmd.ReadLong();
                void* pAuthTicket = cmd.ReadData(AUTH_TICKET_MAXSIZE);


                int result = GSteamGameServer->BeginAuthSession(pAuthTicket, cbAuthTicket, steamID);

                msg.WriteByte(SHIMEVENT_AUTHSESSIONVALIDATED);
                msg.WriteInt(result);
                msg.Transmit();
            }
            break;
        case SHIMCMD_ENDAUTHSESSION:
            {
                uint64 steamID = cmd.ReadLong();
                GSteamGameServer->EndAuthSession(steamID);
            }
            break;
    } // switch

    return 0;
}


static void processCommands()
{
  pipebuff_t buf;

  while (1){

    if (!buf.Recieve())
      continue;

    if (buf.hasmsg){
        volatile unsigned int evlen =buf.ReadInt();

        ShimCmd cmd = (ShimCmd)buf.ReadByte();

        processCommand(buf, cmd, evlen);
    }
  }
} 


static bool initSteamworks(PipeType fd)
{
    if (GServerType == STEAMGAMESERVER) {
        if (!SteamGameServer_Init(0, 27015, 0,eServerModeNoAuthentication,"0.0.0.0"))
            return 0;
        GSteamGameServer = SteamGameServer();
        if (!GSteamGameServer)
            return 0;
        
    }else{
        // this can fail for many reasons:
        //  - you forgot a steam_appid.txt in the current working directory.
        //  - you don't have Steam running
        //  - you don't own the game listed in steam_appid.txt
        if (!SteamAPI_Init())
            return 0;

        GSteamStats = SteamUserStats();
        GSteamUtils = SteamUtils();
        GSteamUser = SteamUser();

        GAppID = GSteamUtils ? GSteamUtils->GetAppID() : 0;
	    GUserID = GSteamUser ? GSteamUser->GetSteamID().ConvertToUint64() : 0;
    }
    //
    // GSteamBridge = new SteamBridge(fd);
    return 1;
} 

static bool setEnvironmentVars(PipeType pipeChildRead, PipeType pipeChildWrite)
{
    char buf[64];
    snprintf(buf, sizeof (buf), "%llu", (unsigned long long) pipeChildRead);
    if (!setEnvVar("STEAMSHIM_READHANDLE", buf))
        return false;

    snprintf(buf, sizeof (buf), "%llu", (unsigned long long) pipeChildWrite);
    if (!setEnvVar("STEAMSHIM_WRITEHANDLE", buf))
        return false;

    return true;
} // setEnvironmentVars


int main(int argc, char **argv)
{
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    GArgc = argc;
    GArgv = argv;


    PipeType pipeParentRead = NULLPIPE;
    PipeType pipeParentWrite = NULLPIPE;
    PipeType pipeChildRead = NULLPIPE;
    PipeType pipeChildWrite = NULLPIPE;
    ProcessType childPid;

    dbgpipe("Parent starting mainline.\n");

    char* exename;

    // temporary hack, make this better
    if (strstr(*GArgv,"warfork_steam"))
    {
        exename = GAME_CLIENT_LAUNCH_NAME;
        GServerType = STEAMGAMECLIENT;
    } else {
        GServerType = STEAMGAMESERVER;
        if (strstr(*GArgv,"wftv_server"))
        {
            exename = GAME_TVSERVER_LAUNCH_NAME;
        }else{
            exename = GAME_SERVER_LAUNCH_NAME;
        }
    }

    if (!createPipes(&pipeParentRead, &pipeParentWrite, &pipeChildRead, &pipeChildWrite))
        fail("Failed to create application pipes");
    else if (!initSteamworks(pipeParentWrite))
        fail("Failed to initialize Steamworks");
    else if (!setEnvironmentVars(pipeChildRead, pipeChildWrite))
        fail("Failed to set environment variables");
    else if (!launchChild(&childPid,exename))
        fail("Failed to launch application");

    // Close the ends of the pipes that the child will use; we don't need them.
    closePipe(pipeChildRead);
    closePipe(pipeChildWrite);
    pipeChildRead = pipeChildWrite = NULLPIPE;

    dbgpipe("Parent in command processing loop.\n");

    // Now, we block for instructions until the pipe fails (child closed it or
    //  terminated/crashed).
    GPipeRead = pipeParentRead;
    GPipeWrite = pipeParentWrite;
    processCommands();

    dbgpipe("Parent shutting down.\n");

    // Close our ends of the pipes.
    // writeBye(pipeParentWrite);
    closePipe(pipeParentRead);
    closePipe(pipeParentWrite);

    // deinitSteamworks();

    dbgpipe("Parent waiting on child process.\n");

    // Wait for the child to terminate, close the child process handles.
    const int retval = closeProcess(&childPid);

    dbgpipe("Parent exiting mainline (child exit code %d).\n", retval);

    return retval;

}

#ifdef _WIN32

// from win_sys
#define MAX_NUM_ARGVS 128
int argc;
char *argv[MAX_NUM_ARGVS];

static void ParseCommandLine( LPSTR lpCmdLine )
{
	argc = 1;
	argv[0] = "exe";

	while( *lpCmdLine && ( argc < MAX_NUM_ARGVS ) ) {
		while( *lpCmdLine && ( *lpCmdLine <= 32 || *lpCmdLine > 126 ) )
			lpCmdLine++;

		if( *lpCmdLine ) {
			char quote = ( ( '"' == *lpCmdLine || '\'' == *lpCmdLine ) ? *lpCmdLine++ : 0 );

			argv[argc++] = lpCmdLine;
			if( quote ) {
				while( *lpCmdLine && *lpCmdLine != quote && *lpCmdLine >= 32 && *lpCmdLine <= 126 )
					lpCmdLine++;
			} else {
				while( *lpCmdLine && *lpCmdLine > 32 && *lpCmdLine <= 126 )
					lpCmdLine++;
			}

			if( *lpCmdLine )
				*lpCmdLine++ = 0;
		}
	}
}

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	ParseCommandLine( lpCmdLine );
	return main( argc, argv );
} // WinMain
#endif

