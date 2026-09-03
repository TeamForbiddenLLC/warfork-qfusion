/*
Copyright (C) 2015 SiPlus, Chasseur de bots

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <SDL3/SDL.h>
#include "../client/client.h"
#include "sdl_input_joy.h"

// Number of standard gamepad buttons we map (matches the keys[] table below: the 15 buttons
// SOUTH..DPAD_RIGHT). SDL3's SDL_GAMEPAD_BUTTON_COUNT (21) includes paddles/touchpad we don't map,
// and the two trigger pseudo-buttons are packed at bit positions COUNT and COUNT+1.
#define GAMEPAD_NUM_MAPPED_BUTTONS ( SDL_GAMEPAD_BUTTON_DPAD_RIGHT + 1 )

static bool in_sdl_joyInitialized, in_sdl_joyActive;
static SDL_Gamepad *in_sdl_joyController;

/*
* IN_SDL_JoyInit
*
* SDL game controller code called in IN_Init.
*/
void IN_SDL_JoyInit( bool active )
{
	in_sdl_joyActive = active;

	if( !SDL_InitSubSystem( SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD ) )
		return;

	in_sdl_joyInitialized = true;
}

/*
* IN_SDL_JoyActivate
*/
void IN_SDL_JoyActivate( bool active )
{
	in_sdl_joyActive = active;
}

/*
* IN_SDL_JoyCommands
*
* SDL game controller code called in IN_Commands.
*/
void IN_SDL_JoyCommands( void )
{
	int i, buttons = 0, buttonsDiff;
	static int buttonsOld;
	const int keys[] =
	{
		K_A_BUTTON, K_B_BUTTON, K_X_BUTTON, K_Y_BUTTON, K_ESCAPE, 0, 0,
		K_LSTICK, K_RSTICK, K_LSHOULDER, K_RSHOULDER,
		K_DPAD_UP, K_DPAD_DOWN, K_DPAD_LEFT, K_DPAD_RIGHT,
		K_LTRIGGER, K_RTRIGGER
	};

	if( in_sdl_joyInitialized )
	{
		SDL_UpdateGamepads();

		if( in_sdl_joyController && !SDL_GamepadConnected( in_sdl_joyController ) )
		{
			SDL_CloseGamepad( in_sdl_joyController );
			in_sdl_joyController = NULL;
		}

		if( !in_sdl_joyController )
		{
			int count = 0;
			SDL_JoystickID *ids = SDL_GetGamepads( &count );
			if( ids )
			{
				for( i = 0; i < count; i++ )
				{
					in_sdl_joyController = SDL_OpenGamepad( ids[i] );
					if( in_sdl_joyController )
						break;
				}
				SDL_free( ids );
			}
		}
	}

	if( in_sdl_joyActive )
	{
		SDL_Gamepad *controller = in_sdl_joyController;
		if( controller )
		{
			for( i = 0; i < GAMEPAD_NUM_MAPPED_BUTTONS; i++ )
			{
				if( keys[i] && SDL_GetGamepadButton( controller, i ) )
					buttons |= 1 << i;
			}

			if( SDL_GetGamepadButton( controller, SDL_GAMEPAD_BUTTON_START ) )
				buttons |= 1 << SDL_GAMEPAD_BUTTON_BACK;
			if( SDL_GetGamepadAxis( controller, SDL_GAMEPAD_AXIS_LEFT_TRIGGER ) > ( 30 * 128 ) )
				buttons |= 1 << GAMEPAD_NUM_MAPPED_BUTTONS;
			if( SDL_GetGamepadAxis( controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER ) > ( 30 * 128 ) )
				buttons |= 1 << ( GAMEPAD_NUM_MAPPED_BUTTONS + 1 );
		}
	}

	buttonsDiff = buttons ^ buttonsOld;
	if( buttonsDiff )
	{
		unsigned int time = Sys_Milliseconds();

		for( i = 0; i < ( sizeof( keys ) / sizeof( keys[0] ) ); i++ )
		{
			if( buttonsDiff & ( 1 << i ) )
				Key_Event( keys[i], ( buttons & ( 1 << i ) ) ? true : false, time );
		}

		buttonsOld = buttons;
	}
}

/*
* IN_SDL_JoyThumbstickValue
*/
static float IN_SDL_JoyThumbstickValue( int value )
{
	return value * ( ( value >= 0 ) ? ( 1.0f / 32767.0f ) : ( 1.0f / 32768.0f ) );
}

/*
* IN_GetThumbsticks
*/
void IN_GetThumbsticks( vec4_t sticks )
{
	SDL_Gamepad *controller = in_sdl_joyController;

	if( !controller || !in_sdl_joyActive )
	{
		Vector4Set( sticks, 0.0f, 0.0f, 0.0f, 0.0f );
		return;
	}

	sticks[0] = IN_SDL_JoyThumbstickValue( SDL_GetGamepadAxis( controller, SDL_GAMEPAD_AXIS_LEFTX ) );
	sticks[1] = IN_SDL_JoyThumbstickValue( SDL_GetGamepadAxis( controller, SDL_GAMEPAD_AXIS_LEFTY ) );
	sticks[2] = IN_SDL_JoyThumbstickValue( SDL_GetGamepadAxis( controller, SDL_GAMEPAD_AXIS_RIGHTX ) );
	sticks[3] = IN_SDL_JoyThumbstickValue( SDL_GetGamepadAxis( controller, SDL_GAMEPAD_AXIS_RIGHTY ) );
}

/*
* IN_SDL_JoyShutdown
*
* SDL game controller code called in IN_Shutdown.
*/
void IN_SDL_JoyShutdown( void )
{
	if( !in_sdl_joyInitialized )
		return;

	in_sdl_joyController = NULL;
	SDL_QuitSubSystem( SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD );
	in_sdl_joyInitialized = false;
}
