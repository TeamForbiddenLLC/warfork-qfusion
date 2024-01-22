/*
Copyright (C) 2002-2007 Victor Luchits

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
#ifndef R_GRAPHICS_H
#define R_GRAPHICS_H

#include "../gameshared/q_shared.h"
#include "r_shared.h"

static const uint32_t ByteToKB = 1024;
static const uint32_t ByteToMB = 1024 * ByteToKB;
static const uint32_t ByteToGB = 1024 * ByteToMB;

static inline size_t R_Align( size_t x, size_t alignment )
{
	return ( ( x + alignment - 1 ) & ~( alignment - 1 ) );
}

#define DECLARE_RENDERER_FUNCTION( ret, name, ... ) \
	typedef ret( qcdecl *name##Fn )( __VA_ARGS__ ); \
	extern name##Fn name;


typedef struct {
	uint16_t width;
	uint16_t height;
	bool fullScreen;
} r_renderer_state_t;
extern r_renderer_state_t r_renderer_state;

#endif // !R_GRAPHICS_H
