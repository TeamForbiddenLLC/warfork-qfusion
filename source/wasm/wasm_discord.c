#ifdef __WASM__

#include "discord_rpc.h"
#include "discord_register.h"

void Discord_Initialize( const char *applicationId, DiscordEventHandlers *handlers, int autoRegister, const char *optionalSteamId )
{
	(void)applicationId;
	(void)handlers;
	(void)autoRegister;
	(void)optionalSteamId;
}

void Discord_Shutdown( void )
{
}

void Discord_RunCallbacks( void )
{
}

void Discord_UpdatePresence( const DiscordRichPresence *presence )
{
	(void)presence;
}

void Discord_ClearPresence( void )
{
}

void Discord_Respond( const char *userid, int reply )
{
	(void)userid;
	(void)reply;
}

void Discord_UpdateHandlers( DiscordEventHandlers *handlers )
{
	(void)handlers;
}

void Discord_Register( const char *applicationId, const char *command )
{
	(void)applicationId;
	(void)command;
}

void Discord_RegisterSteamGame( const char *applicationId, const char *steamId )
{
	(void)applicationId;
	(void)steamId;
}

#endif // __WASM__
