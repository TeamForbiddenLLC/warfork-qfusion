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

#pragma once
#include <stdint.h>

#include "steamshim_types.h"
#ifdef __cplusplus
extern "C" {
#endif

#define STEAM_MAX_AVATAR_SIZE ( 128 * 128 * 4 )
#define STEAM_AUTH_TICKET_MAXSIZE 1024

typedef enum eventtype {

	EVT_CL_STEAMIDRECIEVED,
	EVT_CL_PERSONANAMERECIEVED,
	EVT_CL_AUTHSESSIONTICKETRECIEVED,
	EVT_CL_AVATARRECIEVED,
	EVT_CL_COMMANDLINERECIEVED,

	EVT_CL_GAMEJOINREQUESTED,
	EVT_CL_SERVERRECIEVED,

	EVT_SV_AUTHSESSIONVALIDATED,
} SteamshimEventType;

typedef uint64_t Event_cl_steamidrecieved_t;
typedef char *Event_cl_personanamerecieved_t;
typedef struct Event_cl_authsessionticketrecieved {
	char pTicket[AUTH_TICKET_MAXSIZE];
	long pcbTicket;
} Event_cl_authsessionticketrecieved_t;
typedef struct Event_cl_avatarrecieved {
	uint8_t *avatar;
	uint64_t steamid;
} Event_cl_avatarrecieved_t;
typedef char *Event_cl_commandlinerecieved_t;
typedef struct Event_cl_gamejoinrequested {
	uint64_t steamIDFriend;
	char *connectString;
} Event_cl_gamejoinrequested_t;
typedef struct Event_cl_serverrecieved {
	char *serverName;
} Event_cl_serverrecieved_t;
typedef int Event_sv_authsessionvalidated_t;

typedef struct SteamshimEvent {
	SteamshimEventType type;
	union {
		Event_cl_steamidrecieved_t cl_steamidrecieved;
		Event_cl_personanamerecieved_t cl_personanamerecieved;
		Event_cl_authsessionticketrecieved_t cl_authsessionticketrecieved;
		Event_cl_avatarrecieved_t cl_avatarrecieved;
		Event_cl_commandlinerecieved_t cl_commandlinerecieved;
		Event_cl_gamejoinrequested_t cl_gamejoinrequested;
		Event_cl_serverrecieved_t cl_serverrecieved;
		Event_sv_authsessionvalidated_t sv_authsessionvalidated;
	};
} SteamshimEvent;

typedef enum {
	AVATAR_SMALL,
	AVATAR_MEDIUM,
	AVATAR_LARGE,
} SteamAvatarSize;


#ifdef __cplusplus
}
#endif
