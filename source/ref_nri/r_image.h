/*
Copyright (C) 2013 Victor Luchits

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

#ifndef R_IMAGE_H
#define R_IMAGE_H

#include "qstr.h"
#include "r_nri.h"

enum
{
	IT_NONE
	,IT_CLAMP			= 1<<0
	,IT_NOMIPMAP		= 1<<1
	,IT_NOPICMIP		= 1<<2
	,IT_SKY				= 1<<3
	,IT_CUBEMAP			= 1<<4
	,IT_FLIPX			= 1<<5
	,IT_FLIPY			= 1<<6
	,IT_FLIPDIAGONAL	= 1<<7		// when used alone, equals to rotating 90 CW and flipping X; with FLIPX|Y, 90 CCW and flipping X
	,IT_NOCOMPRESS		= 1<<8
	,IT_DEPTH			= 1<<9       // DEPCREATED
	,IT_NORMALMAP		= 1<<10
	,IT_FRAMEBUFFER		= 1<<11      // DEPCREATED
	,IT_DEPTHRB			= 1<<12		 // DEPCREATED
	,IT_NOFILTERING		= 1<<13
	,IT_ALPHAMASK		= 1<<14		// image only contains an alpha mask
	,IT_SYNC			= 1<<16		// load image synchronously
	,IT_DEPTHCOMPARE	= 1<<17
	,IT_ARRAY			= 1<<18
	,IT_3D				= 1<<19
	,IT_STENCIL			= 1<<20	    // DEPCREATED	
	,IT_NO_DATA_SYNC	= 1<<21		// owned by the drawing thread, do not sync in the frontend thread
};

/**
 * These flags don't effect the actual usage and purpose of the image.
 * They are ignored when searching for an image.
 * The loader threads may modify these flags (but no other flags),
 * so they must not be used for anything that has a long-term effect.
 */
#define IT_LOADFLAGS		( IT_ALPHAMASK|IT_SYNC )

#define IT_SPECIAL			( IT_CLAMP|IT_NOMIPMAP|IT_NOPICMIP|IT_NOCOMPRESS )
#define IT_SKYFLAGS			( IT_SKY|IT_NOMIPMAP|IT_CLAMP|IT_SYNC )

/**
 * Image usage tags, to allow certain images to be freed separately.
 */
enum
{
	IMAGE_TAG_GENERIC	= 1<<0		// Images that don't fall into any other category.
	,IMAGE_TAG_BUILTIN	= 1<<1		// Internal ref images that must not be released.
	,IMAGE_TAG_WORLD	= 1<<2		// World textures.
};


typedef struct image_s
{
	union {
    #if(DEVICE_IMPL_VULKAN)
    struct {
    	VkImage image;
    	struct VmaAllocation_T* vmaAlloc;
    } vk;
    #endif
	};
	uint8_t mipNum;
	struct RIDescriptor_s binding;
	struct RIDescriptor_s* samplerBinding;

	struct QStr name;// game path, not including extension 
	int				registrationSequence;
	volatile bool loaded;
	volatile bool missing;

	const char* extension; // file extension
	int flags;
	uint16_t width, height; // source image
	int layers;				// texture array size
	int minmipsize;	   // size of the smallest mipmap that should be used
	int samples;
	unsigned int framenum; // rf.frameCount texture was updated (rendered to)
	int tags;			   // usage tags of the image
	struct image_s *next, *prev;
} image_t;

void R_InitImages( void );
void R_TouchImage( image_t *image, int tags );
void R_FreeUnusedImagesByTags( int tags );
void R_FreeUnusedImages( void );
void R_ShutdownImages( void );
image_t *R_CreateImage( const char *name, int width, int height, int layers, int flags, int minmipsize, int tags, int samples );
void R_InitDrawFlatTexture( void );
void R_FreeImageBuffers( void );

void R_PrintImageList( const char *pattern, bool (*filter)( const char *filter, const char *value) );
void R_ScreenShot( const char *filename, int x, int y, int width, int height, 
	bool flipx, bool flipy, bool flipdiagonal, bool silent );

void R_TextureMode( char *string );
void R_AnisotropicFilter( int value );

image_t *R_LoadImage( const char *name, uint8_t **pic, int width, int height, int flags, int minmipsize, int tags, int samples );
image_t	*R_FindImage( const char *name, const char *suffix, int flags, int minmipsize, int tags );
void R_ReplaceImage( image_t *image, uint8_t **pic, int width, int height, int flags, int minmipsize, int samples );
void R_ReplaceSubImage( image_t *image, int layer, int x, int y, uint8_t **pic, int width, int height );
void R_ReplaceImageLayer( image_t *image, int layer, uint8_t **pic );

struct RIDescriptor_s* R_ResolveSamplerDescriptor( int flags );

#endif // R_IMAGE_H
