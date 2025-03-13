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

#include "r_image.h"
#include "r_local.h"
#include "r_imagelib.h"

#include "r_texture_format.h"
#include "r_texture_buf.h"
#include "r_texture_format.h"

#include "../gameshared/q_sds.h"
#include "../gameshared/q_arch.h"
#include "r_texture_buffer_load.h"

#include "r_ktx_loader.h"
#include "ri_resource_upload.h"

#include "ri_renderer.h"
#include "ri_types.h"
#include "ri_format.h"

#include <qstr.h>
#include "stb_ds.h"
#include "stb_image.h"
#include "../qalgo/hash.h"

#define	MAX_GLIMAGES	    8192
#define IMAGES_HASH_SIZE    64
#define IMAGE_SAMPLER_HASH_SIZE 1024

static struct RIDescriptor_s samplerDescriptors[IMAGE_SAMPLER_HASH_SIZE] = {0};

typedef struct
{
	int ctx;
	int side;
} loaderCbInfo_t;

static struct image_s images[MAX_GLIMAGES];
static struct image_s images_hash_headnode[IMAGES_HASH_SIZE], *free_images;
static qmutex_t *r_imagesLock;
static mempool_t *r_imagesPool;

enum ImageFilter{
 IMAGE_FILTER_NEAREST,
 IMAGE_FILTER_LINEAR,
};

static int defaultFilterMin = IMAGE_FILTER_NEAREST;
static int defaultFilterMag = IMAGE_FILTER_NEAREST;
static int defaultFilterMipMap = IMAGE_FILTER_LINEAR;
static int defaultAnisotropicFilter = 0;

#define MAX_MIP_SAMPLES 16

static void __FreeImage( struct image_s *image );
static void __R_CopyTextureDataTexture(struct image_s* image, int layer, int mipOffset, int x, int y, int w, int h, enum RI_Format_e srcFormat, uint8_t *data );
static enum RI_Format_e __R_GetImageFormat( struct image_s *image );
static uint16_t __R_calculateMipMapLevel(int flags, int width, int height, uint32_t minMipSize);

// image data is attached to the buffer
static void __FreeGPUImageData( struct image_s *image )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		assert( RI_VK_DESCRIPTOR_IS_IMAGE( image->binding ) );
		struct RIFree_s freeSlot = {};
		struct r_frame_set_s *activeset = RI_ACTIVE_FRAMESET();
		freeSlot.type = RI_FREE_VK_VMA_AllOC;
		freeSlot.vmaAlloc = image->vk.vmaAlloc;
		arrpush( activeset->freeList, freeSlot);

		freeSlot.type = RI_FREE_VK_IMAGE;
		freeSlot.vkImage = image->handle.vk.image;
		arrpush( activeset->freeList, freeSlot );

		freeSlot.type = RI_FREE_VK_IMAGEVIEW;
		freeSlot.vkImageView = image->binding.vk.image.imageView;
		arrpush( activeset->freeList, freeSlot );
		memset(&image->binding, 0, sizeof(struct RIDescriptor_s));
	}
#endif
}

struct RIDescriptor_s *R_ResolveSamplerDescriptor( int flags )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		if( flags & IT_NOFILTERING ) {
			info.minFilter = VK_FILTER_LINEAR;
			info.magFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		} else if( flags & IT_DEPTH ) {
			info.minFilter = VK_FILTER_LINEAR;
			info.magFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.maxAnisotropy = defaultAnisotropicFilter;
			info.anisotropyEnable = defaultAnisotropicFilter > 1.0f;
		} else if( !( flags & IT_NOMIPMAP ) ) {
			VkFilter filterMapping[] = { [IMAGE_FILTER_LINEAR] = VK_FILTER_LINEAR, [IMAGE_FILTER_NEAREST] = VK_FILTER_NEAREST };
			VkSamplerMipmapMode mapMapFilterMapping[] = { [IMAGE_FILTER_LINEAR] = VK_SAMPLER_MIPMAP_MODE_LINEAR, [IMAGE_FILTER_NEAREST] = VK_SAMPLER_MIPMAP_MODE_NEAREST };
			info.minFilter = filterMapping[defaultFilterMin];
			info.magFilter = filterMapping[defaultFilterMag];
			info.mipmapMode = mapMapFilterMapping[defaultFilterMipMap];
			info.maxLod = 16;
			info.maxAnisotropy = defaultAnisotropicFilter;
			info.anisotropyEnable = defaultAnisotropicFilter > 1.0f;
		} else {
			info.minFilter = VK_FILTER_LINEAR;
			info.magFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.maxAnisotropy = defaultAnisotropicFilter;
			info.anisotropyEnable = defaultAnisotropicFilter > 1.0f;
		}

		if( flags & IT_CLAMP ) {
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
		if( ( flags & IT_DEPTH ) && ( flags & IT_DEPTHCOMPARE ) ) {
			info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			info.compareEnable = 1;
		}
		const hash_t hash = hash_data( HASH_INITIAL_VALUE, &info, sizeof( VkSamplerCreateInfo ) );
		const size_t startIndex = ( hash % IMAGE_SAMPLER_HASH_SIZE );
		size_t index = startIndex;
		do {
			if( samplerDescriptors[index].cookie == hash ) {
				return &samplerDescriptors[index];
			} else if( RI_IsEmptyDescriptor(&samplerDescriptors[index] ) ) {
				samplerDescriptors[index].cookie = hash;
				samplerDescriptors[index].vk.type = VK_DESCRIPTOR_TYPE_SAMPLER;
				samplerDescriptors[index].vk.image.imageView = VK_NULL_HANDLE;
				samplerDescriptors[index].vk.image.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				samplerDescriptors[index].flags = RI_VK_DESC_OWN_SAMPLER;
				VK_WrapResult( vkCreateSampler( rsh.device.vk.device, &info, NULL, &samplerDescriptors[index].vk.image.sampler ) );
				RI_UpdateDescriptor( &rsh.device, &samplerDescriptors[index] );
				return &samplerDescriptors[index];
			}
			index = ( index + 1 ) % IMAGE_SAMPLER_HASH_SIZE;
		} while( index != startIndex );
	}
#endif
	return NULL;
}

#undef __SAMPLER_HASH_SIZE


static void __RefreshSamplerDescriptors() {
	image_t	*glt;
	size_t i;
	for( i = 1, glt = images; i < MAX_GLIMAGES; i++, glt++ )
	{
		if( RI_IsEmptyDescriptor( &glt->binding ) ) {
			continue;
		}
		if( (glt->flags & (IT_NOFILTERING | IT_NOMIPMAP)) ) {
			continue;
		}
		glt->samplerBinding = R_ResolveSamplerDescriptor( glt->flags );
	}

}

void R_TextureMode( char *string )
{
	static struct {
		char *name;
		int min, mag, mip;
	} modes[] = { { "GL_NEAREST", IMAGE_FILTER_NEAREST, IMAGE_FILTER_NEAREST, IMAGE_FILTER_LINEAR},
				  { "GL_LINEAR",  IMAGE_FILTER_LINEAR, IMAGE_FILTER_LINEAR, IMAGE_FILTER_LINEAR},
				  { "GL_NEAREST_MIPMAP_NEAREST", IMAGE_FILTER_NEAREST, IMAGE_FILTER_NEAREST, IMAGE_FILTER_NEAREST},
				  { "GL_LINEAR_MIPMAP_NEAREST", IMAGE_FILTER_LINEAR, IMAGE_FILTER_LINEAR, IMAGE_FILTER_NEAREST},
				  { "GL_NEAREST_MIPMAP_LINEAR", IMAGE_FILTER_NEAREST, IMAGE_FILTER_NEAREST, IMAGE_FILTER_LINEAR },
				  { "GL_LINEAR_MIPMAP_LINEAR", IMAGE_FILTER_LINEAR, IMAGE_FILTER_LINEAR, IMAGE_FILTER_LINEAR } };

	size_t i = 0;
	for(i = 0; i < Q_ARRAY_COUNT(modes); i++) {
		if(!Q_stricmp( modes[i].name, string ) )
			break;
	}

	if( i == Q_ARRAY_COUNT(modes))
	{
		Com_Printf( "R_TextureMode: bad filter name\n" );
		return;
	}

	defaultFilterMin = modes[i].min;
	defaultFilterMag = modes[i].mag;
	defaultFilterMipMap = modes[i].mip;
	__RefreshSamplerDescriptors();
}

void R_AnisotropicFilter( int value )
{
	defaultAnisotropicFilter = bound( 1, value, rsh.device.physicalAdapter.samplerAnisotropyMax );
	__RefreshSamplerDescriptors();
}

/*
* R_PrintImageList
*/
void R_PrintImageList( const char *mask, bool (*filter)( const char *mask, const char *value) )
{
	int i, bpp, bytes;
	int numImages;
	image_t	*image;
	double texels = 0, add, total_bytes = 0;

	Com_Printf( "------------------\n" );

	numImages = 0;
	for( i = 0, image = images; i < MAX_GLIMAGES; i++, image++ )
	{
			
		if( !RI_IsEmptyDescriptor(&image->binding)) {
			continue;
		}
		if( !image->width || !image->height || !image->layers ) {
			continue;
		}
		if( filter && !filter( mask, image->name.buf) ) {
			continue;
		}
		if( !image->loaded || image->missing ) {
			continue;
		}
		assert((image->flags & (IT_FRAMEBUFFER | IT_DEPTHRB | IT_DEPTH)) == 0);

		add = image->width * image->height * image->layers;
		if( !(image->flags & (IT_DEPTH|IT_NOFILTERING|IT_NOMIPMAP)) )
			add = (unsigned)floor( add / 0.75 );
		if( image->flags & IT_CUBEMAP )
			add *= 6;
		texels += add;

		bpp = image->samples;
		bytes = add * bpp;
		total_bytes += bytes;

		Com_Printf( " %iW x %iH", image->width, image->height );
		if( image->layers > 1 )
			Com_Printf( " x %iL", image->layers );
		Com_Printf( " x %iBPP: %s%s%s %.1f KB\n", bpp, image->name, image->extension,
			((image->flags & (IT_NOMIPMAP|IT_NOFILTERING)) ? "" : " (mip)"), bytes / 1024.0 );

		numImages++;
	}

	Com_Printf( "Total texels count (counting mipmaps, approx): %.0f\n", texels );
	Com_Printf( "%i RGBA images, totalling %.3f megabytes\n", numImages, total_bytes / 1048576.0 );
}

/*
=================================================================

TEMPORARY IMAGE BUFFERS

=================================================================
*/

enum
{
	TEXTURE_LOADING_BUF0,TEXTURE_LOADING_BUF1,TEXTURE_LOADING_BUF2,TEXTURE_LOADING_BUF3,TEXTURE_LOADING_BUF4,TEXTURE_LOADING_BUF5,
	TEXTURE_RESAMPLING_BUF0,TEXTURE_RESAMPLING_BUF1,TEXTURE_RESAMPLING_BUF2,TEXTURE_RESAMPLING_BUF3,TEXTURE_RESAMPLING_BUF4,TEXTURE_RESAMPLING_BUF5,
	TEXTURE_LINE_BUF,
	TEXTURE_CUT_BUF,
	TEXTURE_FLIPPING_BUF0,TEXTURE_FLIPPING_BUF1,TEXTURE_FLIPPING_BUF2,TEXTURE_FLIPPING_BUF3,TEXTURE_FLIPPING_BUF4,TEXTURE_FLIPPING_BUF5,

	NUM_IMAGE_BUFFERS
};

static uint8_t *r_screenShotBuffer;
static size_t r_screenShotBufferSize;

static uint8_t *r_imageBuffers[NUM_QGL_CONTEXTS][NUM_IMAGE_BUFFERS];
static size_t r_imageBufSize[NUM_QGL_CONTEXTS][NUM_IMAGE_BUFFERS];

#define R_PrepareImageBuffer(ctx,buffer,size) _R_PrepareImageBuffer(ctx,buffer,size,__FILE__,__LINE__)

/*
* R_PrepareImageBuffer
*/
static uint8_t *_R_PrepareImageBuffer( int ctx, int buffer, size_t size, 
	const char *filename, int fileline )
{
	if( r_imageBufSize[ctx][buffer] < size )
	{
		r_imageBufSize[ctx][buffer] = size;
		if( r_imageBuffers[ctx][buffer] )
			Q_Free( r_imageBuffers[ctx][buffer] );
		r_imageBuffers[ctx][buffer] = Q_Malloc( size );
		Q_LinkToPool(r_imageBuffers[ctx][buffer],r_imagesPool);
	}

	memset( r_imageBuffers[ctx][buffer], 255, size );

	return r_imageBuffers[ctx][buffer];
}

/*
* R_FreeImageBuffers
*/
void R_FreeImageBuffers( void )
{
	int i, j;

	for( i = 0; i < NUM_QGL_CONTEXTS; i++ )
		for( j = 0; j < NUM_IMAGE_BUFFERS; j++ )
		{
			if( r_imageBuffers[i][j] )
			{
				Q_Free( r_imageBuffers[i][j] );
				r_imageBuffers[i][j] = NULL;
			}
			r_imageBufSize[i][j] = 0;
		}
}

/*
* R_AllocImageBufferCb
*/
static uint8_t *_R_AllocImageBufferCb( void *ptr, size_t size, const char *filename, int linenum )
{
	loaderCbInfo_t *cbinfo = ptr;
	return _R_PrepareImageBuffer( cbinfo->ctx, cbinfo->side, size, filename, linenum );
}

static void __R_stbi_free_image(void* p) {
	stbi_uc* img = (stbi_uc*) p;
	stbi_image_free(img);
}

// TODO: replace r_texture_buffer_load.h has a replacment 
static bool __R_ReadImageFromDisk_stbi(char *filename, struct texture_buf_s* buffer ) {
	uint8_t* data;
	size_t size = R_LoadFile( filename, ( void ** ) &data );
	if(data == NULL) {
		ri.Com_Printf(S_COLOR_YELLOW "can't resolve file: %s", filename);
		return false;
	}

	struct texture_buf_desc_s desc = {
		.alignment = 1,
	};

	int channelCount = 0;
	int w;
	int h;
	stbi_uc* stbiBuffer = stbi_load_from_memory( data, size, &w, &h, &channelCount, 0 );
	desc.width = w;
	desc.height = h;
	switch(channelCount) {
		case 1:
			desc.def = R_BaseFormatDef(R_FORMAT_A8_UNORM);
			break;
		case 2:
			desc.def = R_BaseFormatDef(R_FORMAT_L8_A8_UNORM);
			break;
		case 3:
			desc.def = R_BaseFormatDef(R_FORMAT_RGB8_UNORM);
			break;
		case 4:
			desc.def = R_BaseFormatDef(R_FORMAT_RGBA8_UNORM);
			break;
		default:
			stbi_image_free(stbiBuffer);
			ri.Com_Printf(S_COLOR_YELLOW "unhandled channel count: %d", channelCount);
			return false;
	}
	R_FreeFile( data );
	const int res = T_AliasTextureBuf_Free(buffer, &desc, stbiBuffer, 0, stbiBuffer, __R_stbi_free_image);
	assert(res == TEXTURE_BUF_SUCCESS);
	return true;
}

/*
* R_FlipTexture
*/
static void R_FlipTexture( const uint8_t *in, uint8_t *out, int width, int height, 
	int samples, bool flipx, bool flipy, bool flipdiagonal )
{
	int i, x, y;
	const uint8_t *p, *line;
	int row_inc = ( flipy ? -samples : samples ) * width, col_inc = ( flipx ? -samples : samples );
	int row_ofs = ( flipy ? ( height - 1 ) * width * samples : 0 ), col_ofs = ( flipx ? ( width - 1 ) * samples : 0 );

	if( !in )
		return;

	if( flipdiagonal )
	{
		for( x = 0, line = in + col_ofs; x < width; x++, line += col_inc )
			for( y = 0, p = line + row_ofs; y < height; y++, p += row_inc, out += samples )
				for( i = 0; i < samples; i++ )
					out[i] = p[i];
	}
	else
	{
		for( y = 0, line = in + row_ofs; y < height; y++, line += row_inc )
			for( x = 0, p = line + col_ofs; x < width; x++, p += col_inc, out += samples )
				for( i = 0; i < samples; i++ )
					out[i] = p[i];
	}
}

/*
* R_ResampleTexture
*/
static void R_ResampleTexture( int ctx, const uint8_t *in, int inwidth, int inheight, uint8_t *out, 
	int outwidth, int outheight, int samples, int alignment )
{
	int i, j, k;
	int inwidthS, outwidthS;
	unsigned int frac, fracstep;
	const uint8_t *inrow, *inrow2, *pix1, *pix2, *pix3, *pix4;
	unsigned *p1, *p2;
	uint8_t *opix;

	if( inwidth == outwidth && inheight == outheight )
	{
		memcpy( out, in, inheight * ALIGN( inwidth * samples, alignment ) );
		return;
	}

	p1 = ( unsigned * )R_PrepareImageBuffer( ctx, TEXTURE_LINE_BUF, outwidth * sizeof( *p1 ) * 2 );
	p2 = p1 + outwidth;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for( i = 0; i < outwidth; i++ )
	{
		p1[i] = samples * ( frac >> 16 );
		frac += fracstep;
	}

	frac = 3 * ( fracstep >> 2 );
	for( i = 0; i < outwidth; i++ )
	{
		p2[i] = samples * ( frac >> 16 );
		frac += fracstep;
	}

	inwidthS = ALIGN( inwidth * samples, alignment );
	outwidthS = ALIGN( outwidth * samples, alignment );
	for( i = 0; i < outheight; i++, out += outwidthS )
	{
		inrow = in + inwidthS * (int)( ( i + 0.25 ) * inheight / outheight );
		inrow2 = in + inwidthS * (int)( ( i + 0.75 ) * inheight / outheight );
		for( j = 0; j < outwidth; j++ )
		{
			pix1 = inrow + p1[j];
			pix2 = inrow + p2[j];
			pix3 = inrow2 + p1[j];
			pix4 = inrow2 + p2[j];
			opix = out + j * samples;

			for( k = 0; k < samples; k++ )
				opix[k] = ( pix1[k] + pix2[k] + pix3[k] + pix4[k] ) >> 2;
		}
	}
}

/*
* R_ResampleTexture16
*
* Assumes 16-bit unpack alignment
*/
static void R_ResampleTexture16( int ctx, const unsigned short *in, int inwidth, int inheight,
	unsigned short *out, int outwidth, int outheight, int rMask, int gMask, int bMask, int aMask )
{
	int i, j;
	int inwidthA, outwidthA;
	unsigned int frac, fracstep;
	const unsigned short *inrow, *inrow2, *pix1, *pix2, *pix3, *pix4;
	unsigned *p1, *p2;
	unsigned short *opix;

	if( inwidth == outwidth && inheight == outheight )
	{
		memcpy( out, in, inheight * ALIGN( inwidth * sizeof( unsigned short ), 4 ) );
		return;
	}

	p1 = ( unsigned * )R_PrepareImageBuffer( ctx, TEXTURE_LINE_BUF, outwidth * sizeof( *p1 ) * 2 );
	p2 = p1 + outwidth;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for( i = 0; i < outwidth; i++ )
	{
		p1[i] = frac >> 16;
		frac += fracstep;
	}

	frac = 3 * ( fracstep >> 2 );
	for( i = 0; i < outwidth; i++ )
	{
		p2[i] = frac >> 16;
		frac += fracstep;
	}

	inwidthA = ALIGN( inwidth, 2 );
	outwidthA = ALIGN( outwidth, 2 );
	for( i = 0; i < outheight; i++, out += outwidthA )
	{
		inrow = in + inwidthA * (int)( ( i + 0.25 ) * inheight / outheight );
		inrow2 = in + inwidthA * (int)( ( i + 0.75 ) * inheight / outheight );
		for( j = 0; j < outwidth; j++ )
		{
			pix1 = inrow + p1[j];
			pix2 = inrow + p2[j];
			pix3 = inrow2 + p1[j];
			pix4 = inrow2 + p2[j];
			opix = out + j;

			*opix =	( ( ( ( *pix1 & rMask ) + ( *pix2 & rMask ) + ( *pix3 & rMask ) + ( *pix4 & rMask ) ) >> 2 ) & rMask ) |
					( ( ( ( *pix1 & gMask ) + ( *pix2 & gMask ) + ( *pix3 & gMask ) + ( *pix4 & gMask ) ) >> 2 ) & gMask ) |
					( ( ( ( *pix1 & bMask ) + ( *pix2 & bMask ) + ( *pix3 & bMask ) + ( *pix4 & bMask ) ) >> 2 ) & bMask ) |
					( ( ( ( *pix1 & aMask ) + ( *pix2 & aMask ) + ( *pix3 & aMask ) + ( *pix4 & aMask ) ) >> 2 ) & aMask );
		}
	}
}

/*
* R_MipMap
* 
* Operates in place, quartering the size of the texture
*/
static void R_MipMap( uint8_t *in, int width, int height, int samples, int alignment )
{
	int i, j, k;
	int instride = ALIGN( width * samples, alignment );
	int outwidth, outheight, outpadding;
	uint8_t *out = in;
	uint8_t *next;
	int inofs;

	outwidth = width >> 1;
	outheight = height >> 1;
	if( !outwidth )
		outwidth = 1;
	if( !outheight )
		outheight = 1;
	outpadding = ALIGN( outwidth * samples, alignment ) - outwidth * samples;

	for( i = 0; i < outheight; i++, in += instride * 2, out += outpadding )
	{
		next = ( ( ( i << 1 ) + 1 ) < height ) ? ( in + instride ) : in;
		for( j = 0, inofs = 0; j < outwidth; j++, inofs += samples )
		{
			if( ( ( j << 1 ) + 1 ) < width )
			{
				for( k = 0; k < samples; ++k, ++inofs )
					*( out++ ) = ( in[inofs] + in[inofs + samples] + next[inofs] + next[inofs + samples] ) >> 2;
			}
			else
			{
				for( k = 0; k < samples; ++k, ++inofs )
					*( out++ ) = ( in[inofs] + next[inofs] ) >> 1;
			}
		}
	}
}

/*
* R_MipMap16
*
* Operates in place, quartering the size of the 16-bit texture, assumes unpack alignment of 4
*/
static void R_MipMap16( unsigned short *in, int width, int height, int rMask, int gMask, int bMask, int aMask )
{
	int i, j;
	int instride = ALIGN( width, 2 );
	int outwidth, outheight, outpadding;
	unsigned short *out = in;
	unsigned short *next;
	int col, p[4];

	outwidth = width >> 1;
	outheight = height >> 1;
	if( !outwidth )
		outwidth = 1;
	if( !outheight )
		outheight = 1;
	outpadding = outwidth & 1;

	for( i = 0; i < outheight; i++, in += instride * 2, out += outpadding )
	{
		next = ( ( ( i << 1 ) + 1 ) < height ) ? ( in + instride ) : in;
		for( j = 0; j < outwidth; j++ )
		{
			col = j << 1;
			p[0] = in[col];
			p[1] = next[col];
			if( ( col + 1 ) < width )
			{
				p[2] = in[col + 1];
				p[3] = next[col + 1];
				*( out++ ) =	( ( ( ( p[0] & rMask ) + ( p[1] & rMask ) + ( p[2] & rMask ) + ( p[3] & rMask ) ) >> 2 ) & rMask ) |
								( ( ( ( p[0] & gMask ) + ( p[1] & gMask ) + ( p[2] & gMask ) + ( p[3] & gMask ) ) >> 2 ) & gMask ) |
								( ( ( ( p[0] & bMask ) + ( p[1] & bMask ) + ( p[2] & bMask ) + ( p[3] & bMask ) ) >> 2 ) & bMask ) |
								( ( ( ( p[0] & aMask ) + ( p[1] & aMask ) + ( p[2] & aMask ) + ( p[3] & aMask ) ) >> 2 ) & aMask );
			}
			else
			{
				*( out++ ) =	( ( ( ( p[0] & rMask ) + ( p[1] & rMask ) ) >> 1 ) & rMask ) |
								( ( ( ( p[0] & gMask ) + ( p[1] & gMask ) ) >> 1 ) & gMask ) |
								( ( ( ( p[0] & bMask ) + ( p[1] & bMask ) ) >> 1 ) & bMask ) |
								( ( ( ( p[0] & aMask ) + ( p[1] & aMask ) ) >> 1 ) & aMask );
			}
		}
	}
}


/*
* R_MipCount
*/
static int R_MipCount( int width, int height, int minmipsize )
{
	int mips = 1;
	while( ( width > minmipsize ) || ( height > minmipsize ) )
	{
		++mips;
		width >>= 1;
		height >>= 1;
		if( !width )
			width = 1;
		if( !height )
			height = 1;
	}
	return mips;
}

static bool R_IsKTXFormatValid( int format, int type )
{
	switch( type )
	{
	case GL_UNSIGNED_BYTE:
		switch( format )
		{
		case GL_RGBA:
		case GL_BGRA_EXT:
		case GL_RGB:
		case GL_BGR_EXT:
		case GL_LUMINANCE_ALPHA:
		case GL_ALPHA:
		case GL_LUMINANCE:
			return true;
		}
		return false;
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_5_5_5_1:
		return ( format == GL_RGBA ) ? true : false;
	case GL_UNSIGNED_SHORT_5_6_5:
		return ( format == GL_RGB ) ? true : false;
	case 0:
		return ( format == GL_ETC1_RGB8_OES ) ? true : false;
	}
	return false;
}

//TODO: move ktx loader to a seperate file
static bool __R_LoadKTX( image_t *image, const char *pathname )
{
	const uint_fast16_t numFaces = ( ( image->flags & IT_CUBEMAP ) ? 6 : 1 );
	if( image->flags & ( IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL ) )
		return false;

	uint8_t *buffer = NULL;
	const size_t bufferSize = R_LoadFile( pathname, ( void ** )&buffer );
	if( !buffer )
		return false;

	struct ktx_context_s ktxContext = {0};
	struct ktx_context_err_s err = {0};
	if( !R_InitKTXContext( &ktxContext, buffer, bufferSize, &err ) ) {
		switch(err.type) {
			case KTX_ERR_INVALID_IDENTIFIER:
				ri.Com_Printf( S_COLOR_RED "R_LoadKTX: Bad file identifier: %s\n", pathname );
				goto error;
			case KTX_ERR_UNHANDLED_TEXTURE_TYPE:
				ri.Com_Printf( S_COLOR_RED "R_LoadKTX: Unhandeled texture (type: %04x internalFormat %04x): %s\n", err.errTextureType.type, err.errTextureType.internalFormat, pathname );
				goto error;
			case KTX_ERR_TRUNCATED:
				ri.Com_Printf( S_COLOR_RED "R_LoadKTX: Truncated Data size:(orignal: %lu expected: %lu): %s\n", err.errTruncated.size, err.errTruncated.expected, pathname );
				goto error;
			case KTX_WARN_MIPLEVEL_TRUNCATED:
				ri.Com_Printf( S_COLOR_YELLOW "R_LoadKTX: Truncated MipLevel size: (orignal: %lu expected: %lu) mip: (orignal: %lu expected: %lu): %s\n", err.errTruncated.size, err.errTruncated.expected,
							   err.mipTruncated.mipLevels, err.mipTruncated.expectedMipLevels, pathname );
				break;
			case KTX_ERR_ZER_TEXTURE_SIZE:
				ri.Com_Printf( S_COLOR_RED "R_LoadKTX: Zero texture size: %s\n", pathname );
				goto error;
				break;
		}
	}

	if( ktxContext.format && ( ktxContext.format != ktxContext.baseInternalFormat ) )
	{
		ri.Com_Printf( S_COLOR_YELLOW "R_LoadKTX: Pixel format doesn't match internal format: %s\n", pathname );
		goto error;
	}
	if( !R_IsKTXFormatValid( ktxContext.format ? ktxContext.baseInternalFormat : ktxContext.internalFormat, ktxContext.type ) )
	{
		ri.Com_Printf( S_COLOR_YELLOW "R_LoadKTX: Unsupported pixel format: %s\n", pathname );
		goto error;
	}
	if( R_KTXIsCompressed(&ktxContext) && ( ( ktxContext.pixelWidth & ( ktxContext.pixelWidth - 1 ) ) || ( ktxContext.pixelHeight & ( ktxContext.pixelHeight - 1 ) ) ) )
	{
		// NPOT compressed textures may crash on certain drivers/GPUs
		ri.Com_Printf( S_COLOR_YELLOW "R_LoadKTX: Compressed image must be power-of-two: %s\n", pathname );
		goto error;
	}
	if( ( image->flags & IT_CUBEMAP ) && ( ktxContext.pixelWidth != ktxContext.pixelHeight ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Not square cubemap image: %s\n", pathname );
		goto error;
	}
	if( ( ktxContext.pixelDepth > 1 ) || ( ktxContext.numberOfArrayElements > 1 ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: 3D textures and texture arrays are not supported: %s\n", pathname );
		goto error;
	}
	if( ktxContext.numberOfFaces != numFaces )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Bad number of cubemap faces: %s\n", pathname );
		goto error;
	}
	const struct base_format_def_s *definition = ktxContext.desc;
	const enum RI_Format_e dstFormat = __R_GetImageFormat(image);
	const uint32_t numberOfFaces = R_KTXGetNumberFaces( &ktxContext );
	const uint16_t minMipSize = __R_calculateMipMapLevel(image->flags, R_KTXWidth(&ktxContext), R_KTXHeight(&ktxContext), image->minmipsize);
	const uint16_t numberOfMipLevels = R_KTXIsCompressed( &ktxContext )  ? 1 : min(minMipSize, R_KTXGetNumberMips( &ktxContext ));
	
	image->extension = extensionKTX;
	image->width = R_KTXWidth(&ktxContext);
	image->height = R_KTXHeight( &ktxContext );
#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
		VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT; // typeless
		const struct RIFormatProps_s *formatProps = GetRIFormatProps( dstFormat );
		if( formatProps->blockWidth > 1 )
			flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
		if( image->flags & IT_CUBEMAP )
			flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
		info.flags = flags;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = RIFormatToVK( dstFormat );
		info.extent.width = image->width;
		info.extent.height = image->height;
		info.extent.depth = 1;
		info.mipLevels = numberOfMipLevels;
		info.arrayLayers = ( image->flags & IT_CUBEMAP ) ? 6 : 1;
		info.samples = 1;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		info.pQueueFamilyIndices = queueFamilies;
		VK_ConfigureImageQueueFamilies( &info, rsh.device.queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocationCreateInfo mem_reqs = {};
		mem_reqs.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		if(!VK_WrapResult(vmaCreateImage(rsh.device.vk.vmaAllocator, &info, &mem_reqs, &image->handle.vk.image, &image->vk.vmaAlloc, NULL))) {
			goto error;
		}
		if(vkSetDebugUtilsObjectNameEXT){
			VkDebugUtilsObjectNameInfoEXT debugName = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, NULL, VK_OBJECT_TYPE_IMAGE, (uint64_t)image->handle.vk.image, image->name.buf };
			VK_WrapResult( vkSetDebugUtilsObjectNameEXT( rsh.device.vk.device, &debugName ) );
		}

		VkImageViewUsageCreateInfo usageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
		usageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageSubresourceRange subresource = {
			VK_IMAGE_ASPECT_COLOR_BIT, 0, Q_MAX(image->mipNum, 1), 0, 1,
		};

		VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		createInfo.pNext = &usageInfo;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = RIFormatToVK( dstFormat );
		createInfo.subresourceRange = subresource;
		createInfo.image = image->handle.vk.image;

		image->binding.flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
		image->binding.texture = &image->handle;
		image->binding.vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		image->binding.vk.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VK_WrapResult( vkCreateImageView( rsh.device.vk.device, &createInfo, NULL, &image->binding.vk.image.imageView ) );
		RI_UpdateDescriptor( &rsh.device, &image->binding );
		image->samplerBinding = R_ResolveSamplerDescriptor( image->flags );
		assert(image->samplerBinding);

		//RI_VK_InitImageView( &rsh.device, &createInfo, &image->binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
	}
#endif
	//NriTextureDesc textureDesc = { 
	//	.width = R_KTXWidth( &ktxContext ),
	//	.height = R_KTXHeight( &ktxContext ),
	//	.usage = __R_NRITextureUsageBits( image->flags ),
	//	.layerNum = ( image->flags & IT_CUBEMAP ) ? 6 : 1,
	//	.depth = 1,
	//	.format = R_ToNRIFormat( dstFormat ),
	//	.sampleNum = 1,
	//	.type = NriTextureType_TEXTURE_2D,
	//	.mipNum = numberOfMipLevels 
	//};

	//NriResourceGroupDesc resourceGroupDesc = {
	//	.textureNum = 1,
	//	.textures = &image->texture,
	//	.memoryLocation = NriMemoryLocation_DEVICE,
	//};

	//if( rsh.nri.coreI.CreateTexture( rsh.nri.device, &textureDesc, &image->texture ) != NriResult_SUCCESS ) {
	//	ri.Com_Printf( S_COLOR_YELLOW "Failed to Create Image: %s\n", image->name );
	//	return false;
	//}
	//
	//const uint32_t allocationNum = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
	//assert( allocationNum <= Q_ARRAY_COUNT( image->memory ) );
	//image->numAllocations = allocationNum;
	//if( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, image->memory ) ) {
	//	ri.Com_Printf( S_COLOR_YELLOW "Failed Allocation: %s\n", image->name );
	//	return false;
	//}

	//NriTexture2DViewDesc textureViewDesc = {
	//	.texture = image->texture,
	//	.viewType = (image->flags & IT_CUBEMAP) ? NriTexture2DViewType_SHADER_RESOURCE_CUBE: NriTexture2DViewType_SHADER_RESOURCE_2D,
	//	.format = textureDesc.format
	//};
	//NriDescriptor* descriptor = NULL;
	//NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &textureViewDesc, &descriptor) );
	//image->descriptor = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
	//image->samplerDescriptor = R_CreateDescriptorWrapper(&rsh.nri, R_ResolveSamplerDescriptor(image->flags)); 
	//assert(image->samplerDescriptor.descriptor);
	//rsh.nri.coreI.SetTextureDebugName( image->texture, image->name.buf );

	if( R_KTXIsCompressed( &ktxContext ) ) {
		struct texture_buf_s uncomp = { 0 };
		for( size_t faceIdx = 0; faceIdx < numFaces; ++faceIdx ) {
			struct texture_buf_s *tex = R_KTXResolveBuffer( &ktxContext, 0, faceIdx, 0 );
			struct texture_buf_desc_s desc = { 
				.width = tex->width, 
				.height = tex->height, 
				.def = R_BaseFormatDef( R_FORMAT_RGB8_UNORM ), 
				.alignment = 1 
			};
			T_ReallocTextureBuf( &uncomp, &desc );
			DecompressETC1( tex->buffer, tex->width, tex->height, uncomp.buffer, false );
			__R_CopyTextureDataTexture(image, faceIdx, 0, 0, 0, uncomp.width, uncomp.height, R_FORMAT_RGB8_UNORM, uncomp.buffer);
			
		}
		T_FreeTextureBuf(&uncomp);
		image->samples = 3;

	} else {
		const enum texture_logical_channel_e expectBGR[] = { R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED };
		const enum texture_logical_channel_e expectBGRA[] = { R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED, R_LOGICAL_C_ALPHA };
		const enum texture_logical_channel_e expectA[] = { R_LOGICAL_C_ALPHA };
		const bool isBGRTexture = RT_ExpectChannelsMatch( definition, expectBGR, Q_ARRAY_COUNT( expectBGR ) ) || RT_ExpectChannelsMatch( definition, expectBGRA, Q_ARRAY_COUNT( expectBGRA ) );
		image->flags |= ( ( RT_ExpectChannelsMatch( definition, expectA, Q_ARRAY_COUNT( expectA ) ) ? IT_ALPHAMASK : 0 ) );
		image->samples = RT_NumberChannels( definition );

		enum texture_logical_channel_e swizzleChannel[R_LOGICAL_C_MAX] = { 0 };
		// R_ResourceTransitionTexture(image->texture, (NriAccessLayoutStage){} );

		for( uint16_t mipIndex = 0; mipIndex < numberOfMipLevels; mipIndex++ ) {
			for( uint32_t faceIndex = 0; faceIndex < numberOfFaces; faceIndex++ ) {
				struct texture_buf_s *texBuffer = R_KTXResolveBuffer( &ktxContext, mipIndex, faceIndex, 0 );
				assert(texBuffer);
				if( isBGRTexture ) {
					const size_t numberChannels = RT_NumberChannels( definition );
					assert( numberChannels >= 3 && numberChannels <= Q_ARRAY_COUNT( swizzleChannel ) );
					memcpy( swizzleChannel, RT_Channels( definition ), numberChannels );
					swizzleChannel[0] = R_LOGICAL_C_RED;
					swizzleChannel[1] = R_LOGICAL_C_GREEN;
					swizzleChannel[2] = R_LOGICAL_C_BLUE;
					T_SwizzleInplace( texBuffer, swizzleChannel );
				}

				__R_CopyTextureDataTexture(image, faceIndex, mipIndex, 0, 0, texBuffer->width, texBuffer->height, definition->format, texBuffer->buffer);
			}
		}
	}


	R_KTXFreeContext(&ktxContext);
	R_FreeFile( buffer );
	R_DeferDataSync();
	return true;
error: // must not be reached after actually starting uploading the texture
	R_KTXFreeContext(&ktxContext);
	R_FreeFile( buffer );
	return false;
}


static struct image_s *__R_AllocImage( const struct QStrSpan name) {
	if( !free_images ) {
		return NULL;
	}
	const unsigned hashIndex = hash_data_hsieh(HASH_INITIAL_VALUE, name.buf, name.len) % IMAGES_HASH_SIZE;
	ri.Mutex_Lock( r_imagesLock );
	image_t *image = free_images;
	free_images = image->next;
	
	memset(image, 0, sizeof(*image));

	// link to the list of active images
	image->prev = &images_hash_headnode[hashIndex];
	image->next = images_hash_headnode[hashIndex].next;
	image->next->prev = image;
	image->prev->next = image;

	ri.Mutex_Unlock( r_imagesLock );

	if( !image ) {
		ri.Com_Error( ERR_DROP, "R_LoadImage: r_numImages == MAX_GLIMAGES" );
	}
	struct QStr value = {0};
	qStrAppendSlice(&value, name);
	image->name = value; 
	image->registrationSequence = rsh.registrationSequence;
	image->extension = NULL;
	image->loaded = true;
	image->missing = false;

	return image;
}

static enum RI_Format_e __R_ResolveDataFormat( int flags, int samples )
{
	assert((flags & (IT_FRAMEBUFFER | IT_DEPTHRB | IT_DEPTH)) == 0);
	if( samples == 4 ) {
		return RI_FORMAT_RGBA8_UNORM;
	} else if( samples == 3 ) {
		return RI_FORMAT_RGB8_UNORM;
	} else if( samples == 2 ) {
		return RI_FORMAT_RG8_UNORM;
	} else if( flags & IT_ALPHAMASK ) {
		return RI_FORMAT_A8_UNORM;
	}
	return RI_FORMAT_R8_UNORM;
}

static enum RI_Format_e __R_GetImageFormat( struct image_s* image )
{
	assert( ( image->flags & ( IT_FRAMEBUFFER | IT_DEPTHRB | IT_DEPTH ) ) == 0 );
	return RI_FORMAT_RGBA8_UNORM;
}


static void __R_CopyTextureDataTexture(struct image_s* image, int layer, int mipOffset, int x, int y, int w, int h, enum RI_Format_e srcFormat, uint8_t *data )
{
	const struct RIFormatProps_s *srcDef = GetRIFormatProps( srcFormat );
	const struct RIFormatProps_s *destDef = GetRIFormatProps( __R_GetImageFormat( image ) );

	struct RIResourceTextureTransaction_s uploadDesc = {};
	uploadDesc.target = image->handle;
	uploadDesc.sliceNum = h;
	uploadDesc.rowPitch = w * destDef->stride;
	uploadDesc.arrayOffset = layer;
	uploadDesc.mipOffset = mipOffset; 
	uploadDesc.x = x;
	uploadDesc.y = y;
	uploadDesc.depth = 1;
	uploadDesc.width = w; 
	uploadDesc.height = h;
	uploadDesc.format = __R_GetImageFormat(image);
#if ( DEVICE_IMPL_VULKAN )
	{
		uploadDesc.postBarrier.vk.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		uploadDesc.postBarrier.vk.stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		uploadDesc.postBarrier.vk.access = VK_ACCESS_2_SHADER_READ_BIT;
	}
#endif

	RI_ResourceBeginCopyTexture( &rsh.device, &rsh.uploader, &uploadDesc );
	for( size_t slice = 0; slice < uploadDesc.height; slice++ ) {
		const size_t dstRowStart = uploadDesc.alignRowPitch * slice;
		memset( &( (uint8_t *)uploadDesc.data )[dstRowStart], 255, uploadDesc.rowPitch );
		for( size_t column = 0; column < uploadDesc.width; column++ ) {
			switch( srcFormat ) {
				case RI_FORMAT_L8_A8_UNORM: {
					const uint8_t luminance = data[( uploadDesc.width * srcDef->stride * slice ) + ( column * srcDef->stride )];
					const uint8_t alpha = data[( uploadDesc.width * srcDef->stride * slice ) + ( column * srcDef->stride ) + 1];
					uint8_t color[4] = { luminance, luminance, luminance, alpha };
					memcpy( &( (uint8_t *)uploadDesc.data )[dstRowStart + ( destDef->stride * column )], color, Q_MIN( sizeof( color ), destDef->stride ) );
					break;
				}
				case RI_FORMAT_A8_UNORM:
				case RI_FORMAT_R8_UNORM: {
					const uint8_t c1 = data[( uploadDesc.width * srcDef->stride * slice ) + ( column * srcDef->stride )];
					uint8_t color[4]; //= { c1, c1, c1, c1 };
					if( image->flags & IT_ALPHAMASK ) {
						color[0] = color[1] = color[2] = 0.0f;
						color[3] = c1;
					} else {
						color[0] = color[1] = color[2] = c1;
						color[3] = 255;
					}
					memcpy( &( (uint8_t *)uploadDesc.data )[dstRowStart + ( destDef->stride * column )], color, min( 4, destDef->stride ) );
					break;
				}
				default:
					memcpy( &( (uint8_t *)uploadDesc.data )[dstRowStart + ( destDef->stride * column )], &data[( uploadDesc.width * srcDef->stride * slice ) + ( column * srcDef->stride )],
							min( srcDef->stride, destDef->stride ) );
					break;
			}
		}
	}
	RI_ResourceEndCopyTexture( &rsh.device, &rsh.uploader, &uploadDesc );
}

static uint16_t __R_calculateMipMapLevel(int flags, int width, int height, uint32_t minMipSize) {

	if( !( flags & IT_NOPICMIP ) )
	{
		// let people sample down the sky textures for speed
		uint16_t mip = 1;
		int picmip = ( flags & IT_SKY ) ? r_skymip->integer : r_picmip->integer;
		while( ( mip < picmip ) && ( ( width > minMipSize ) || ( height > minMipSize) ) )
		{
			++mip;
			width >>= 1;
			height >>= 1;
			if( !width )
				width = 1;
			if( !height )
				height = 1;
		}
		return max(1, mip);
	}
	return  max(1,( flags & IT_NOMIPMAP ) ? 1 :ceil(log2(max(width, height))));
}


struct image_s *R_LoadImage( const char *name, uint8_t **pic, int width, int height, int flags, int minmipsize, int tags, int samples )
{
	
	struct image_s *image = __R_AllocImage( qCToStrRef(name) );
	
	image->width = width;
	image->height = height;
	image->layers = 1;
	image->flags = flags;
	image->minmipsize = minmipsize;
	image->tags = tags;
	image->samples = samples;
	image->width = width;
	image->height = height;
	image->mipNum = __R_calculateMipMapLevel( flags, width, height, minmipsize );

	enum RI_Format_e srcFormat = __R_ResolveDataFormat( flags, samples );
	enum RI_Format_e destFormat = __R_GetImageFormat( image );
	
	uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	info.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT; // typeless
	const struct RIFormatProps_s *formatProps = GetRIFormatProps( destFormat );
	if( formatProps->blockWidth > 1 )
		info.flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
	if( image->flags & IT_CUBEMAP )
		info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = RIFormatToVK( destFormat );
	info.extent.width = image->width;
	info.extent.height = image->height;
	info.extent.depth = 1;
	info.mipLevels = image->mipNum;
	info.arrayLayers = ( image->flags & IT_CUBEMAP ) ? 6 : 1;
	info.samples = 1;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	info.pQueueFamilyIndices = queueFamilies;
	VK_ConfigureImageQueueFamilies( &info, rsh.device.queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	VmaAllocationCreateInfo memCreateInfo = { 0 };
	memCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	if(!VK_WrapResult(vmaCreateImage(rsh.device.vk.vmaAllocator, &info, &memCreateInfo, &image->handle.vk.image, &image->vk.vmaAlloc, NULL))) {
		ri.Com_Printf( S_COLOR_YELLOW "Failed to Create Image: %s\n", image->name.buf );
		__FreeImage( image );
		image = NULL;
		return NULL;
	}
	if( vkSetDebugUtilsObjectNameEXT ) {
		VkDebugUtilsObjectNameInfoEXT debugName = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, NULL, VK_OBJECT_TYPE_IMAGE, (uint64_t)image->handle.vk.image, name };
		VK_WrapResult( vkSetDebugUtilsObjectNameEXT( rsh.device.vk.device, &debugName ) );
	}

	// create desctipror
	VkImageViewUsageCreateInfo usageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
	usageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

	VkImageSubresourceRange subresource = {
		VK_IMAGE_ASPECT_COLOR_BIT, 0, info.mipLevels, 0, info.arrayLayers,
	};

	VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.pNext = &usageInfo;
	createInfo.viewType = ( image->flags & IT_CUBEMAP ) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = RIFormatToVK( destFormat );
	createInfo.subresourceRange = subresource;
	createInfo.image = image->handle.vk.image;

	image->binding.flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
	image->binding.texture = &image->handle;
	image->binding.vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	image->binding.vk.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VK_WrapResult( vkCreateImageView( rsh.device.vk.device, &createInfo, NULL, &image->binding.vk.image.imageView ) );
	RI_UpdateDescriptor( &rsh.device, &image->binding );
	image->samplerBinding = R_ResolveSamplerDescriptor(image->flags); 

	//RI_VK_InitImageView(&rsh.device, &createInfo, &image->binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	const size_t reservedSize = width * height * samples;
	uint8_t *tmpBuffer = NULL;
	if( pic ) {
		for( size_t index = 0; index < info.arrayLayers; index++ ) {
			if( !pic[index] ) {
				continue;
			}
			uint8_t *buf = pic[index];
			if( !( flags & IT_CUBEMAP ) && ( flags & ( IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL ) ) ) {
				arrsetlen( tmpBuffer, reservedSize );
				R_FlipTexture( buf, tmpBuffer, width, height, samples, ( flags & IT_FLIPX ) ? true : false, ( flags & IT_FLIPY ) ? true : false, ( flags & IT_FLIPDIAGONAL ) ? true : false );
				buf = tmpBuffer;
			}

			uint32_t w = width;
			uint32_t h = height;
			for( size_t i = 0; i < info.mipLevels; i++ ) {
				__R_CopyTextureDataTexture(image, index, i, 0, 0, w, h, srcFormat, buf);
				w >>= 1;
				h >>= 1;
				if( w == 0 ) {
					w = 1;
				}
				if( h == 0 ) {
					h = 1;
				}
				R_MipMap( buf, w, h, samples, 1 );
			}
		}
	}
	arrfree( tmpBuffer );
	return image;
}

image_t *R_CreateImage( const char *name, int width, int height, int layers, int flags, int minmipsize, int tags, int samples )
{
	image_t* image = __R_AllocImage(qCToStrRef(name));
	if( !image ) {
		ri.Com_Error( ERR_DROP, "R_LoadImage: r_numImages == MAX_GLIMAGES" );
	}
	
	image->width = width;
	image->height = height;
	image->layers = layers;
	image->flags = flags;
	image->minmipsize = minmipsize;
	image->samples = samples;
	image->registrationSequence = rsh.registrationSequence;
	image->tags = tags;
	image->loaded = true;
	image->missing = false;
	image->extension = '\0';

	return image;
}

static void __FreeImage( struct image_s *image )
{
	{
		__FreeGPUImageData( image );
		// R_ReleaseNriTexture(image);
		memset(&image->binding, 0, sizeof(struct RIDescriptor_s));
		//memset(&image->samplerDescriptor, 0, sizeof(struct  nri_descriptor_s));
	
		qStrFree(&image->name);
		image->flags = 0;
		image->loaded = false;
		image->tags = 0;
		image->registrationSequence = 0;
		image->samplerBinding = NULL;
		//image->texture = NULL;
		//image->descriptor = (struct nri_descriptor_s){0};
		//image->samplerDescriptor= (struct nri_descriptor_s){0};
		//image->numAllocations = 0;
	}
	{
		ri.Mutex_Lock( r_imagesLock );
		// remove from linked active list
		image->prev->next = image->next;
		image->next->prev = image->prev;

		// insert into linked free list
		image->next = free_images;
		free_images = image;
		image->prev = NULL;
		ri.Mutex_Unlock( r_imagesLock );
	}
}

void R_ReplaceImage( image_t *image, uint8_t **pic, int width, int height, int flags, int minmipsize, int samples )
{
	assert( image );
	// const NriTextureDesc *textureDesc = rsh.nri.coreI.GetTextureDesc( image->texture );
	uint32_t mipNum = __R_calculateMipMapLevel( flags, width, height, minmipsize );
	if( image->width != width || image->height != height || image->samples != samples || image->mipNum != mipNum ) {
		// struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
		__FreeGPUImageData( image );
		enum RI_Format_e destFormat = __R_GetImageFormat( image );
#if ( DEVICE_IMPL_VULKAN )
		uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
		VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT; // typeless
		const struct RIFormatProps_s *formatProps = GetRIFormatProps( destFormat );
		if( formatProps->blockWidth > 1 )
			flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
		if( flags & IT_CUBEMAP )
			flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
		info.flags = flags;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = RIFormatToVK( destFormat );
		info.extent.width = width;
		info.extent.height = height;
		info.extent.depth = 1;
		info.mipLevels = mipNum;
		info.arrayLayers = ( flags & IT_CUBEMAP ) ? 6 : 1;
		info.samples = 1;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		info.pQueueFamilyIndices = queueFamilies;
		VK_ConfigureImageQueueFamilies( &info, rsh.device.queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if( !VK_WrapResult( vkCreateImage( rsh.device.vk.device, &info, NULL, &image->handle.vk.image ) ) ) {
			ri.Com_Printf( S_COLOR_YELLOW "Failed to Create Image: %s\n", image->name.buf );
			__FreeImage( image );
			image = NULL;
			return;
		}

		// create desctipror
		VkImageViewUsageCreateInfo usageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
		usageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageSubresourceRange subresource = {
			VK_IMAGE_ASPECT_COLOR_BIT, 0, mipNum, 0, 1,
		};

		VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		createInfo.pNext = &usageInfo;
		createInfo.viewType = ( image->flags & IT_CUBEMAP ) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = RIFormatToVK( destFormat );
		createInfo.subresourceRange = subresource;
		createInfo.image = image->handle.vk.image;
	
		image->binding.flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
		image->binding.texture = &image->handle;
		image->binding.vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		image->binding.vk.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VK_WrapResult( vkCreateImageView( rsh.device.vk.device, &createInfo, NULL, &image->binding.vk.image.imageView ) );
		RI_UpdateDescriptor( &rsh.device, &image->binding );
		image->samplerBinding = R_ResolveSamplerDescriptor( image->flags);
		assert( image->samplerBinding && !RI_IsEmptyDescriptor( image->samplerBinding ) );

		//RI_VK_InitImageView( &rsh.device, &createInfo, &image->binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE );
#else
#error unhandled
#endif
	}
	image->flags = flags;
	image->samples = samples;
	image->minmipsize = minmipsize;

	R_ReplaceSubImage( image, 0, 0, 0, pic, width, height );
}

/*
* R_ReplaceSubImage
*/
void R_ReplaceSubImage( image_t *image, int layer, int x, int y, uint8_t **pic, int width, int height )
{
	assert( image );

	const size_t reservedSize = width * height * image->samples;
	uint8_t *buf = NULL;
	arrsetlen( buf, reservedSize );
	memcpy( buf, pic[0], reservedSize );

	// uint8_t *buf = tmpBuffer;
	enum RI_Format_e  srcFormat = __R_ResolveDataFormat( image->flags, image->samples );

	uint8_t *tmpBuffer = NULL;
	if( !( image->flags & IT_CUBEMAP ) && ( image->flags & ( IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL ) ) ) {
		R_FlipTexture( buf, tmpBuffer, width, height, image->samples, ( image->flags & IT_FLIPX ) ? true : false, ( image->flags & IT_FLIPY ) ? true : false,
					   ( image->flags & IT_FLIPDIAGONAL ) ? true : false );
		buf = tmpBuffer;
	}

	uint32_t w = width;
	uint32_t h = height;
	// R_ResourceTransitionTexture(image->texture,(NriAccessLayoutStage){} );
	for( size_t i = 0; i < image->mipNum; i++ ) {
		__R_CopyTextureDataTexture(image, layer, i, x, y, w, h, srcFormat, buf);
		w >>= 1;
		h >>= 1;
		if( w == 0 ) {
			w = 1;
		}
		if( h == 0 ) {
			h = 1;
		}
		R_MipMap( buf, w, h, image->samples, 1 );
	}
	arrfree( buf );

	if( !(image->flags & IT_NO_DATA_SYNC) )
		R_DeferDataSync();

	image->registrationSequence = rsh.registrationSequence;
}

/*
* R_ReplaceImageLayer
*/
void R_ReplaceImageLayer( image_t *image, int layer, uint8_t **pic )
{
	assert( image );
	//assert( image->texture );

	const size_t reservedSize = image->width * image->height * image->samples;
	uint8_t *buf = NULL;
	arrsetlen( buf, reservedSize );
	memcpy( buf, pic[0], reservedSize );

	// uint8_t *buf = tmpBuffer;
	enum RI_Format_e srcFormat = __R_ResolveDataFormat( image->flags, image->samples );

	uint8_t *tmpBuffer = NULL;
	if( !( image->flags & IT_CUBEMAP ) && ( image->flags & ( IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL ) ) ) {
		R_FlipTexture( buf, tmpBuffer, image->width, image->height, image->samples, ( image->flags & IT_FLIPX ) ? true : false, ( image->flags & IT_FLIPY ) ? true : false,
					   ( image->flags & IT_FLIPDIAGONAL ) ? true : false );
		buf = tmpBuffer;
	}

	uint32_t w = image->width;
	uint32_t h = image->height;
	// R_ResourceTransitionTexture(image->texture,(NriAccessLayoutStage){} );
	for( size_t i = 0; i < image->mipNum; i++ ) {
		__R_CopyTextureDataTexture( image, layer, i, 0, 0, image->width, image->height, srcFormat, buf );
		w >>= 1;
		h >>= 1;
		if( w == 0 ) {
			w = 1;
		}
		if( h == 0 ) {
			h = 1;
		}
		R_MipMap( buf, w, h, image->samples, 1 );
	}
	arrfree( buf );

	if( !(image->flags & IT_NO_DATA_SYNC) )
		R_DeferDataSync();

	image->registrationSequence = rsh.registrationSequence;
}

/*
* R_FindImage
* 
* Finds and loads the given image. IT_SYNC images are loaded synchronously.
* For synchronous missing images, NULL is returned.
*/
image_t	*R_FindImage( const char *name, const char *suffix, int flags, int minmipsize, int tags )
{
	assert( name );
	assert( name[0] );
	struct UploadImgBuffer {
		struct texture_buf_s buffer; 
		int flags;
	} uploads[6] = { 0 };
	size_t uploadCount = 0;

	const size_t reserveSize = strlen( name ) + ( suffix ? strlen( suffix ) : 0 ) + 15;

	struct QStr resolvedPath = {0};
	qStrSetResv(&resolvedPath, reserveSize);
	{
		int lastDot = -1;
		int lastSlash = -1;
		for( size_t i = ( name[0] == '/' || name[0] == '\\' ); name[i]; i++ ) {
			const char c = name[i];
			if( c == '\\' ) {
				qStrAppendChar( &resolvedPath, '/' );
			} else {
				qStrAppendChar( &resolvedPath, tolower( c ));
			}
			switch( c ) {
				case '.':
					lastDot = i;
					break;
				case '/':
					lastSlash = i;
					break;
			}
		}
		// don't confuse paths such as /ui/xyz.cache/123 with file extensions
		if( lastDot >= lastSlash && lastDot >= 0) {
			// truncate string omitting the extension
			qStrSetLen( &resolvedPath, lastDot );
		}
	}
	if( suffix ) {
		for( size_t i = 0; suffix[i]; i++ ) {
			qStrAppendChar( &resolvedPath, tolower( suffix[i] ));
		}
	}

	const uint32_t basePathLen = resolvedPath.len;
	qStrSetLen( &resolvedPath, basePathLen);
	qStrSetNullTerm(&resolvedPath);

	image_t	*image = NULL;

	const unsigned key  = hash_data_hsieh(HASH_INITIAL_VALUE, resolvedPath.buf, resolvedPath.len) % IMAGES_HASH_SIZE;
	const image_t* hnode = &images_hash_headnode[key];
	const int searchFlags = flags & ~IT_LOADFLAGS;
	for( image = hnode->prev; image != hnode; image = image->prev )
	{
		if( ( ( image->flags & ~IT_LOADFLAGS ) == searchFlags ) &&
			!qStrCompare( qToStrRef(image->name), qToStrRef(resolvedPath)) && ( image->minmipsize == minmipsize ) ) {
			R_TouchImage( image, tags );
			goto done;
		}
	}
	
	qStrSetLen( &resolvedPath, basePathLen);
	qStrSetNullTerm(&resolvedPath);

	image = __R_AllocImage( qToStrRef(resolvedPath));
	image->layers = 1;
	image->flags = flags;
	image->minmipsize = minmipsize;
	image->tags = tags;

	const char *extensions[] = {
			extensionTGA,
			extensionJPG,
			extensionPNG,
			extensionWAL,
			extensionKTX
	};
	
	qStrSetNullTerm(&resolvedPath);
	const char *ext = FS_FirstExtension2( resolvedPath.buf, extensions, Q_ARRAY_COUNT( extensions ) ); // last is KTX
	if( ext != NULL ) {
		qStrAppendSlice(&resolvedPath, qCToStrRef(ext));
		image->extension = ext;
	}
		
	qStrSetNullTerm(&resolvedPath);
	if( ext == extensionKTX && __R_LoadKTX( image, resolvedPath.buf ) ) {
		goto done;
	}

	if( flags & IT_CUBEMAP ) {
		static struct cubemapSufAndFlip {
			char *suf;
			int flags;
		} cubemapSides[2][6] = {
			{ 
				{ "px", 0 }, 
				{ "nx", 0 }, 
				{ "py", 0 }, 
				{ "ny", 0 }, 
				{ "pz", 0 }, 
				{ "nz", 0 } 
			},
			{ 
				{ "rt", IT_FLIPDIAGONAL }, 
				{ "lf", IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL }, 
				{ "bk", IT_FLIPY }, 
				{ "ft", IT_FLIPX }, 
				{ "up", IT_FLIPDIAGONAL }, 
				{ "dn", IT_FLIPDIAGONAL } 
		} };

		for( size_t i = 0; i < 2; i++ ) {
			for( size_t u = 0; u < 6; u++ ) {
				qStrSetLen( &resolvedPath, basePathLen);
				qstrcatfmt(&resolvedPath, "_%s.tga", cubemapSides[i][u].suf );
				qStrSetNullTerm(&resolvedPath);
				if(!T_LoadImageSTBI(resolvedPath.buf, &uploads[u].buffer)) {
					ri.Com_DPrintf( S_COLOR_YELLOW "failed to load image %s\n", resolvedPath );
					break;
				}

				if( uploads[u].buffer.width != uploads[u].buffer.height ) {
					ri.Com_DPrintf( S_COLOR_YELLOW "Not square cubemap image %s\n", resolvedPath );
					break;
				}
				if( uploads[u].buffer.width != uploads[u].buffer.width ) {
					ri.Com_DPrintf( S_COLOR_YELLOW "Different cubemap image size: %s\n", resolvedPath );
					break;
				}
				
				image->width = image->width = T_PixelW(&uploads[u].buffer);
				image->height = image->height = T_PixelH(&uploads[u].buffer);
				image->samples =  RT_NumberChannels(uploads[u].buffer.def);
				image->extension = extensionTGA;

				uploads[u].flags = cubemapSides[i][u].flags;
				uploadCount++;
			}
		}
		qStrSetLen( &resolvedPath, basePathLen); // truncate the pathext + cubemap
		qStrSetNullTerm(&resolvedPath);
		if( uploadCount != 6 ) {
			ri.Com_DPrintf( S_COLOR_YELLOW "Missing image: %s\n", image->name.buf );
			__FreeImage( image );
			image = NULL;
			goto done;
		}
	} else {
	
		struct UploadImgBuffer *upload = &uploads[uploadCount++];
		upload->flags = flags;
		qStrSetNullTerm(&resolvedPath);
		if( ( ext == extensionPNG || ext == extensionJPG || ext == extensionTGA ) && T_LoadImageSTBI( resolvedPath.buf, &upload->buffer ) ) {
			image->width = image->width = T_PixelW( &upload->buffer );
			image->height = image->height = T_PixelH( &upload->buffer );
			image->samples = RT_NumberChannels( upload->buffer.def );
		}  else if( ext == extensionWAL && T_LoadImageWAL( resolvedPath.buf, &upload->buffer ) ) {
			image->width = image->width = T_PixelW( &upload->buffer );
			image->height = image->height = T_PixelH( &upload->buffer );
			image->samples = RT_NumberChannels( upload->buffer.def );
		} else {
			ri.Com_Printf( S_COLOR_YELLOW "Missing image: %s\n", image->name.buf );
			__FreeImage( image );
			image = NULL;
			goto done;
		}
	}
	assert( uploadCount <= 6 );
	assert( ( flags & IT_CUBEMAP && uploadCount == 6 ) || ( !( flags & IT_CUBEMAP ) && uploadCount == 1 ) );

	const uint32_t mipSize = __R_calculateMipMapLevel( flags, uploads[0].buffer.width, uploads[0].buffer.height, minmipsize );
	const uint32_t destFormat = __R_GetImageFormat( image );

#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
		VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT; // typeless
		const struct RIFormatProps_s *formatProps = GetRIFormatProps( destFormat );
		if( formatProps->blockWidth > 1 )
			flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
		if( image->flags & IT_CUBEMAP )
			flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
		info.flags = flags;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = RIFormatToVK( destFormat );
		info.extent.width = image->width;
		info.extent.height = image->height;
		info.extent.depth = 1;
		info.mipLevels = mipSize;
		info.arrayLayers = ( image->flags & IT_CUBEMAP ) ? 6 : 1;
		info.samples = 1;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		info.pQueueFamilyIndices = queueFamilies;
		VK_ConfigureImageQueueFamilies( &info, rsh.device.queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// allocate vma and bind dedicated VMA
		VmaAllocationCreateInfo mem_reqs = { 0 };
		mem_reqs.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		if( !VK_WrapResult( vmaCreateImage( rsh.device.vk.vmaAllocator, &info, &mem_reqs, &image->handle.vk.image, &image->vk.vmaAlloc, NULL ) ) ) {
			ri.Com_Printf( S_COLOR_YELLOW "Failed to Create Image: %s\n", image->name.buf );
			__FreeImage( image );
			image = NULL;
			goto done;
		}
		if(vkSetDebugUtilsObjectNameEXT){
			VkDebugUtilsObjectNameInfoEXT debugName = { 
				VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, 
				NULL, 
				VK_OBJECT_TYPE_IMAGE, 
				(uint64_t)image->handle.vk.image, 
				name 
			};
			VK_WrapResult( vkSetDebugUtilsObjectNameEXT( rsh.device.vk.device, &debugName ) );
		}

		// create desctipror
		VkImageViewUsageCreateInfo usageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
		usageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageSubresourceRange subresource = {
			VK_IMAGE_ASPECT_COLOR_BIT, 0, info.mipLevels, 0, info.arrayLayers,
		};

		VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		createInfo.pNext = &usageInfo;
		createInfo.viewType = ( image->flags & IT_CUBEMAP ) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = info.format;
		createInfo.subresourceRange = subresource;
		createInfo.image = image->handle.vk.image;
		
		image->binding.flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
		image->binding.texture = &image->handle;
		image->binding.vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		image->binding.vk.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VK_WrapResult( vkCreateImageView( rsh.device.vk.device, &createInfo, NULL, &image->binding.vk.image.imageView ) );
		RI_UpdateDescriptor( &rsh.device, &image->binding );
		image->samplerBinding = R_ResolveSamplerDescriptor( image->flags);
		assert( image->samplerBinding && !RI_IsEmptyDescriptor( image->samplerBinding ) );
		
		//RI_VK_InitImageView( &rsh.device, &createInfo, &image->binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE );
	}
#endif

	struct texture_buf_s transformBuffer = { 0 };
	for( size_t index = 0; index < uploadCount; index++ ) {
		const uint8_t channelCount = RT_NumberChannels( uploads[index].buffer.def );
		assert(image->samples == 0 || image->samples == channelCount);
		image->samples = channelCount;

		struct texture_buf_s* src = &uploads[index].buffer;
		if( uploads[index].flags & ( IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL ) ) {
			struct texture_buf_desc_s desc = {
				.width = uploads[index].buffer.width, 
				.height = uploads[index].buffer.height,
				.def = uploads[index].buffer.def, 
				.alignment = 1 
			};
			T_ReallocTextureBuf(&transformBuffer, &desc);
			R_FlipTexture( 
				uploads[index].buffer.buffer, transformBuffer.buffer, 
				uploads[index].buffer.width, uploads[index].buffer.height, 
				channelCount, 
				( uploads[index].flags & IT_FLIPX ) ? true : false,
				( uploads[index].flags & IT_FLIPY ) ? true : false, 
				( uploads[index].flags & IT_FLIPDIAGONAL ) ? true : false );
			src = &transformBuffer;
		}

		// R_ResourceTransitionTexture(image->texture, (NriAccessLayoutStage){} );
		for( size_t i = 0; i < mipSize; i++ ) {
			__R_CopyTextureDataTexture(image, index, i, 0, 0, src->width, src->height, src->def->format, src->buffer);
			R_MipMap( src->buffer, src->width, src->height, channelCount, 1 );
			src->width >>= 1;
			src->height >>= 1;
			if( src->width == 0 ) {
				src->width = 1;
			}
			if( src->height == 0 ) {
				src->height = 1;
			}
		}
	}

done:
	assert(uploadCount <= 6);
	for( size_t i = 0; i < uploadCount; i++ ) {
		T_FreeTextureBuf(&uploads[i].buffer);
	}
	qStrFree(&resolvedPath);
	return image;
}

/*
* R_ScreenShot
*/
void R_ScreenShot( const char *filename, int x, int y, int width, int height,
				   bool flipx, bool flipy, bool flipdiagonal, bool silent ) {
	size_t size, buf_size;
	uint8_t *buffer, *flipped, *rgb, *rgba;
	r_imginfo_t imginfo;
	const char *extension;

	extension = COM_FileExtension( filename );
	if( !extension ) {
		Com_Printf( "R_ScreenShot: Invalid filename\n" );
		return;
	}

	size = width * height * 3;
	// add extra space incase we need to flip the screenshot
	buf_size = width * height * 4 + size;
	if( buf_size > r_screenShotBufferSize ) {
		if( r_screenShotBuffer ) {
			Q_Free( r_screenShotBuffer );
		}
		r_screenShotBuffer = Q_Malloc(buf_size);
		Q_LinkToPool(r_screenShotBuffer, r_imagesPool);
		r_screenShotBufferSize = buf_size;
	}

	buffer = r_screenShotBuffer;
	if( flipx || flipy || flipdiagonal ) {
		flipped = buffer + size;
	} else {
		flipped = NULL;
	}

	imginfo.width = width;
	imginfo.height = height;
	imginfo.samples = 3;
	imginfo.pixels = flipped ? flipped : buffer;

	qglReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer );

	rgb = rgba = buffer;
	while( ( size_t )( rgb - buffer ) < size ) {
		*( rgb++ ) = *( rgba++ );
		*( rgb++ ) = *( rgba++ );
		*( rgb++ ) = *( rgba++ );
		rgba++;
	}

	if( flipped ) {
		R_FlipTexture( buffer, flipped, width, height, 3,
					   flipx, flipy, flipdiagonal );
	}

		if( WriteScreenShot( filename, &imginfo, r_screenshot_format->integer ) && !silent ) {
		Com_Printf( "Wrote %s\n", filename );
	}
}

//=======================================================

/*
* R_InitNoTexture
*/
static void R_InitNoTexture( int *w, int *h, int *flags, int *samples )
{
	int x, y;
	uint8_t *data;
	uint8_t dottexture[8][8] =
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 1, 1, 0, 0, 0, 0 },
		{ 0, 1, 1, 1, 1, 0, 0, 0 },
		{ 0, 1, 1, 1, 1, 0, 0, 0 },
		{ 0, 0, 1, 1, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	//
	// also use this for bad textures, but without alpha
	//
	*w = *h = 8;
	*flags = 0;
	*samples = 3;

	// ch : check samples
	data = R_PrepareImageBuffer( QGL_CONTEXT_MAIN, TEXTURE_LOADING_BUF0, 8 * 8 * 3 );
	for( x = 0; x < 8; x++ )
	{
		for( y = 0; y < 8; y++ )
		{
			data[( y*8 + x )*3+0] = dottexture[x&3][y&3]*127;
			data[( y*8 + x )*3+1] = dottexture[x&3][y&3]*127;
			data[( y*8 + x )*3+2] = dottexture[x&3][y&3]*127;
		}
	}
}

/*
* R_InitSolidColorTexture
*/
static uint8_t *R_InitSolidColorTexture( int *w, int *h, int *flags, int *samples, int color )
{
	uint8_t *data;

	//
	// solid color texture
	//
	*w = *h = 1;
	*flags = IT_NOPICMIP|IT_NOCOMPRESS;
	*samples = 3;

	// ch : check samples
	data = R_PrepareImageBuffer( QGL_CONTEXT_MAIN, TEXTURE_LOADING_BUF0, 1 * 1 * 3 );
	data[0] = data[1] = data[2] = color;
	return data;
}

/*
* R_InitParticleTexture
*/
static void R_InitParticleTexture( int *w, int *h, int *flags, int *samples )
{
	int x, y;
	int dx2, dy, d;
	float dd2;
	uint8_t *data;

	//
	// particle texture
	//
	*w = *h = 16;
	*flags = IT_NOPICMIP|IT_NOMIPMAP;
	*samples = 4;

	data = R_PrepareImageBuffer( QGL_CONTEXT_MAIN, TEXTURE_LOADING_BUF0, 16 * 16 * 4 );
	for( x = 0; x < 16; x++ )
	{
		dx2 = x - 8;
		dx2 = dx2 * dx2;

		for( y = 0; y < 16; y++ )
		{
			dy = y - 8;
			dd2 = dx2 + dy * dy;
			d = 255 - 35 * sqrt( dd2 );
			data[( y*16 + x ) * 4 + 3] = bound( 0, d, 255 );
		}
	}
}

/*
* R_InitWhiteTexture
*/
static void R_InitWhiteTexture( int *w, int *h, int *flags, int *samples )
{
	R_InitSolidColorTexture( w, h, flags, samples, 255 );
}

/*
* R_InitWhiteCubemapTexture
*/
static void R_InitWhiteCubemapTexture( int *w, int *h, int *flags, int *samples )
{
	int i;

	*w = *h = 1;
	*flags = IT_NOPICMIP|IT_NOCOMPRESS|IT_CUBEMAP;
	*samples = 3;

	for( i = 0; i < 6; i++ ) {
		uint8_t *data;
		data = R_PrepareImageBuffer( QGL_CONTEXT_MAIN, TEXTURE_LOADING_BUF0+i, 1 * 1 * 3 );
		data[0] = data[1] = data[2] = 255;
	}
}

/*
* R_InitBlackTexture
*/
static void R_InitBlackTexture( int *w, int *h, int *flags, int *samples )
{
	R_InitSolidColorTexture( w, h, flags, samples, 0 );
}

/*
* R_InitGreyTexture
*/
static void R_InitGreyTexture( int *w, int *h, int *flags, int *samples )
{
	R_InitSolidColorTexture( w, h, flags, samples, 127 );
}

/*
* R_InitBlankBumpTexture
*/
static void R_InitBlankBumpTexture( int *w, int *h, int *flags, int *samples )
{
	uint8_t *data = R_InitSolidColorTexture( w, h, flags, samples, 128 );

/*
	data[0] = 128;	// normal X
	data[1] = 128;	// normal Y
*/
	data[2] = 255;	// normal Z
	data[3] = 128;	// height
}

/*
* R_InitCoronaTexture
*/
static void R_InitCoronaTexture( int *w, int *h, int *flags, int *samples )
{
	int x, y, a;
	float dx, dy;
	uint8_t *data;

	//
	// light corona texture
	//
	*w = *h = 32;
	*flags = IT_SPECIAL;
	*samples = 4;

	data = R_PrepareImageBuffer( QGL_CONTEXT_MAIN, TEXTURE_LOADING_BUF0, 32 * 32 * 4 );
	for( y = 0; y < 32; y++ )
	{
		dy = ( y - 15.5f ) * ( 1.0f / 16.0f );
		for( x = 0; x < 32; x++ )
		{
			dx = ( x - 15.5f ) * ( 1.0f / 16.0f );
			a = (int)( ( ( 1.0f / ( dx * dx + dy * dy + 0.2f ) ) - ( 1.0f / ( 1.0f + 0.2 ) ) ) * 32.0f / ( 1.0f / ( 1.0f + 0.2 ) ) );
			clamp( a, 0, 255 );
			data[( y*32+x )*4+0] = data[( y*32+x )*4+1] = data[( y*32+x )*4+2] = a;
		}
	}
}


/*
* R_ReleaseBuiltinImages
*/
static void R_ReleaseBuiltinImages( void )
{
	rsh.rawTexture = NULL;
	rsh.rawYUVTextures[0] = rsh.rawYUVTextures[1] = rsh.rawYUVTextures[2] = NULL;
	rsh.noTexture = NULL;
	rsh.whiteTexture = rsh.blackTexture = rsh.greyTexture = NULL;
	rsh.whiteCubemapTexture = NULL;
	rsh.blankBumpTexture = NULL;
	rsh.particleTexture = NULL;
	rsh.coronaTexture = NULL;
}

void R_InitImages( void )
{
	assert(!r_imagesPool);

	r_imagesPool = Q_CreatePool( r_mempool, "Images" );
	r_imagesLock = ri.Mutex_Create();

	memset( images, 0, sizeof( images ) );

	// link images
	free_images = images;
	for(size_t i = 0; i < IMAGES_HASH_SIZE; i++ ) {
		images_hash_headnode[i].prev = &images_hash_headnode[i];
		images_hash_headnode[i].next = &images_hash_headnode[i];
	}
	for(size_t i = 0; i < MAX_GLIMAGES - 1; i++ ) {
		images[i].next = &images[i+1];
	}

	rsh.rawTexture = R_CreateImage( "*** raw ***", 0, 0, 1, IT_SPECIAL, 1, IMAGE_TAG_BUILTIN, 3 );
	rsh.rawYUVTextures[0] = R_CreateImage( "*** rawyuv0 ***", 0, 0, 1, IT_SPECIAL, 1, IMAGE_TAG_BUILTIN, 1 );
	rsh.rawYUVTextures[1] = R_CreateImage( "*** rawyuv1 ***", 0, 0, 1, IT_SPECIAL, 1, IMAGE_TAG_BUILTIN, 1 );
	rsh.rawYUVTextures[2] = R_CreateImage( "*** rawyuv2 ***", 0, 0, 1, IT_SPECIAL, 1, IMAGE_TAG_BUILTIN, 1 );
	const struct {
		char *name;
		image_t **image;
		void ( *init )( int *w, int *h, int *flags, int *samples );
	} textures[] = { { "***r_notexture***", &rsh.noTexture, &R_InitNoTexture },
					 { "***r_whitetexture***", &rsh.whiteTexture, &R_InitWhiteTexture },
					 { "***r_whitecubemaptexture***", &rsh.whiteCubemapTexture, &R_InitWhiteCubemapTexture },
					 { "***r_blacktexture***", &rsh.blackTexture, &R_InitBlackTexture },
					 { "***r_greytexture***", &rsh.greyTexture, &R_InitGreyTexture },
					 { "***r_blankbumptexture***", &rsh.blankBumpTexture, &R_InitBlankBumpTexture },
					 { "***r_particletexture***", &rsh.particleTexture, &R_InitParticleTexture },
					 { "***r_coronatexture***", &rsh.coronaTexture, &R_InitCoronaTexture } };

	for( size_t i = 0; i < Q_ARRAY_COUNT( textures ); i++ ) {
	
		int w, h, flags, samples;
		textures[i].init( &w, &h, &flags, &samples );

		image_t *image = R_LoadImage( textures[i].name, r_imageBuffers[QGL_CONTEXT_MAIN], w, h, flags, 1, IMAGE_TAG_BUILTIN, samples );

		if( textures[i].image ) {
			*( textures[i].image ) = image;
		}
	}
}

/*
* R_TouchImage
*/
void R_TouchImage( image_t *image, int tags )
{
	if( !image ) {
		return;
	}

	image->tags |= tags;

	if( image->registrationSequence == rsh.registrationSequence ) {
		return;
	}

	image->registrationSequence = rsh.registrationSequence;
}

/*
* R_FreeUnusedImagesByTags
*/
void R_FreeUnusedImagesByTags( int tags )
{
	int i;
	image_t *image;
	int keeptags = ~tags;

	for( i = 0, image = images; i < MAX_GLIMAGES; i++, image++ ) {
		if(qStrEmpty(image->name)) {
			// free image
			continue;
		}
		if( image->registrationSequence == rsh.registrationSequence ) {
			// we need this image
			continue;
		}

		image->tags &= keeptags;
		if( image->tags ) {
			// still used for a different purpose
			continue;
		}

		__FreeImage( image );
	}
}

/*
* R_FreeUnusedImages
*/
void R_FreeUnusedImages( void )
{
	R_FreeUnusedImagesByTags( ~IMAGE_TAG_BUILTIN );
}

/*
* R_ShutdownImages
*/
void R_ShutdownImages( void )
{
	int i;
	image_t *image;

	if( !r_imagesPool )
		return;

	for(size_t i = 0; i < IMAGE_SAMPLER_HASH_SIZE; i++) {
		FreeRIDescriptor( &rsh.device, &samplerDescriptors[i]);
	}
	memset(samplerDescriptors, 0, sizeof(samplerDescriptors));

	R_ReleaseBuiltinImages();

	for( i = 0, image = images; i < MAX_GLIMAGES; i++, image++ ) {
		if(qStrEmpty(image->name) ) {
			// free texture
			continue;
		}
		__FreeImage(image );
	}

	R_FreeImageBuffers();

	ri.Mutex_Destroy( &r_imagesLock );

	Q_FreePool( r_imagesPool );
	r_imagesPool = NULL;
	r_screenShotBuffer = NULL;
	r_screenShotBufferSize = 0;

}

