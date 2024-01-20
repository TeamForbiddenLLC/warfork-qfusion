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

#define NRI_STATIC_LIBRARY 1
#include "NRI.h"

#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRISwapChain.h"

#include "../gameshared/q_shared.h"


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

#define NRI_ABORT_ON_FAILURE(result) \
    if (result != NriResult_SUCCESS) { exit(0);}


typedef enum { 
	BACKEND_OPENGL_LEGACY, 
	BACKEND_NRI_VULKAN, 
	BACKEND_NRI_METAL, 
	BACKEND_NRI_DX12 
} backend_api_t;

static const char* NriResultToString[NriResult_MAX_NUM] = {
    [NriResult_SUCCESS]= "SUCESS",
    [NriResult_FAILURE]= "FAILURE",
    [NriResult_INVALID_ARGUMENT]= "INVALID_ARGUMENT",
    [NriResult_OUT_OF_MEMORY]= "OUT_OF_MEMORY",
    [NriResult_UNSUPPORTED]= "UNSUPPORTED",
    [NriResult_DEVICE_LOST] = "DEVICE_LOST",
    [NriResult_OUT_OF_DATE] = "OUT_OF_DATE"
};

#define	GLINF_FOFS(x) offsetof(glextinfo_t,x)
#define	GLINF_EXMRK() GLINF_FOFS(_extMarker)
#define	GLINF_FROM(from,ofs) (*((char *)from + ofs))


typedef struct {
	backend_api_t backend;
	// harder to mange this lets not union this
	struct renderer_nri_s {
		NriHelperInterface helperI;
		NriCoreInterface coreI;
		NriSwapChainInterface swapChainI;
		NriDevice *device;
	} nri;
} renderer_t;

extern renderer_t renderer;

typedef struct {
	const char *applicationName;
	int iconResource;
	int startupColor;
	const int *iconXPM;
	void *hinstance;
	void *wndproc;
	void *parenthWnd;
	const char* screenshotPrefix;

	backend_api_t backend;
} renderer_desc_t;

//rserr_t  RE_initRenderer( const renderer_desc_t *desc, renderer_t *renderer );
//bool RE_exitRenderer( renderer_t *renderer );

//void RE_printDetails( renderer_t *renderer );

//bool NRI_checkNumberOfAllocations(renderer_t *renderer, NriResourceGroupDesc* desc, uint32_t minimumNumAllocations); 
#endif // !R_GRAPHICS_H
