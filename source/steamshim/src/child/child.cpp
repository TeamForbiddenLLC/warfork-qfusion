#include <cstdint>
#include <cstring>
#include <stdio.h>
#define DEBUGPIPE 1
#include "child_private.h"
#include "../steamshim.h"
#include "../steamshim_private.h"
#include "../os.h"
#include "../steamshim_types.h"

int GArgc = 0;
char **GArgv = NULL;

static int initPipes(void)
{
    char buf[64];
    unsigned long long val;

    if (!getEnvVar("STEAMSHIM_READHANDLE", buf, sizeof (buf)))
        return 0;
    else if (sscanf(buf, "%llu", &val) != 1)
        return 0;
    else
        GPipeRead = (PipeType) val;

    if (!getEnvVar("STEAMSHIM_WRITEHANDLE", buf, sizeof (buf)))
        return 0;
    else if (sscanf(buf, "%llu", &val) != 1)
        return 0;
    else
        GPipeWrite = (PipeType) val;
    
    return ((GPipeRead != NULLPIPE) && (GPipeWrite != NULLPIPE));
} /* initPipes */

static STEAMSHIM_Event* ProcessEvent(){
    static STEAMSHIM_Event event;
    // make sure this is static, since it needs to persist between pumps
    static pipebuff_t buf;

    if (!buf.Recieve())
        return NULL;

    if (!buf.hasmsg)
        return NULL;

    volatile unsigned int msglen =buf.ReadInt();


    char type = buf.ReadByte();
    event.type = (STEAMSHIM_EventType)type;

    #if DEBUGPIPE
    if (0) {}
    #define PRINTGOTEVENT(x) else if (type == x) printf("Child got " #x ".\n")
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
    #undef PRINTGOTEVENT
    else printf("Child got unknown shimevent %d.\n", (int) type);
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
        default:
            return NULL;
    }

    return &event;
}

extern "C" {
  int STEAMSHIM_init(void)
  {
      dbgpipe("Child init start.\n");
      if (!initPipes())
      {
          dbgpipe("Child init failed.\n");
          return 0;
      }

#ifndef _WIN32
      signal(SIGPIPE, SIG_IGN);
#endif

      dbgpipe("Child init success!\n");
      return 1;
  } 

  void STEAMSHIM_deinit(void)
  {
      dbgpipe("Child deinit.\n");
      if (GPipeWrite != NULLPIPE)
      {
          // writeBye();
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
    // Write1ByteMessage(SHIMCMD_PUMP);
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
      pipebuff_t buf;
      buf.WriteByte(SHIMCMD_BEGINAUTHSESSION);
      buf.WriteLong(steamid);
      buf.WriteLong(ticket->pcbTicket);
      buf.WriteData(ticket->pTicket, AUTH_TICKET_MAXSIZE);
      buf.Transmit();
  }

  void STEAMSHIM_endAuthSession(uint64_t steamid){
    pipebuff_t buf;
    buf.WriteByte(SHIMCMD_ENDAUTHSESSION);
    buf.WriteLong(steamid);
    buf.Transmit();
  }

  void STEAMSHIM_setRichPresence(char* key, char* val){
      pipebuff_t buf;
      buf.WriteByte(SHIMCMD_SETRICHPRESENCE);
      buf.WriteString(key);
      buf.WriteString(val);
      buf.Transmit();
  }
}
