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

#ifndef STEAMSHIM_H
#define STEAMSHIM_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define AUTH_TICKET_MAXSIZE 1024

typedef enum STEAMSHIM_EventType
{
    SHIMEVENT_BYE,
    SHIMEVENT_STATSRECEIVED,
    SHIMEVENT_STATSSTORED,
    SHIMEVENT_SETACHIEVEMENT,
    SHIMEVENT_GETACHIEVEMENT,
    SHIMEVENT_RESETSTATS,
    SHIMEVENT_SETSTATI,
    SHIMEVENT_GETSTATI,
    SHIMEVENT_SETSTATF,
    SHIMEVENT_GETSTATF,
    SHIMEVENT_STEAMIDRECIEVED,
    SHIMEVENT_PERSONANAMERECIEVED,
    SHIMEVENT_AUTHSESSIONTICKETRECIEVED,
    SHIMEVENT_AUTHSESSIONVALIDATED,
    SHIMEVENT_BEACONCREATED,
    SHIMEVENT_AVATARRECIEVED,
} STEAMSHIM_EventType;

/* not all of these fields make sense in a given event. */
typedef struct STEAMSHIM_Event
{
    STEAMSHIM_EventType type;
    int okay;
    int ivalue;
    float fvalue;
    unsigned long long lvalue;
    char name[4096];
} STEAMSHIM_Event;

#ifdef __cplusplus
}
#endif

#endif

