#ifndef STEAMSHIM_CHILD_H
#define STEAMSHIM_CHILD_H

#include <stdint.h>
#include "../steamshim.h"
#include "../steamshim_types.h"
#ifdef __cplusplus
extern "C" {
#endif

int STEAMSHIM_init(void);  /* non-zero on success, zero on failure. */
void STEAMSHIM_deinit(void);
int STEAMSHIM_alive(void);
const STEAMSHIM_Event *STEAMSHIM_pump(void);
void STEAMSHIM_getSteamID();
void STEAMSHIM_getPersonaName();
void STEAMSHIM_setRichPresence(const char* key, const char* val);
void STEAMSHIM_getAuthSessionTicket();
void STEAMSHIM_beginAuthSession(uint64_t steamid, SteamAuthTicket_t* ticket);
void STEAMSHIM_endAuthSession(uint64_t steamid);
#ifdef __cplusplus
}
#endif

#endif

