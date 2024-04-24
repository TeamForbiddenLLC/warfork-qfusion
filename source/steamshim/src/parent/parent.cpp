/*
Copyright (C) 2023 coolelectronics

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

#include <cstdint>
#include <cstring>
#include <stdio.h>
#include <thread>
#define DEBUGPIPE 1
#include "parent.h"
#include "../steamshim.h"
#include "../steamshim_private.h"
#include "../os.h"
#include "../steamshim_types.h"


int GArgc = 0;
char **GArgv = NULL;

static STEAMSHIM_Event* ProcessEvent(){
    static STEAMSHIM_Event event;
    // make sure this is static, since it needs to persist between pumps
    static PipeBuffer buf;

    if (!buf.Recieve())
        return NULL;

    if (!buf.hasmsg)
        return NULL;

    volatile unsigned int msglen =buf.ReadInt();


    char type = buf.ReadByte();
    event.type = (STEAMSHIM_EventType)type;

    #if DEBUGPIPE
    if (0) {}
    #define PRINTGOTEVENT(x) else if (type == x) dbgprintf("Parent got " #x ".\n")
    PRINTGOTEVENT(SHIMEVENT_BYE);
    PRINTGOTEVENT(SHIMEVENT_STATSRECEIVED);
    PRINTGOTEVENT(SHIMEVENT_STATSSTORED);
    PRINTGOTEVENT(SHIMEVENT_SETACHIEVEMENT);
    PRINTGOTEVENT(SHIMEVENT_GETACHIEVEMENT);
    PRINTGOTEVENT(SHIMEVENT_RESETSTATS);
    PRINTGOTEVENT(SHIMEVENT_SETSTATI);
    PRINTGOTEVENT(SHIMEVENT_GETSTATI);
    PRINTGOTEVENT(SHIMEVENT_SETSTATF);
    PRINTGOTEVENT(SHIMEVENT_GETSTATF);
    PRINTGOTEVENT(SHIMEVENT_STEAMIDRECIEVED);
    PRINTGOTEVENT(SHIMEVENT_PERSONANAMERECIEVED);
    PRINTGOTEVENT(SHIMEVENT_AUTHSESSIONTICKETRECIEVED);
    PRINTGOTEVENT(SHIMEVENT_AUTHSESSIONVALIDATED);
    PRINTGOTEVENT(SHIMEVENT_AVATARRECIEVED);
    PRINTGOTEVENT(SHIMEVENT_COMMANDLINERECIEVED);
    PRINTGOTEVENT(SHIMEVENT_GAMEJOINREQUESTED);
    #undef PRINTGOTEVENT
    else printf("Parent got unknown shimevent %d.\n", (int) type);
    #endif

    switch (type){
        case SHIMEVENT_STEAMIDRECIEVED:
            {
                event.lvalue = buf.ReadLong();
            }
            break;
        case SHIMEVENT_PERSONANAMERECIEVED:
            {
                char *string = buf.ReadString();
                strcpy(event.name, string);
            }
            break;
        case SHIMEVENT_AUTHSESSIONTICKETRECIEVED:
            {
                long long pcbTicket = buf.ReadLong();
                void *ticket = buf.ReadData(AUTH_TICKET_MAXSIZE);
                event.lvalue = pcbTicket;
                memcpy(event.name, ticket, AUTH_TICKET_MAXSIZE);
            }
            break;
        case SHIMEVENT_AUTHSESSIONVALIDATED:
            {
                int result = buf.ReadInt();
                event.ivalue = result;
            }
            break;
        case SHIMEVENT_AVATARRECIEVED:
            {
                uint64_t id = buf.ReadLong();
                void *image = buf.ReadData(STEAM_AVATAR_SIZE);
                event.lvalue = id;
                memcpy(event.name, image, STEAM_AVATAR_SIZE);
            }
            break;
        case SHIMEVENT_GAMEJOINREQUESTED:
            event.lvalue = buf.ReadLong();
            [[fallthrough]];
        case SHIMEVENT_COMMANDLINERECIEVED:
            {
                char *string = (char*)buf.ReadString();
                strncpy(event.name, string, sizeof(event.name));
            }
            break;
    }

    return &event;
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
} 

static void taskHeartbeat(){
    while (true) {
        Write1ByteMessage(SHIMCMD_PUMP);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

extern "C" {
  int STEAMSHIM_init(SteamshimOptions *options)
  {
    debug = options->debug;


    PipeType pipeParentRead = NULLPIPE;
    PipeType pipeParentWrite = NULLPIPE;
    PipeType pipeChildRead = NULLPIPE;
    PipeType pipeChildWrite = NULLPIPE;
    ProcessType childPid;

    if (options->runclient)
        setEnvVar("STEAMSHIM_RUNCLIENT", "1");
    if (options->runserver)
        setEnvVar("STEAMSHIM_RUNSERVER", "1");


    if (!createPipes(&pipeParentRead, &pipeParentWrite, &pipeChildRead, &pipeChildWrite)){
        printf("steamshim: Failed to create application pipes\n");
        return 0;
    }
    else if (!setEnvironmentVars(pipeChildRead, pipeChildWrite)){
        printf("steamshim: Failed to set environment variables\n");
        return 0;
    }
    else if (!launchChild(&childPid))
    {
        printf("steamshim: Failed to launch application\n");
        return 0;
    }

    GPipeRead = pipeParentRead;
    GPipeWrite = pipeParentWrite;

    char status;
     
    readPipe(GPipeRead, &status, sizeof status);

    if (!status){
        closePipe(GPipeRead);
        closePipe(GPipeWrite);

        GPipeWrite = GPipeRead = pipeChildRead = pipeChildWrite = NULLPIPE;
        return 0;
    }

    dbgprintf("Parent init start.\n");

    // Close the ends of the pipes that the child will use; we don't need them.
    closePipe(pipeChildRead);
    closePipe(pipeChildWrite);

    pipeChildRead = pipeChildWrite = NULLPIPE;


    std::thread task(taskHeartbeat);
    task.detach();

#ifndef _WIN32
      signal(SIGPIPE, SIG_IGN);
#endif

      dbgprintf("Child init success!\n");
      return 1;
  } 

  void STEAMSHIM_deinit(void)
  {
      dbgprintf("Child deinit.\n");
      if (GPipeWrite != NULLPIPE)
      {
          Write1ByteMessage(SHIMCMD_BYE);
          closePipe(GPipeWrite);
      } 

      if (GPipeRead != NULLPIPE)
          closePipe(GPipeRead);

      GPipeRead = GPipeWrite = NULLPIPE;

#ifndef _WIN32
      signal(SIGPIPE, SIG_DFL);
#endif
  } 

  static inline int isAlive(void)
  {
      return ((GPipeRead != NULLPIPE) && (GPipeWrite != NULLPIPE));
  } 

  static inline int isDead(void)
  {
      return !isAlive();
  }

  int STEAMSHIM_alive(void)
  {
      return isAlive();
  } 

  const STEAMSHIM_Event *STEAMSHIM_pump(void)
  {
    Write1ByteMessage(SHIMCMD_PUMP);
    return ProcessEvent();
  } 

  void STEAMSHIM_getSteamID()
  {
	  Write1ByteMessage(SHIMCMD_REQUESTSTEAMID);
  }

  void STEAMSHIM_getPersonaName(){
      Write1ByteMessage(SHIMCMD_REQUESTPERSONANAME);
  }

  void STEAMSHIM_getAuthSessionTicket(){
      Write1ByteMessage(SHIMCMD_REQUESTAUTHSESSIONTICKET);
  }

  void STEAMSHIM_beginAuthSession(uint64_t steamid, SteamAuthTicket_t* ticket){
      PipeBuffer buf;
      buf.WriteByte(SHIMCMD_BEGINAUTHSESSION);
      buf.WriteLong(steamid);
      buf.WriteLong(ticket->pcbTicket);
      buf.WriteData(ticket->pTicket, AUTH_TICKET_MAXSIZE);
      buf.Transmit();
  }

  void STEAMSHIM_endAuthSession(uint64_t steamid){
    PipeBuffer buf;
    buf.WriteByte(SHIMCMD_ENDAUTHSESSION);
    buf.WriteLong(steamid);
    buf.Transmit();
  }

  void STEAMSHIM_setRichPresence(int num, const char** key, const char** val){
      PipeBuffer buf;
      buf.WriteByte(SHIMCMD_SETRICHPRESENCE);
      buf.WriteInt(num);
      for (int i=0; i < num;i++){
          buf.WriteString(key[i]);
          buf.WriteString(val[i]);
      }
      buf.Transmit();
  }

  void STEAMSHIM_createBeacon(uint32_t openSlots, char* connectString, char* metadata)
  {
      PipeBuffer buf;
      buf.WriteByte(SHIMCMD_CREATEBEACON);
      buf.WriteInt(openSlots);
      buf.WriteString(connectString);
      buf.WriteString(metadata);
      buf.Transmit();
  }

  void STEAMSHIM_requestAvatar(uint64_t steamid, int size){
    PipeBuffer buf;
    buf.WriteByte(SHIMCMD_REQUESTAVATAR);
    buf.WriteLong(steamid);
    buf.WriteInt(size);
    buf.Transmit();
  }

  void STEAMSHIM_openProfile(uint64_t steamid) {
      PipeBuffer buf;
      buf.WriteByte(SHIMCMD_OPENPROFILE);
      buf.WriteLong(steamid);
      buf.Transmit();
  }

  void STEAMSHIM_requestCommandLine(){
    Write1ByteMessage(SHIMCMD_REQUESTCOMMANDLINE);
  }
}
