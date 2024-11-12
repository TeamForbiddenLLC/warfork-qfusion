/*
Copyright (C) 2016 Victor Luchits

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

#ifndef R_FRONTEND_H
#define R_FRONTEND_H

#include "r_local.h"
#include "r_cmdque.h"

// sync-to-async frontend adapter
typedef struct {
	void			*owner;				// pointer to parent ref_frontend_t
	void            *GLcontext;
	unsigned		frameNum;
	uint32_t 		frameId;
	volatile uint32_t readFrameId;
	qthread_t		*thread;
	qmutex_t		*frameLock;
	ref_cmdpipe_t	*cmdPipe;
	volatile bool 	shutdown;
	volatile int 	maxfps;
} ref_frontendAdapter_t;

typedef struct
{
	unsigned		frameNum; 			// wrapped
	unsigned		lastFrameNum;
	uint32_t 		frameId;

	ref_cmdbuf_t	*frames[3];			// triple-buffered
	ref_cmdbuf_t	*frame; 			// current frontend frame

	void            *auxGLContext;

	ref_frontendAdapter_t adapter;

	// these fields serve as the frontend cache which can also queried by the public API
	int 			scissor[4];
	float			cameraSeparation;
	byte_vec4_t		customColors[NUM_CUSTOMCOLORS];
} ref_frontend_t;

#endif // R_FRONTEND_H
