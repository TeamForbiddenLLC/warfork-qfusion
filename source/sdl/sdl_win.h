/*
   Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef __SDL_WIN_H_
#define __SDL_WIN_H_

#include <SDL.h>
#include "../gameshared/q_cvar.h"


typedef int (* wndproc_t)(void *, int, int, int);

typedef struct {
	char *applicationName;
	int *applicationIcon;
	SDL_Window *sdl_window;
	wndproc_t wndproc;
} winstate_t;

extern winstate_t r_winState;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_multiscreen_head;

#endif // __SDL_GLW_H_

