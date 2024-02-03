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

#ifndef _INCL_STEAMSHIM_TYPES_H_
#define _INCL_STEAMSHIM_TYPES_H_

#define STEAM_AVATAR_SIZE (32*32*4)
#define PIPEMESSAGE_MAX (STEAM_AVATAR_SIZE + 64)
#define AUTH_TICKET_MAXSIZE 1024
typedef struct {
  char pTicket[AUTH_TICKET_MAXSIZE];
  long long pcbTicket;
} SteamAuthTicket_t;

#endif
