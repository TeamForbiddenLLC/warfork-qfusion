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

#define DEBUGPIPE 1

#if DEBUGPIPE
#define dbgprintf printf
#else
inline void dbgprintf(const char *fmt, ...) {}
#endif

#include "steam/isteamfriends.h"
#include "steam/isteamuser.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

void TransmitAvatar(uint64 id);
class SteamCallbacks
{
public:
    SteamCallbacks();
	STEAM_CALLBACK(SteamCallbacks, OnCreateBeacon, UserStatsReceived_t, m_CallbackCreateBeacon);
	STEAM_CALLBACK(SteamCallbacks, OnPersonaStateChange, PersonaStateChange_t, m_CallbackPersonaStateChange);

};

extern int GArgc;
extern char **GArgv;
