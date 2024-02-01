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

#include "r_local.h"
#include "r_imagelib.h"
#include "r_nri.h"

#include "r_texture_format.h"
#include "r_resource_upload.h"

#include "../gameshared/q_sds.h"
#include "../gameshared/q_arch.h"

#include "../qalgo/hash.h"

#include "stb_ds.h"
#include "stb_image.h"
#include <strings.h>


#define	MAX_GLIMAGES	    8192
#define IMAGES_HASH_SIZE    64

typedef struct
{
	int ctx;
	int side;
} loaderCbInfo_t;

static struct image_s images[MAX_GLIMAGES];
static struct image_s images_hash_headnode[IMAGES_HASH_SIZE], *free_images;
static qmutex_t *r_imagesLock;
static mempool_t *r_imagesPool;

static int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
static int gl_filter_max = GL_LINEAR;

static int gl_filter_depth = GL_LINEAR;

static int gl_anisotropic_filter = 0;

int R_TextureTarget( int flags, int *uploadTarget )
{
	int target, target2;

	if( flags & IT_CUBEMAP ) {
		target = GL_TEXTURE_CUBE_MAP_ARB;
		target2 = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
	} else if( flags & IT_ARRAY ) {
		target = target2 = GL_TEXTURE_2D_ARRAY_EXT;
	} else if( flags & IT_3D ) {
		target = target2 = GL_TEXTURE_3D_EXT;
	} else {
		target = target2 = GL_TEXTURE_2D;
	}

	if( uploadTarget )
		*uploadTarget = target2;
	return target;
}

/*
* R_BindImage
*/
static void R_BindImage( const image_t *tex )
{
	qglBindTexture( R_TextureTarget( tex->flags, NULL ), tex->texnum );
	RB_FlushTextureCache();
}

/*
* R_UnbindImage
*/
static void R_UnbindImage( const image_t *tex )
{
	qglBindTexture( R_TextureTarget( tex->flags, NULL ), 0 );
	RB_FlushTextureCache();
}

/*
* R_TextureMode
*/
void R_TextureMode( char *string )
{
	//TODO: Vulkan doesn't have texture samplers they are bound seperatly
	static struct {
		char *name;
		int minimize, maximize;
	} modes[] = { { "GL_NEAREST", GL_NEAREST, GL_NEAREST },
				  { "GL_LINEAR", GL_LINEAR, GL_LINEAR },
				  { "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },
				  { "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR },
				  { "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST },
				  { "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR } };

	for(size_t i = 0; i < Q_ARRAY_COUNT(modes); i++) {
	 // if(Q_stricmp(modes[i], string)) {

	 // }
	}

 // int i;
 // image_t	*glt;
 // int target;

 // for( i = 0; i < NUM_GL_MODES; i++ )
 // {
 // 	if( !Q_stricmp( modes[i].name, string ) )
 // 		break;
 // }

 // if( i == NUM_GL_MODES )
 // {
 // 	Com_Printf( "R_TextureMode: bad filter name\n" );
 // 	return;
 // }

 // gl_filter_min = modes[i].minimize;
 // gl_filter_max = modes[i].maximize;

 // // change all the existing mipmap texture objects
 // for( i = 1, glt = images; i < MAX_GLIMAGES; i++, glt++ )
 // {
 // 	if( !glt->texnum ) {
 // 		continue;
 // 	}
 // 	if( glt->flags & (IT_NOFILTERING|IT_DEPTH) ) {
 // 		continue;
 // 	}

 // 	target = R_TextureTarget( glt->flags, NULL );

 // 	R_BindImage( glt );

 // 	if( !( glt->flags & IT_NOMIPMAP ) )
 // 	{
 // 		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_min );
 // 		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_max );
 // 	}
 // 	else 
 // 	{
 // 		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_max );
 // 		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_max );
 // 	}
 // }
}

static void R_UnpackAlignment( int ctx, int value )
{
	assert(false);
}

/*
* R_AnisotropicFilter
*/
void R_AnisotropicFilter( int value )
{
	int i, old;
	image_t	*glt;

	if( !glConfig.ext.texture_filter_anisotropic )
		return;

	old = gl_anisotropic_filter;
	gl_anisotropic_filter = bound( 1, value, glConfig.maxTextureFilterAnisotropic );
	if( gl_anisotropic_filter == old )
		return;

	// change all the existing mipmap texture objects
	for( i = 1, glt = images; i < MAX_GLIMAGES; i++, glt++ )
	{
		if( !glt->texnum ) {
			continue;
		}
		if( (glt->flags & (IT_NOFILTERING|IT_DEPTH|IT_NOMIPMAP)) ) {
			continue;
		}

		R_BindImage( glt );

		qglTexParameteri( R_TextureTarget( glt->flags, NULL ), GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic_filter );
	}
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
		if( !image->texnum ) {
			continue;
		}
		if( !image->upload_width || !image->upload_height || !image->layers ) {
			continue;
		}
		if( filter && !filter( mask, image->name ) ) {
			continue;
		}
		if( !image->loaded || image->missing ) {
			continue;
		}

		add = image->upload_width * image->upload_height * image->layers;
		if( !(image->flags & (IT_DEPTH|IT_NOFILTERING|IT_NOMIPMAP)) )
			add = (unsigned)floor( add / 0.75 );
		if( image->flags & IT_CUBEMAP )
			add *= 6;
		texels += add;

		bpp = image->samples;
		if( image->flags & IT_DEPTH )
			bpp = 0; // added later
		else if( ( image->flags & IT_FRAMEBUFFER ) && !glConfig.ext.rgb8_rgba8 )
			bpp = 2;

		if( image->flags & ( IT_DEPTH|IT_DEPTHRB ) )
		{
			if( image->flags & IT_STENCIL )
				bpp += 4;
			else if( glConfig.ext.depth24 )
				bpp += 3;
			else
				bpp += 2;
		}

		bytes = add * bpp;
		total_bytes += bytes;

		Com_Printf( " %iW x %iH", image->upload_width, image->upload_height );
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
			R_Free( r_imageBuffers[ctx][buffer] );
		r_imageBuffers[ctx][buffer] = R_MallocExt( r_imagesPool, size, 0, 1 );
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
				R_Free( r_imageBuffers[i][j] );
				r_imageBuffers[i][j] = NULL;
			}
			r_imageBufSize[i][j] = 0;
		}
}

/*
* R_SwapBlueRed
*/
static void R_SwapBlueRed( uint8_t *data, int width, int height, int samples, int alignment )
{
	int i, j, k, padding;

	padding = ALIGN( width * samples, alignment ) - width * samples;
	for( i = 0; i < height; i++, data += padding )
	{
		for( j = 0; j < width; j++, data += samples )
		{
			k = data[0];
			data[0] = data[2];
			data[2] = k;
			// data[0] ^= data[2];
			// data[2] = data[0] ^ data[2];
			// data[0] ^= data[2];
		}
	}
}

/*
* R_EndianSwap16BitImage
*/
static void R_EndianSwap16BitImage( unsigned short *data, int width, int height )
{
	int i;
	while( height-- > 0 )
	{
		for( i = 0; i < width; i++, data++ )
			*data = ( ( *data & 255 ) << 8 ) | ( *data >> 8 );
		data += width & 1; // 4 unpack alignment
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

/**
 *
 * returns stbi buffer needs to be freed with stbi_image_free
 **/
static stbi_uc* __loadFileImageStbi(const char* filepath, int* width, int* height, enum texture_format_e* format) {
	uint8_t * fileBuffer;
	size_t size = R_LoadFile( filepath, ( void ** ) &fileBuffer );
	if( fileBuffer == NULL )
		return NULL;

	int channelCount;
	stbi_uc* stbiBuffer = stbi_load_from_memory(fileBuffer, size, 
																						width,
																						height,
																						&channelCount, 0);
	switch(channelCount) {
		case 3:
			*format = R_FORMAT_RGB8_UNORM;
			break;
		case 4:
			*format = R_FORMAT_RGBA8_UNORM;
			break;
		default:
			stbi_image_free(stbiBuffer);
			stbiBuffer = NULL;
			ri.Com_Printf(S_COLOR_YELLOW "unhandled channel count: %d", channelCount);
			break;
	}
	R_Free(fileBuffer);
	return stbiBuffer; 
}

static int R_ReadImageFromDisk( int ctx, char *pathname, size_t pathname_size, 
	uint8_t **pic, int *width, int *height, int *flags, int side )
{
	const char *extension;
	int samples;

	*pic = NULL;
	*width = *height = 0;
	samples = 0;

	extension = ri.FS_FirstExtension( pathname, IMAGE_EXTENSIONS, NUM_IMAGE_EXTENSIONS - 1 ); // last is KTX
	if( extension )
	{

		loaderCbInfo_t cbinfo = { ctx, side };

		COM_ReplaceExtension( pathname, extension, pathname_size );

		r_imginfo_t imginfo = IMG_LoadImage( pathname, _R_AllocImageBufferCb, (void *)&cbinfo );

		if( imginfo.samples >= 3 )
		{
			if( ( (imginfo.comp & ~1) == IMGCOMP_BGR ) && ( !glConfig.ext.bgra || !flags ) )
			{
				R_SwapBlueRed( imginfo.pixels, imginfo.width, imginfo.height, imginfo.samples, 1 );
				imginfo.comp = IMGCOMP_RGB | (imginfo.comp & 1);
			}
		}

		*pic = imginfo.pixels;
		*width = imginfo.width;
		*height = imginfo.height;
		samples = imginfo.samples;
		if( flags )
		{
			if( ( imginfo.samples >= 3 ) && ( ( imginfo.comp & ~1 ) == IMGCOMP_BGR ) )
				*flags |= IT_BGRA;
		}
	}

	return samples;
}

/*
* R_ScaledImageSize
*/
static int R_ScaledImageSize( int width, int height, int *scaledWidth, int *scaledHeight, int flags, int mips, int minmipsize, bool forceNPOT )
{
	int maxSize;
	int mip = 0;
	int clampedWidth, clampedHeight;
	bool makePOT;

	if( flags & ( IT_FRAMEBUFFER|IT_DEPTH ) )
		maxSize = glConfig.maxRenderbufferSize;
	else if( flags & IT_CUBEMAP )
		maxSize = glConfig.maxTextureCubemapSize;
	else if( flags & IT_3D )
		maxSize = glConfig.maxTexture3DSize;
	else
		maxSize = glConfig.maxTextureSize;

	makePOT = !glConfig.ext.texture_non_power_of_two && !forceNPOT;
#ifdef GL_ES_VERSION_2_0
	makePOT = makePOT && ( ( flags & ( IT_CLAMP|IT_NOMIPMAP ) ) != ( IT_CLAMP|IT_NOMIPMAP ) );
#endif
	if( makePOT )
	{
		int potWidth, potHeight;

		for( potWidth = 1; potWidth < width; potWidth <<= 1 );
		for( potHeight = 1; potHeight < height; potHeight <<= 1 );

		if( ( width != potWidth ) || ( height != potHeight ) )
			mips = 1;

		width = potWidth;
		height = potHeight;
	}

	if( !( flags & IT_NOPICMIP ) )
	{
		// let people sample down the sky textures for speed
		int picmip = ( flags & IT_SKY ) ? r_skymip->integer : r_picmip->integer;
		while( ( mip < picmip ) && ( ( width > minmipsize ) || ( height > minmipsize ) ) )
		{
			++mip;
			width >>= 1;
			height >>= 1;
			if( !width )
				width = 1;
			if( !height )
				height = 1;
		}
	}

	// try to find the smallest supported texture size from mipmaps
	clampedWidth = width;
	clampedHeight = height;
	while( ( clampedWidth > maxSize ) || ( clampedHeight > maxSize ) )
	{
		++mip;
		clampedWidth >>= 1;
		clampedHeight >>= 1;
		if( !clampedWidth )
			clampedWidth = 1;
		if( !clampedHeight )
			clampedHeight = 1;
	}

	if( mip >= mips )
	{
		// the smallest size is not in mipmaps, so ignore mipmaps and aspect ratio and simply clamp
		*scaledWidth = min( width, maxSize );
		*scaledHeight = min( height, maxSize );
		return -1;
	}

	*scaledWidth = clampedWidth;
	*scaledHeight = clampedHeight;
	return mip;
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
* R_TextureInternalFormat
*/
#ifndef GL_ES_VERSION_2_0
static int R_TextureInternalFormat( int samples, int flags, int pixelType )
{
	int bits = r_texturebits->integer;

	if( !( flags & IT_NOCOMPRESS ) && r_texturecompression->integer && glConfig.ext.texture_compression )
	{
		if( samples == 4 )
			return GL_COMPRESSED_RGBA_ARB;
		if( samples == 3 )
			return GL_COMPRESSED_RGB_ARB;
		if( samples == 2 )
			return GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
		if( ( samples == 1 ) && !( flags & IT_ALPHAMASK ) )
			return GL_COMPRESSED_LUMINANCE_ARB;
	}

	if( samples == 3 )
	{
		if( bits == 16 )
			return GL_RGB5;
		return GL_RGB;
	}

	if( samples == 2 )
	{
		return GL_LUMINANCE_ALPHA;
	}

	if( samples == 1 )
	{
		return ( ( flags & IT_ALPHAMASK ) ? GL_ALPHA : GL_LUMINANCE );
	}

	if( ( bits == 16 ) && ( pixelType != GL_UNSIGNED_SHORT_5_5_5_1 ) )
		return GL_RGBA4;
	return GL_RGBA;
}
#endif

/*
* R_TextureFormat
*/
static void R_TextureFormat( int flags, int samples, int *comp, int *format, int *type )
{
	if( flags & IT_DEPTH )
	{
		if( flags & IT_STENCIL )
		{
			*comp = *format = GL_DEPTH_STENCIL_EXT;
			*type = GL_UNSIGNED_INT_24_8_EXT;
		}
		else
		{
			*comp = *format = GL_DEPTH_COMPONENT;
			if( glConfig.ext.depth24 )
			{
				*type = GL_UNSIGNED_INT;
			}
			else
			{
				*type = GL_UNSIGNED_SHORT;
				if( glConfig.ext.depth_nonlinear )
					*comp = GL_DEPTH_COMPONENT16_NONLINEAR_NV;
			}
		}
	}
	else if( flags & IT_FRAMEBUFFER )
	{
		if( samples == 4 )
		{
			*comp = *format = GL_RGBA;
			*type = glConfig.ext.rgb8_rgba8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT_4_4_4_4;
		}
		else
		{
			*comp = *format = GL_RGB;
			*type = glConfig.ext.rgb8_rgba8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT_5_6_5;
		}
	}
	else
	{
		*type = GL_UNSIGNED_BYTE;
		if( samples == 4 )
			*format = ( flags & IT_BGRA ? GL_BGRA_EXT : GL_RGBA );
		else if( samples == 3 )
			*format = ( flags & IT_BGRA ? GL_BGR_EXT : GL_RGB );
		else if( samples == 2 )
			*format = GL_LUMINANCE_ALPHA;
		else if( flags & IT_ALPHAMASK )
			*format = GL_ALPHA;
		else
			*format = GL_LUMINANCE;
		*comp = *format;
#ifndef GL_ES_VERSION_2_0
		if( !( flags & IT_3D ) )
			*comp = R_TextureInternalFormat( samples, flags, GL_UNSIGNED_BYTE );
#endif
	}
}

/*
* R_SetupTexParameters
*/
static void R_SetupTexParameters( int flags, int upload_width, int upload_height, int minmipsize )
{
	int target = R_TextureTarget( flags, NULL );
	int wrap = GL_REPEAT;

	if( flags & IT_NOFILTERING )
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	else if( flags & IT_DEPTH )
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_depth );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_depth );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
	}
	else if( !( flags & IT_NOMIPMAP ) )
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_min );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_max );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic_filter );

		if( minmipsize > 1 )
		{
			int mipwidth = upload_width, mipheight = upload_height, mip = 0;
			while( ( mipwidth > minmipsize ) || ( mipheight > minmipsize ) )
			{
				++mip;
				mipwidth >>= 1;
				mipheight >>= 1;
				if( !mipwidth )
					mipwidth = 1;
				if( !mipheight )
					mipheight = 1;
			}
			qglTexParameteri( target, GL_TEXTURE_MAX_LOD_SGIS, mip );
			qglTexParameteri( target, GL_TEXTURE_MAX_LEVEL_SGIS, mip );
		}
	}
	else
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_max );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_max );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
	}

	// clamp if required
	if( flags & IT_CLAMP )
	{
		if( glConfig.ext.texture_edge_clamp )
			wrap = GL_CLAMP_TO_EDGE;
#ifndef GL_ES_VERSION_2_0
		else
			wrap = GL_CLAMP;
#endif
	}
	qglTexParameteri( target, GL_TEXTURE_WRAP_S, wrap );
	qglTexParameteri( target, GL_TEXTURE_WRAP_T, wrap );
	if( flags & IT_3D )
		qglTexParameteri( target, GL_TEXTURE_WRAP_R_EXT, wrap );

	if( ( flags & IT_DEPTH ) && ( flags & IT_DEPTHCOMPARE ) && glConfig.ext.shadow )
	{
		qglTexParameteri( target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );
		qglTexParameteri( target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );
	}
}

/*
* R_Upload32
*/
static void R_Upload32( int ctx, uint8_t **data, int layer,
	int x, int y, int width, int height,
	int flags, int minmipsize, int *upload_width, int *upload_height, int samples,
	bool subImage, bool noScale )
{
	int i, comp, format, type;
	int target;
	int numTextures;
	uint8_t *scaled = NULL;
	int scaledWidth, scaledHeight;

	assert( samples );

	R_ScaledImageSize( width, height, &scaledWidth, &scaledHeight, flags, 1, minmipsize,
		( subImage && noScale ) ? true : false );

	R_TextureTarget( flags, &target );

	// don't ever bother with > maxSize textures
	if( flags & IT_CUBEMAP )
	{
		numTextures = 6;
	}
	else
	{
		if( flags & ( IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL ) )
		{
			uint8_t *temp = R_PrepareImageBuffer( ctx, TEXTURE_FLIPPING_BUF0, width * height * samples );
			R_FlipTexture( data[0], temp, width, height, samples, 
				(flags & IT_FLIPX) ? true : false, 
				(flags & IT_FLIPY) ? true : false, 
				(flags & IT_FLIPDIAGONAL) ? true : false );
			data = &r_imageBuffers[ctx][TEXTURE_FLIPPING_BUF0];
		}

		numTextures = 1;
	}

	if( upload_width )
		*upload_width = scaledWidth;
	if( upload_height )
		*upload_height = scaledHeight;

	R_TextureFormat( flags, samples, &comp, &format, &type );

	if( !( flags & ( IT_ARRAY | IT_3D ) ) ) // set in R_Create3DImage
		R_SetupTexParameters( flags, scaledWidth, scaledHeight, minmipsize );

	R_UnpackAlignment( ctx, 1 );

	if( ( scaledWidth == width ) && ( scaledHeight == height ) && ( flags & IT_NOMIPMAP ) )
	{
		if( flags & ( IT_ARRAY | IT_3D ) )
		{
			for( i = 0; i < numTextures; i++, target++ )
				qglTexSubImage3DEXT( target, 0, 0, 0, layer, scaledWidth, scaledHeight, 1, format, type, data[i] );
		}
		else if( subImage )
		{
			for( i = 0; i < numTextures; i++, target++ )
				qglTexSubImage2D( target, 0, x, y, scaledWidth, scaledHeight, format, type, data[i] );
		}
		else
		{
			for( i = 0; i < numTextures; i++, target++ )
				qglTexImage2D( target, 0, comp, scaledWidth, scaledHeight, 0, format, type, data[i] );
		}
	}
	else
	{
		for( i = 0; i < numTextures; i++, target++ )
		{
			uint8_t *mip;

			if( !scaled )
				scaled = R_PrepareImageBuffer( ctx, TEXTURE_RESAMPLING_BUF0, 
				scaledWidth * scaledHeight * samples );

			// resample the texture
			mip = scaled;
			if( data[i] )
				R_ResampleTexture( ctx, data[i], width, height, (uint8_t *)mip, scaledWidth, scaledHeight, samples, 1 );
			else
				mip = NULL;

			if( flags & ( IT_ARRAY | IT_3D ) )
				qglTexSubImage3DEXT( target, 0, 0, 0, layer, scaledWidth, scaledHeight, 1, format, type, mip );
			else if( subImage )
				qglTexSubImage2D( target, 0, x, y, scaledWidth, scaledHeight, format, type, mip );
			else
				qglTexImage2D( target, 0, comp, scaledWidth, scaledHeight, 0, format, type, mip );

			// mipmaps generation
			if( !( flags & IT_NOMIPMAP ) && mip )
			{
				int w, h;
				int miplevel = 0;

				w = scaledWidth;
				h = scaledHeight;
				while( w > minmipsize || h > minmipsize )
				{
					R_MipMap( mip, w, h, samples, 1 );

					w >>= 1;
					h >>= 1;
					if( w < 1 )
						w = 1;
					if( h < 1 )
						h = 1;
					miplevel++;

					if( flags & ( IT_ARRAY | IT_3D ) )
						qglTexSubImage3DEXT( target, miplevel, 0, 0, layer, w, h, 1, format, type, mip );
					else if( subImage )
						qglTexSubImage2D( target, miplevel, x, y, w, h, format, type, mip );
					else
						qglTexImage2D( target, miplevel, comp, w, h, 0, format, type, mip );
				}
			}
		}
	}
}

/*
* R_PixelFormatSize
*/
static int R_PixelFormatSize( int format, int type )
{
	switch( type )
	{
	case GL_UNSIGNED_BYTE:
		switch( format )
		{
		case GL_RGBA:
		case GL_BGRA_EXT:
			return 4;
		case GL_RGB:
		case GL_BGR_EXT:
			return 3;
		case GL_LUMINANCE_ALPHA:
			return 2;
		case GL_ALPHA:
		case GL_LUMINANCE:
			return 1;
		}
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_5_6_5:
		return 2;
	case 0: // 4x4 block sizes
		switch( format )
		{
		case GL_ETC1_RGB8_OES:
			return 8;
		}
		break;
	}
	return 0;
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

/*
* R_UploadMipmapped
*
* Loads a 16/24/32-bit image or cubemap with mipmaps.
* If the image is a cubemap, each face is data[mip * 6 + face].
* Otherwise, it's data[mip].
*/
static void R_UploadMipmapped( int ctx, uint8_t **data,
	int width, int height, int mipLevels, int flags, int minmipsize,
	int *upload_width, int *upload_height,
	int format, int type )
{
	int i, j;
	int pixelSize = R_PixelFormatSize( format, type );
	int rMask = 0, gMask = 0, bMask = 0, aMask = 0;
	int scaledWidth, scaledHeight;
	int mip;
	uint8_t *scaled[6] = { NULL };
	int faces, faceSize = 0;
	int target, comp;
	int mips;
	uint8_t *face;
	int oldWidth = 0, oldHeight = 0;

	switch( type )
	{
	case GL_UNSIGNED_SHORT_4_4_4_4:
		rMask = 15 << 12;
		gMask = 15 << 8;
		bMask = 15 << 4;
		aMask = 15;
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		rMask = 31 << 11;
		gMask = 31 << 6;
		bMask = 31 << 1;
		aMask = 1;
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		rMask = 31 << 11;
		gMask = 63 << 5;
		bMask = 31;
		break;
	}

	R_TextureTarget( flags, &target );

	faces = ( flags & IT_CUBEMAP ) ? 6 : 1;
	
	mip = R_ScaledImageSize( width, height, &scaledWidth, &scaledHeight, flags, mipLevels, minmipsize, false );

	if( upload_width )
		*upload_width = scaledWidth;
	if( upload_height )
		*upload_height = scaledHeight;

	if( mip < 0 )
	{
		faceSize = ALIGN( scaledWidth * pixelSize, 4 ) * scaledHeight;

		for( i = 0; i < faces; i++ )
			scaled[i] = R_PrepareImageBuffer( ctx, TEXTURE_RESAMPLING_BUF0 + i, faceSize );

		// find the mip with the size closest to the target
		for( mip = 0; mip < ( mipLevels - 1 ); mip++ )
		{
			if( ( max( width >> 1, 1 ) < scaledWidth ) || ( max( height >> 1, 1 ) < scaledHeight ) )
				break;
			width >>= 1;
			height >>= 1;
			if( !width )
				width = 1;
			if( !height )
				height = 1;
		}

		if( type == GL_UNSIGNED_BYTE )
		{
			for( i = 0; i < faces; i++ )
			{
				R_ResampleTexture( ctx, data[mip * faces + i], width, height,
					scaled[i], scaledWidth, scaledHeight, pixelSize, 4 );
			}
		}
		else
		{
			for( i = 0; i < faces; i++ )
			{
				R_ResampleTexture16( ctx, ( unsigned short * )( data[mip * faces + i] ), width, height,
					( unsigned short * )( scaled[i] ), scaledWidth, scaledHeight, rMask, gMask, bMask, aMask );
			}
		}
		data = scaled;
		mip = 0;
		mipLevels = 1;
	}

#ifdef GL_ES_VERSION_2_0
	comp = format;
#else
	comp = R_TextureInternalFormat( pixelSize, flags, type );
#endif

	R_SetupTexParameters( flags, scaledWidth, scaledHeight, minmipsize );

	R_UnpackAlignment( ctx, 4 );

	mips = ( flags & IT_NOMIPMAP ) ? 1 : R_MipCount( scaledWidth, scaledHeight, minmipsize );
	for( i = 0; ( i < mips ) && ( mip < mipLevels ); i++, mip++ )
	{
		faceSize = ALIGN( scaledWidth * pixelSize, 4 ) * scaledHeight; // will be used for the first remaining mipmap
		for( j = 0; j < faces; j++ )
			qglTexImage2D( target + j, i, comp, scaledWidth, scaledHeight, 0, format, type, data[mip * faces + j] );
		oldWidth = scaledWidth;
		oldHeight = scaledHeight;
		scaledWidth >>= 1;
		scaledHeight >>= 1;
		if( !scaledWidth )
			scaledWidth = 1;
		if( !scaledHeight )
			scaledHeight = 1;
	}

	for( ; i < mips; i++ )
	{
		for( j = 0; j < faces; j++ )
		{
			if( !( scaled[j] ) )
			{
				scaled[j] = R_PrepareImageBuffer( ctx, TEXTURE_RESAMPLING_BUF0 + j, faceSize );
				memcpy( scaled[j], data[( mip - 1 ) * faces + j], faceSize );
			}
			face = scaled[j];
			if( type == GL_UNSIGNED_BYTE )
				R_MipMap( face, oldWidth, oldHeight, pixelSize, 4 );
			else
				R_MipMap16( ( unsigned short * )face, oldWidth, oldHeight, rMask, gMask, bMask, aMask );
			qglTexImage2D( target + j, i, comp, scaledWidth, scaledHeight, 0, format, type, face );
		}

		oldWidth = scaledWidth;
		oldHeight = scaledHeight;
		scaledWidth >>= 1;
		scaledHeight >>= 1;
		if( !scaledWidth )
			scaledWidth = 1;
		if( !scaledHeight )
			scaledHeight = 1;
	}
}

/*
* R_IsKTXFormatValid
*/
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

typedef struct ktx_header_s
{
	char identifier[12];
	int endianness;
	int type;
	int typeSize;
	int format;
	int internalFormat;
	int baseInternalFormat;
	int pixelWidth;
	int pixelHeight;
	int pixelDepth;
	int numberOfArrayElements;
	int numberOfFaces;
	int numberOfMipmapLevels;
	int bytesOfKeyValueData;
} ktx_header_t;



/*
* R_LoadKTX
*/
static bool R_LoadKTX( int ctx, image_t *image, const char *pathname )
{
	int i, j;
	uint8_t *buffer;
	ktx_header_t *header;
	bool swapEndian;
	uint8_t *data;
	int numFaces = ( ( image->flags & IT_CUBEMAP ) ? 6 : 1 ), numMips;

	if( image->flags & ( IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL ) )
		return false;

	R_LoadFile( pathname, ( void ** )&buffer );
	if( !buffer )
		return false;

	header = ( ktx_header_t * )buffer;
	if( memcmp( header->identifier, "\xABKTX 11\xBB\r\n\x1A\n", 12 ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Bad file identifier: %s\n", pathname );
		goto error;
	}

	swapEndian = ( header->endianness == 0x01020304 ) ? true : false;
	if( swapEndian )
	{
		for( i = 3; i < 16; ++i )
			( ( int * )header )[i] = LongSwap( ( ( int * )header )[i] );
	}

	if( header->format && ( header->format != header->baseInternalFormat ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Pixel format doesn't match internal format: %s\n", pathname );
		goto error;
	}
	if( !R_IsKTXFormatValid( header->format ? header->baseInternalFormat : header->internalFormat, header->type ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Unsupported pixel format: %s\n", pathname );
		goto error;
	}
	if( ( header->pixelWidth < 1 ) || ( header->pixelHeight < 0 ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Zero texture size: %s\n", pathname );
		goto error;
	}
	if( !header->pixelHeight )
		header->pixelHeight = 1;
	if( !header->type && ( ( header->pixelWidth & ( header->pixelWidth - 1 ) ) || ( header->pixelHeight & ( header->pixelHeight - 1 ) ) ) )
	{
		// NPOT compressed textures may crash on certain drivers/GPUs
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Compressed image must be power-of-two: %s\n", pathname );
		goto error;
	}
	if( ( image->flags & IT_CUBEMAP ) && ( header->pixelWidth != header->pixelHeight ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Not square cubemap image: %s\n", pathname );
		goto error;
	}
	if( ( header->pixelDepth > 1 ) || ( header->numberOfArrayElements > 1 ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: 3D textures and texture arrays are not supported: %s\n", pathname );
		goto error;
	}
	if( header->numberOfFaces != numFaces )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Bad number of cubemap faces: %s\n", pathname );
		goto error;
	}
	if( header->numberOfMipmapLevels < 1 )
		header->numberOfMipmapLevels = 1;

	numMips = R_MipCount( header->pixelWidth, header->pixelHeight, image->minmipsize );

	data = buffer + sizeof( ktx_header_t ) + header->bytesOfKeyValueData;
	
	R_BindImage( image );

	if( header->type == 0 )
	{
		int mips = numMips;
		int mip;
		int scaledWidth, scaledHeight;

		if( ( header->numberOfMipmapLevels == 1 ) && ( image->flags & IT_NOMIPMAP ) )
		{
			mips = 1;
		}
		else if( header->numberOfMipmapLevels < mips )
		{
			ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Compressed image has too few mip levels: %s\n", pathname );
			goto error;
		}

		mip = R_ScaledImageSize( header->pixelWidth, header->pixelHeight, &scaledWidth, &scaledHeight,
			image->flags, mips, image->minmipsize, false );

		image->upload_width = scaledWidth;
		image->upload_height = scaledHeight;

		// If different compression formats are added, make this more general-purpose!

		if( !glConfig.ext.texture_compression || !( glConfig.ext.compressed_ETC1_RGB8_texture || glConfig.ext.ES3_compatibility ) || ( mip < 0 ) )
		{
			int inSize = ( ( ALIGN( header->pixelWidth, 4 ) * ALIGN( header->pixelHeight, 4 ) ) >> 4 ) * 8;
			int outSize = ALIGN( header->pixelWidth * 3, 4 ) * header->pixelHeight;
			uint8_t *in = data + sizeof( int );
			uint8_t *decompressed[6];

			for( i = 0; i < numFaces; ++i )
			{
				decompressed[i] = R_PrepareImageBuffer( ctx, TEXTURE_LOADING_BUF0 + i, outSize );
				DecompressETC1( in, header->pixelWidth, header->pixelHeight, decompressed[i], glConfig.ext.bgra ? true : false );
				in += inSize;
			}

			R_UploadMipmapped( ctx, decompressed, header->pixelWidth, header->pixelHeight, 1,
				image->flags, image->minmipsize, &image->upload_width, &image->upload_height,
				glConfig.ext.bgra ? GL_BGR_EXT : GL_RGB, GL_UNSIGNED_BYTE );
		}
		else
		{
			int target;
			int compressedFormat = glConfig.ext.ES3_compatibility ? GL_COMPRESSED_RGB8_ETC2 : GL_ETC1_RGB8_OES;
			size_t faceSize;

			R_TextureTarget( image->flags, &target );

			R_SetupTexParameters( image->flags, scaledWidth, scaledHeight, image->minmipsize );

			for( i = 0; i < mip; ++i )
			{
				data += sizeof( int ) + numFaces * ( (
					ALIGN( max( header->pixelWidth >> i, 1 ), 4 ) *
					ALIGN( max( header->pixelHeight >> i, 1 ), 4 )
				) >> 4 ) * 8;
			}

			mips -= mip;
			for( i = 0; i < mips; ++i )
			{
				faceSize = ( ( ALIGN( scaledWidth, 4 ) * ALIGN( scaledHeight, 4 ) ) >> 4 ) * 8;
				data += sizeof( int );
				for( j = 0; j < numFaces; ++j )
				{
					qglCompressedTexImage2DARB( target + j, i, compressedFormat,
						scaledWidth, scaledHeight, 0, faceSize, data );
					data += faceSize;
				}
				scaledWidth >>= 1;
				scaledHeight >>= 1;
				if( !scaledWidth )
					scaledWidth = 1;
				if( !scaledHeight )
					scaledHeight = 1;
			}
		}

		image->samples = 3;
	}
	else
	{
		int mips = ( image->flags & IT_NOMIPMAP ) ? 1 : min( header->numberOfMipmapLevels, numMips );
		uint8_t *images[32 * 6];
		int mipWidth = header->pixelWidth, mipHeight = header->pixelHeight;
		size_t pixelSize = 2;
		size_t faceSize;

		switch( header->baseInternalFormat )
		{
		case GL_RGBA:
			image->samples = 4;
			break;
		case GL_BGRA_EXT:
			image->samples = 4;
			image->flags |= IT_BGRA;
			break;
		case GL_RGB:
			image->samples = 3;
			break;
		case GL_BGR_EXT:
			image->samples = 3;
			image->flags |= IT_BGRA;
			break;
		case GL_LUMINANCE_ALPHA:
			image->samples = 2;
			break;
		case GL_LUMINANCE:
			image->samples = 1;
			break;
		case GL_ALPHA:
			image->samples = 1;
			image->flags |= IT_ALPHAMASK;
			break;
		}

		if( header->type == GL_UNSIGNED_BYTE )
			pixelSize = image->samples;

		for( i = 0; i < mips; i++ )
		{
			faceSize = ALIGN( max( header->pixelWidth >> i, 1 ) * pixelSize, 4 ) * max( header->pixelHeight >> i, 1 );
			data += sizeof( int );
			for( j = 0; j < numFaces; j++ )
				images[i * numFaces + j] = data + faceSize * j;
			data += faceSize * numFaces;
		}

		if( !glConfig.ext.bgra &&
			( ( header->baseInternalFormat == GL_BGR_EXT ) || ( header->baseInternalFormat == GL_BGRA_EXT ) ) )
		{
			for( i = 0; i < mips; i++ )
			{
				for( j = 0; j < numFaces; j++ )
				{
					R_SwapBlueRed( images[i * numFaces + j], mipWidth, mipHeight,
						( header->baseInternalFormat == GL_BGR_EXT ) ? 3 : 4, 4 );
				}
				mipWidth >>= 1;
				mipHeight >>= 1;
				if( !mipWidth )
					mipWidth = 1;
				if( !mipHeight )
					mipHeight = 1;
			}
			header->baseInternalFormat = ( ( header->baseInternalFormat == GL_BGR_EXT ) ? GL_RGB : GL_RGBA );
		}
		else if( swapEndian && (
			( header->type == GL_UNSIGNED_SHORT_4_4_4_4 ) || ( header->type == GL_UNSIGNED_SHORT_5_5_5_1 ) ||
			( header->type == GL_UNSIGNED_SHORT_5_6_5 ) ) )
		{
			for( i = 0; i < mips; i++ )
			{
				for( j = 0; j < numFaces; j++ )
					R_EndianSwap16BitImage( ( unsigned short * )( images[i * numFaces + j] ), mipWidth, mipHeight );
				mipWidth >>= 1;
				mipHeight >>= 1;
				if( !mipWidth )
					mipWidth = 1;
				if( !mipHeight )
					mipHeight = 1;
			}
		}

		R_UploadMipmapped( ctx, images, header->pixelWidth, header->pixelHeight, mips, image->flags, image->minmipsize,
			&image->upload_width, &image->upload_height, header->baseInternalFormat, header->type );
	}

	Q_strncpyz( image->extension, ".ktx", sizeof( image->extension ) );
	image->width = header->pixelWidth;
	image->height = header->pixelHeight;

	R_FreeFile( buffer );
	R_DeferDataSync();
	return true;

error: // must not be reached after actually starting uploading the texture
	R_FreeFile( buffer );
	return false;
}

static image_t *R_LinkPic( unsigned int hash )
{
	image_t *image;

	if( !free_images ) {
		return NULL;
	}

	ri.Mutex_Lock( r_imagesLock );

	hash = hash % IMAGES_HASH_SIZE;
	image = free_images;
	free_images = image->next;

	// link to the list of active images
	image->prev = &images_hash_headnode[hash];
	image->next = images_hash_headnode[hash].next;
	image->next->prev = image;
	image->prev->next = image;

	ri.Mutex_Unlock( r_imagesLock );

	return image;
}
static ktx_header_t* R_FetchKTXHeader(uint8_t* buffer, bool* endianess) {
	ktx_header_t *header = (ktx_header_t *)buffer;
	const bool swapEndian = ( header->endianness == 0x01020304 ) ? true : false;
	if(endianess) {
		(*endianess) = swapEndian;
	}
	if( swapEndian )
	{
		header->type = LongSwap(header->type);
		header->typeSize = LongSwap(header->typeSize);
		header->format = LongSwap(header->format);
		header->internalFormat = LongSwap(header->internalFormat);
		header->baseInternalFormat = LongSwap(header->baseInternalFormat);
		header->pixelWidth = LongSwap(header->pixelWidth);
		header->pixelHeight = LongSwap(header->pixelHeight);
		header->pixelDepth = LongSwap(header->pixelDepth);
		header->numberOfArrayElements = LongSwap(header->numberOfArrayElements);
		header->numberOfFaces = LongSwap(header->numberOfFaces);
		header->numberOfMipmapLevels = LongSwap(header->numberOfMipmapLevels);
		header->bytesOfKeyValueData = LongSwap(header->bytesOfKeyValueData);
	}
	return header;
}
static bool R_ValidateKTXHeader(const ktx_header_t* header, const int flags,const char* pathname, const uint16_t expectedNumFaces) {
	assert(header);
	bool result = true;

	if( memcmp( header->identifier, "\xABKTX 11\xBB\r\n\x1A\n", 12 ) ) {
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Bad file identifier: %s\n", pathname );
		result = false;
	}

	if( header->format && ( header->format != header->baseInternalFormat ) ) {
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Pixel format doesn't match internal format: %s\n", pathname );
		result = false;
	}
	if( !R_IsKTXFormatValid( header->format ? header->baseInternalFormat : header->internalFormat, header->type ) ) {
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Unsupported pixel format: %s\n", pathname );
		result = false;
	}
	if( ( header->pixelWidth < 1 ) || ( header->pixelHeight < 0 ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Zero texture size: %s\n", pathname );
		result = false; 
	}
	if( !header->type && ( ( header->pixelWidth & ( header->pixelWidth - 1 ) ) || ( header->pixelHeight & ( header->pixelHeight - 1 ) ) ) )
	{
		// NPOT compressed textures may crash on certain drivers/GPUs
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Compressed image must be power-of-two: %s\n", pathname );
		result = false; 
	}
	if( ( flags & IT_CUBEMAP ) && ( header->pixelWidth != header->pixelHeight ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Not square cubemap image: %s\n", pathname );
		result = false;
	}
	if( ( header->pixelDepth > 1 ) || ( header->numberOfArrayElements > 1 ) )
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: 3D textures and texture arrays are not supported: %s\n", pathname );
		result = false;
	}
	if( header->numberOfFaces != expectedNumFaces)
	{
		ri.Com_DPrintf( S_COLOR_YELLOW "R_LoadKTX: Bad number of cubemap faces: %s\n", pathname );
		result = false;
	}

	return result;
}

//TODO: move ktx loader to a seperate file
static bool __R_LoadKTX( image_t *image, const char *pathname )
{
	const uint16_t numFaces = ( ( image->flags & IT_CUBEMAP ) ? 6 : 1 );
	uint8_t *buffer = NULL;
	if( image->flags & ( IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL ) )
		return false;
	R_LoadFile( pathname, (void **)&buffer );
	if( !buffer )
		return false;
	bool swapEndian = false;
	ktx_header_t *header = R_FetchKTXHeader(buffer, &swapEndian);
	if(!R_ValidateKTXHeader(header, image->flags, pathname, numFaces)) {
		goto error;
	}
	if( !header->pixelHeight ) {
		header->pixelHeight = 1;
	}
	if( header->numberOfMipmapLevels < 1 ) {
		header->numberOfMipmapLevels = 1;
	}
	const uint16_t numMips = fmin((image->flags & IT_NOMIPMAP) ? 1 : ceil(log2(max(header->pixelWidth, header->pixelHeight))), header->numberOfMipmapLevels);
	uint8_t* data = buffer + sizeof( ktx_header_t ) + header->bytesOfKeyValueData;
	image->upload_width = header->pixelWidth;
	image->upload_height = header->pixelHeight;

	// compressed format
	if( header->type == 0 )
	{
		//TODO
		assert(false);
	} else {
		NriTextureDesc textureDesc = { .width = header->pixelWidth,
									   .height = header->pixelHeight,
									   .arraySize = ( image->flags & IT_CUBEMAP ) ? 6 : 1,
									   .sampleNum = 1,
									   .type = NriTextureType_TEXTURE_2D,
									   .mipNum = numMips};

		uint8_t* bufStart[32 * 6];
		switch( header->baseInternalFormat )
		{
		case GL_RGBA:
			image->samples = 4;
			break;
		case GL_BGRA_EXT:
			image->samples = 4;
			image->flags |= IT_BGRA;
			break;
		case GL_RGB:
			image->samples = 3;
			break;
		case GL_BGR_EXT:
			image->samples = 3;
			image->flags |= IT_BGRA;
			break;
		case GL_LUMINANCE_ALPHA:
			image->samples = 2;
			break;
		case GL_LUMINANCE:
			image->samples = 1;
			break;
		case GL_ALPHA:
			image->samples = 1;
			image->flags |= IT_ALPHAMASK;
			break;
		}
		size_t pixelSize = 2;
		if( header->type == GL_UNSIGNED_BYTE )
			pixelSize = image->samples;
		
		for(uint16_t i = 0; i < numMips; i++ )
		{
			uint16_t faceSize = ALIGN(max( header->pixelWidth >> i, 1 ) * pixelSize, 4 ) * max( header->pixelHeight >> i, 1 );
			data += sizeof( int );
			for(size_t j = 0; j < numFaces; j++ )
				bufStart[i * numFaces + j] = data + faceSize * j;
			data += faceSize * numFaces;
		}

		for(uint16_t i = 0; i < numMips; i++) {

		}

	}
	
	//const uint32_t mipSize = ( image->flags & IT_NOMIPMAP ) ? 1 : R_calculateMipMapLevel( flags, image->width, image->height, minmipsize );
	
 // texture_format_t srcFormat = R_ResolveDataFormat( image->flags, tags );
 // texture_format_t destFormat = R_ToSupportedFormat( srcFormat );
	R_FreeFile( buffer );
	return true;
error:
	R_FreeFile( buffer );
	return false;
}


static struct image_s *__R_AllocImage( const char *name )
{
	size_t nameLen = strlen( name );
	unsigned hash = COM_SuperFastHash( (const uint8_t *)name, nameLen, nameLen );
	struct image_s *image = R_LinkPic( hash );
	if( !image ) {
		ri.Com_Error( ERR_DROP, "R_LoadImage: r_numImages == MAX_GLIMAGES" );
	}
	memset(image, 0, sizeof(*image));
	image->name = R_MallocExt( r_imagesPool, nameLen + 1, 0, 1 );
	strcpy( image->name, name );
	image->registrationSequence = rsh.registrationSequence;
	image->extension[0] = '\0';
	image->loaded = true;
	image->missing = false;
	image->fbo = 0;
	return image;
}


static NriTextureUsageBits __R_NRITextureUsageBits(int flags) {
	if( flags & IT_DEPTH ) {
		return NriTextureUsageBits_DEPTH_STENCIL_ATTACHMENT;
	} else if( flags & IT_FRAMEBUFFER ) {
		return NriTextureUsageBits_COLOR_ATTACHMENT;
	}
	return NriTextureUsageBits_SHADER_RESOURCE;
}

static enum texture_format_e __R_ResolveDataFormat( int flags, int samples )
{
	if( flags & IT_DEPTH ) {
		if( flags & IT_STENCIL ) {
			return R_FORMAT_D24_UNORM_S8_UINT;
		} else {
			return R_FORMAT_D32_SFLOAT;
		}
	} else if( flags & IT_FRAMEBUFFER ) {
		if( samples == 4 ) {
			return R_FORMAT_RGBA8_UNORM;
		} else {
			return R_FORMAT_RGB8_UNORM;
		}
	}
	if( samples == 4 ) {
		return ( flags & IT_BGRA ) ? R_FORMAT_BGRA8_UNORM : R_FORMAT_RGBA8_UNORM;
	} else if( samples == 3 ) {
		return ( flags & IT_BGRA ? R_FORMAT_BGR8_UNORM : R_FORMAT_RGB8_UNORM );
	} else if( samples == 2 ) {
		return R_FORMAT_RG8_UNORM;
	} else if( flags & IT_ALPHAMASK ) {
		return R_FORMAT_R8_UNORM;
	}
	return R_FORMAT_R8_UNORM;
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

static enum texture_format_e __R_ToSupportedFormat(enum texture_format_e format) {
  switch(format) {
  	case R_FORMAT_RGB8_UNORM:
			return R_FORMAT_RGBA8_UNORM;
  	case R_FORMAT_BGR8_UNORM:
			return R_FORMAT_RGBA8_UNORM;
  	default:
  		break;
  }
  return format;
}

struct image_s *R_LoadImage( const char *name, uint8_t **pic, int width, int height, int flags, int minmipsize, int tags, int samples )
{
	struct image_s* image = __R_AllocImage( name );
	image->width = width;
	image->height = height;
	image->layers = 1;
	image->flags = flags;
	image->minmipsize = minmipsize;
	image->tags = tags;
	image->samples = samples;
	image->upload_width = width;
	image->upload_height = height;

  const uint32_t mipSize = __R_calculateMipMapLevel( flags, width, height, minmipsize );

  enum texture_format_e srcFormat = __R_ResolveDataFormat( flags, samples);
  enum texture_format_e destFormat = __R_ToSupportedFormat( srcFormat );
  NriTextureDesc textureDesc = { .width = width,
								 .height = height,
								 .depth = 1,
								 .usageMask = __R_NRITextureUsageBits( flags ),
								 .arraySize = ( flags & IT_CUBEMAP ) ? 6 : 1,
								 .format = R_ToNRIFormat( destFormat ),
								 .sampleNum = 1,
								 .type = NriTextureType_TEXTURE_2D,
								 .mipNum = mipSize };
  rsh.nri.coreI.CreateTexture( rsh.nri.device, &textureDesc, &image->texture );

  NriResourceGroupDesc resourceGroupDesc = {
  	.textureNum = 1,
  	.textures = &image->texture,
  	.memoryLocation = NriMemoryLocation_DEVICE,
  };
  const uint32_t allocationNum = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
  assert(allocationNum <= Q_ARRAY_COUNT(image->memory));
  
  rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, image->memory );
  uint32_t srcBlockSize = R_FormatBitSizePerBlock( srcFormat ) / 8;
  uint32_t destBlockSize = R_FormatBitSizePerBlock( destFormat ) / 8;
  uint8_t* tmpBuffer = NULL;
  const size_t reservedSize = width * height * samples;
  if( pic ) {
  	for( size_t index = 0; index < textureDesc.arraySize; index++ ) {
  		if( !pic[index] ) {
  			continue;
  		}
  		uint8_t *buf = pic[index];
  		if( !( flags & IT_CUBEMAP ) && ( flags & ( IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL ) ) ) {
  			arrsetlen(tmpBuffer, reservedSize);
  			R_FlipTexture( buf, tmpBuffer, width, height, samples, ( flags & IT_FLIPX ) ? true : false, ( flags & IT_FLIPY ) ? true : false, ( flags & IT_FLIPDIAGONAL ) ? true : false );
  			buf = tmpBuffer;
  		}

  		uint32_t w = width;
  		uint32_t h = height;
  		for( size_t i = 0; i < mipSize; i++ ) {
  			texture_upload_desc_t uploadDesc = {};
  			uploadDesc.sliceNum = h;
  			uploadDesc.rowPitch = w * destBlockSize;
  			uploadDesc.arrayOffset = index;
  			uploadDesc.mipOffset = i;
  			uploadDesc.target = image->texture;
  			R_ResourceBeginCopyTexture( &uploadDesc );
  			for( size_t slice = 0; slice < height; slice++ ) {
  				const size_t dstRowStart = uploadDesc.alignRowPitch * slice;
  				memset( &( (uint8_t *)uploadDesc.data )[dstRowStart], 255, uploadDesc.rowPitch );
  				for( size_t column = 0; column < width; column++ ) {
  					memcpy( &( (uint8_t *)uploadDesc.data )[dstRowStart + ( destBlockSize * column )], &buf[( width * srcBlockSize ) + ( column * srcBlockSize )],
  							min( srcBlockSize, destBlockSize ) );
  				}
  			}
  			R_ResourceEndCopyTexture( &uploadDesc );
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
  arrfree(tmpBuffer);
	return image;
}

static void R_UnlinkPic( image_t *image )
{
	ri.Mutex_Lock( r_imagesLock );

	if(image->prev && image->next) {
		// remove from linked active list
		image->prev->next = image->next;
		image->next->prev = image->prev;
	}

	// insert into linked free list
	image->next = free_images;
	free_images = image;

	ri.Mutex_Unlock( r_imagesLock );
}

static image_t *R_CreateImage( const char *name, int width, int height, int layers, int flags, int minmipsize, int tags, int samples )
{
	image_t *image;
	int name_len = strlen( name );
	unsigned hash;

	hash = COM_SuperFastHash( ( const uint8_t *)name, name_len, name_len );

	image = R_LinkPic( hash );
	if( !image ) {
		ri.Com_Error( ERR_DROP, "R_LoadImage: r_numImages == MAX_GLIMAGES" );
	}

	image->name = R_MallocExt( r_imagesPool, name_len + 1, 0, 1 );
	strcpy( image->name, name );
	image->width = width;
	image->height = height;
	image->layers = layers;
	image->flags = flags;
	image->minmipsize = minmipsize;
	image->samples = samples;
	image->fbo = 0;
	image->texnum = 0;
	image->registrationSequence = rsh.registrationSequence;
	image->tags = tags;
	image->loaded = true;
	image->missing = false;
	image->extension[0] = '\0';

	return image;
}


image_t *R_Create3DImage( const char *name, int width, int height, int layers, int flags, int tags, int samples, bool array )
{
	image_t *image;
	int scaledWidth, scaledHeight;
	int target, comp, format, type;

	assert( array ? ( layers <= glConfig.maxTextureLayers ) : ( layers <= glConfig.maxTexture3DSize ) );

	flags |= ( array ? IT_ARRAY : IT_3D );

	image = R_CreateImage( name, width, height, layers, flags, 1, tags, samples );
	R_BindImage( image );

	R_ScaledImageSize( width, height, &scaledWidth, &scaledHeight, flags, 1, 1, false );
	image->upload_width = scaledWidth;
	image->upload_height = scaledHeight;

	R_SetupTexParameters( flags, scaledWidth, scaledHeight, 1 );

	R_TextureTarget( flags, &target );
	R_TextureFormat( flags, samples, &comp, &format, &type );

	qglTexImage3DEXT( target, 0, comp, scaledWidth, scaledHeight, layers, 0, format, type, NULL );

	if( !( flags & IT_NOMIPMAP ) )
	{
		int miplevel = 0;
		while( scaledWidth > 1 || scaledHeight > 1 )
		{
			scaledWidth >>= 1;
			scaledHeight >>= 1;
			if( scaledWidth < 1 )
				scaledWidth = 1;
			if( scaledHeight < 1 )
				scaledHeight = 1;
			qglTexImage3DEXT( target, miplevel++, comp, scaledWidth, scaledHeight, layers, 0, format, type, NULL );
		}
	}

	return image;
}

static void R_FreeImage( struct image_s *image )
{

	rsh.nri.coreI.DestroyTexture(image->texture);
	R_Free( image->name );
	for(size_t i = 0; i < image->numAllocations; i++) {
		rsh.nri.coreI.FreeMemory(image->memory[i]);
	}
	image->name = NULL;
	image->registrationSequence = 0;
	R_UnlinkPic( image );
}

/*
* R_ReplaceImage
*
* FIXME: not thread-safe!
*/
void R_ReplaceImage( image_t *image, uint8_t **pic, int width, int height, int flags, int minmipsize, int samples )
{
	assert( image );
	assert( image->texnum );

	R_BindImage( image );

	if( image->width != width || image->height != height || image->samples != samples )
		R_Upload32( QGL_CONTEXT_MAIN, pic, 0, 0, 0, width, height, flags, minmipsize,
		&(image->upload_width), &(image->upload_height), samples, false, false );
	else
		R_Upload32( QGL_CONTEXT_MAIN, pic, 0, 0, 0, width, height, flags, minmipsize,
		&(image->upload_width), &(image->upload_height), samples, true, false );

	if( !(image->flags & IT_NO_DATA_SYNC) )
		R_DeferDataSync();

	image->flags = flags;
	image->width = width;
	image->height = height;
	image->layers = 1;
	image->minmipsize = minmipsize;
	image->samples = samples;
	image->registrationSequence = rsh.registrationSequence;
}

/*
* R_ReplaceSubImage
*/
void R_ReplaceSubImage( image_t *image, int layer, int x, int y, uint8_t **pic, int width, int height )
{
	assert( image );
	assert( image->texnum );

	R_BindImage( image );

	R_Upload32( QGL_CONTEXT_MAIN, pic, layer, x, y, width, height, image->flags, image->minmipsize,
		NULL, NULL, image->samples, true, true );

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
	assert( image->texnum );

	R_BindImage( image );

	R_Upload32( QGL_CONTEXT_MAIN, pic, layer, 0, 0, image->width, image->height, image->flags, image->minmipsize,
		NULL, NULL, image->samples, true, false );

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
  struct image_s* image = NULL;
	const size_t reserveSize = strlen( name ) + ( suffix ? strlen( suffix ) : 0 ) + 15;

	sds resolvedPath = sdsnewlen( 0, reserveSize );
	sdsclear( resolvedPath );
	size_t lastDot = -1;
	size_t lastSlash = -1;
	for( size_t i = ( name[0] == '/' || name[0] == '\\' ); name[i]; i++ ) {
		const char c = name[i];
		if( c == '\\' ) {
			resolvedPath = sdscat( resolvedPath, "/" );
		} else {
			resolvedPath = sdscatfmt( resolvedPath, "%c", tolower( c ) );
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
	sdsupdatelen( resolvedPath );
	if( sdslen( resolvedPath ) < 5 )
		return NULL;

	// don't confuse paths such as /ui/xyz.cache/123 with file extensions
	if( lastDot < lastSlash ) {
		lastDot = -1;
	}
	
	if( suffix ) {
		resolvedPath = strcat(resolvedPath, suffix);
	}

	// look for it
	const uint32_t key = COM_SuperFastHash( (const uint8_t *)resolvedPath, strlen(resolvedPath), strlen(resolvedPath) ) % IMAGES_HASH_SIZE;
	const image_t *hnode = &images_hash_headnode[key];
	const uint32_t searchFlags = flags & ~IT_LOADFLAGS;
	for( image_t *it = hnode->prev; it != hnode; it = it->prev ) {
		if( ( ( it->flags & ~IT_LOADFLAGS ) == searchFlags ) && !strcmp( image->name, resolvedPath ) && ( image->minmipsize == minmipsize ) ) {
			R_TouchImage( it, tags );
			image = it;
			goto done;
		}
	}

	image = __R_AllocImage( resolvedPath );
	image->layers = 1;
	image->flags = flags;
	image->minmipsize = minmipsize;
	image->tags = tags;
	
	const bool isCubeMap = flags & IT_CUBEMAP; 

  enum image_ext_type_e {
  	IMAGE_EXT_TGA,
  	IMAGE_EXT_JPG,
  	IMAGE_EXT_PNG,
  	IMAGE_EXT_KTX
  };

	const char* imageExtension[] = {
		[IMAGE_EXT_TGA] = ".tga", 
		[IMAGE_EXT_JPG] = ".jpg", 
		[IMAGE_EXT_PNG] = ".png", 
		[IMAGE_EXT_KTX] = ".ktx"
	};
	const char* extension = ri.FS_FirstExtension( resolvedPath, imageExtension, Q_ARRAY_COUNT(imageExtension)); // last is KTX
	const size_t basePathLen = sdslen(resolvedPath);
	if(extension == imageExtension[IMAGE_EXT_KTX]) {
		resolvedPath = sdscat(resolvedPath, extension);
		if(__R_LoadKTX(image, resolvedPath)) {
			goto done;
		}
		sdssetlen(resolvedPath, basePathLen); // truncate the pathext  
	}
	
	uint8_t* buffers[6] = {};
	size_t bufferCount = 0;

	struct UploadImgBuffer {
	  enum texture_format_e formatType;	
		uint8_t* buffer;
		stbi_uc* stbi;
		int width;
		int height;
		int flags;
	} uploads[6] = {};
	size_t uploadRawImgCount = 0;
	if( isCubeMap) {
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

		//resolvedPath[pathLen] = '_';
		for(size_t i = 0; i < 2; i++ ) {
			for(uploadRawImgCount = 0; uploadRawImgCount < 6; uploadRawImgCount++ ) {
				sdssetlen( resolvedPath, basePathLen );
				resolvedPath = sdscatfmt( resolvedPath, "_%s.tga", cubemapSides[i][uploadRawImgCount].suf );
				sdsIncrLen( resolvedPath, 0 );
				struct UploadImgBuffer *upload = &uploads[uploadRawImgCount];

				upload->buffer = __loadFileImageStbi( resolvedPath, 
												&upload->width, 
												&upload->height,
												&upload->formatType
												);
				upload->stbi = uploads->buffer;

				if( !upload->buffer) {
					break;
			  }
			  if( uploads[uploadRawImgCount].width != uploads[uploadRawImgCount].height ) {
				  ri.Com_DPrintf( S_COLOR_YELLOW "Not square cubemap image %s\n", resolvedPath );
				  break;
			  }
			  if(uploads[uploadRawImgCount].width != uploads[0].width) {
				  ri.Com_DPrintf( S_COLOR_YELLOW "Different cubemap image size: %s\n", resolvedPath );
				  break;
			  }
			  uploads[uploadRawImgCount].flags = cubemapSides[i][uploadRawImgCount].flags;
			}
		}
		sdssetlen(resolvedPath, basePathLen); // truncate the pathext + cubemap 
		if(uploadRawImgCount != 6) {
			ri.Com_DPrintf( S_COLOR_YELLOW "Missing image: %s\n", image->name );
			image = NULL;
			R_FreeImage( image );
			goto done;
		}
	} else if(extension) {
		resolvedPath = sdscat(resolvedPath, extension);
		struct UploadImgBuffer *upload = &uploads[uploadRawImgCount++];

		upload->buffer = __loadFileImageStbi( resolvedPath, &upload->width, &upload->height, &upload->formatType );
		upload->stbi = uploads->buffer;
		if(!upload->buffer) {
			ri.Com_DPrintf( S_COLOR_YELLOW "Missing image: %s\n", image->name );
			R_FreeImage( image );
			image = NULL;
			goto done;
		}
	} else {
		ri.Com_DPrintf( S_COLOR_YELLOW "Can't find Image: %s\n", image->name );
		R_FreeImage( image );
		image = NULL;
		goto done;
	}

	const uint32_t mipSize =  __R_calculateMipMapLevel( flags, uploads[0].width, uploads[0].height, minmipsize );
	const enum texture_format_e format = R_FORMAT_RGBA8_UNORM;
	const uint32_t destBlockSize = R_FormatBitSizePerBlock( format ) / 8;
	NriTextureDesc textureDesc = { 
		               .width = uploads[0].width,
								   .height = uploads[0].height,
								   .usageMask = __R_NRITextureUsageBits( flags ),
								   .arraySize = uploadRawImgCount,
								   .format = R_ToNRIFormat( format ),
								   .sampleNum = 1,
								   .type = NriTextureType_TEXTURE_2D,
								   .mipNum = mipSize };
	if(rsh.nri.coreI.CreateTexture( rsh.nri.device, &textureDesc, &image->texture ) != NriResult_SUCCESS) {
		ri.Com_Printf( S_COLOR_YELLOW "Failed to Load Image: %s\n", image->name );
		R_FreeImage( image );
		image = NULL;
		goto done;
	}

	NriResourceGroupDesc resourceGroupDesc = {
		.textureNum = 1,
		.textures = &image->texture,
		.memoryLocation = NriMemoryLocation_DEVICE,
	};
	const size_t numAllocations = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
	if(numAllocations >= Q_ARRAY_COUNT( image->memory ) ) {
		ri.Com_Printf( S_COLOR_YELLOW "Failed Allocation: %s\n", image->name );
		R_FreeImage( image );
		image = NULL;
		goto done;
	}
	image->numAllocations = 0;
	if(rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, image->memory )) {
		ri.Com_Printf( S_COLOR_YELLOW "Failed Allocation: %s\n", image->name );
		R_FreeImage( image );
		image = NULL;
		goto done;
	}
	uint8_t *tmpBuffer = NULL;
	for( size_t index = 0; index < uploadRawImgCount; index++ ) {
		uint8_t *buf = uploads[index].buffer;
		const uint8_t channelCount = R_FormatChannelCount(uploads[index].formatType);

		const size_t reservedSize = uploads[index].width * uploads[index].height * R_FormatBitSizePerBlock(uploads[index].formatType); //imgUploadElements[index].channelCount;
		if( !isCubeMap && ( flags & ( IT_FLIPX | IT_FLIPY | IT_FLIPDIAGONAL ) ) ) {
			arrsetlen( tmpBuffer, reservedSize );
			R_FlipTexture( buf, tmpBuffer, uploads[index].width, uploads[index].height, channelCount, ( flags & IT_FLIPX ) ? true : false, ( flags & IT_FLIPY ) ? true : false,
						   ( flags & IT_FLIPDIAGONAL ) ? true : false );
			buf = tmpBuffer;
		}

		uint32_t w = uploads[index].width;
		uint32_t h = uploads[index].height;
		for( size_t i = 0; i < mipSize; i++ ) {
			texture_upload_desc_t uploadDesc = {};
			uploadDesc.sliceNum = h;
			uploadDesc.rowPitch = w * destBlockSize;
			uploadDesc.arrayOffset = index;
			uploadDesc.mipOffset = i;
			uploadDesc.target = image->texture;
			R_ResourceBeginCopyTexture( &uploadDesc );
			for( size_t slice = 0; slice < uploads[index].height; slice++ ) {
				const size_t dstRowStart = uploadDesc.alignRowPitch * slice;
				memset( &( (uint8_t *)uploadDesc.data )[dstRowStart], 255, uploadDesc.rowPitch );
				for( size_t column = 0; column < uploads[index].width; column++ ) {
					memcpy( &( (uint8_t *)uploadDesc.data )[dstRowStart + ( destBlockSize * column )], &buf[( uploads[index].width * channelCount ) + ( column * channelCount )],
							min( channelCount, destBlockSize ) );
				}
			}
			R_ResourceEndCopyTexture( &uploadDesc );
			w >>= 1;
			h >>= 1;
			if( w == 0 ) {
				w = 1;
			}
			if( h == 0 ) {
				h = 1;
			}
			R_MipMap( buf, w, h, channelCount, 1 );
		}
	}

done:
	for(size_t i = 0; i < uploadRawImgCount; i++ ) {
		if( uploads[i].stbi ) {
			stbi_image_free( uploads[i].stbi );
		}
	}

	sdsfree( resolvedPath );
	return image;
}

/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/

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
			R_Free( r_screenShotBuffer );
		}
		r_screenShotBuffer = R_MallocExt( r_imagesPool, buf_size, 0, 1 );
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
* R_GetViewportTextureSize
*/
static void R_GetViewportTextureSize( const int viewportWidth, const int viewportHeight, 
	const int size, const int flags, int *width, int *height )
{
	int limit;
	int width_, height_;
	bool npot;

	// limit the texture size to either screen resolution in case we can't use FBO
	// or hardware limits and ensure it's a POW2-texture if we don't support such textures
	limit = glConfig.maxRenderbufferSize;
	if( size )
		limit = min( limit, size );
	if( limit < 1 )
		limit = 1;
	width_ = height_ = limit;

	npot = glConfig.ext.texture_non_power_of_two;
#ifdef GL_ES_VERSION_2_0
	npot = npot || ( ( flags & ( IT_CLAMP|IT_NOMIPMAP ) ) == ( IT_CLAMP|IT_NOMIPMAP ) );
#endif
	if( npot )
	{
		width_ = min( viewportWidth, limit );
		height_ = min( viewportHeight, limit );
	}
	else
	{
		int d;

		// calculate the upper bound and make sure it's not a pow of 2
		d = min( limit, viewportWidth );
		if( ( d & (d-1) ) == 0 ) d--;
		for( width_ = 2; width_ <= d; width_ <<= 1 );

		d = min( limit, viewportHeight );
		if( ( d & (d-1) ) == 0 ) d--;
		for( height_ = 2; height_ <= d; height_ <<= 1 );

		if( size ) {
			while( width_ > size || height_ > size ) {
				width_ >>= 1;
				height_ >>= 1;
			}
		}
	}

	*width = width_;
	*height = height_;
}

/*
* R_InitViewportTexture
*/
void R_InitViewportTexture( image_t **texture, const char *name, int id, 
	int viewportWidth, int viewportHeight, int size, int flags, int tags, int samples )
{
	int width, height;
	image_t *t;

	R_GetViewportTextureSize( viewportWidth, viewportHeight, size, flags, &width, &height );

	// create a new texture or update the old one
	if( !( *texture ) || ( *texture )->width != width || ( *texture )->height != height )
	{
		uint8_t *data = NULL;

		if( !*texture ) {
			char uploadName[128];

			Q_snprintfz( uploadName, sizeof( uploadName ), "***%s_%i***", name, id );
			t = *texture = R_LoadImage( uploadName, &data, width, height, flags, 1, tags, samples );
		}
		else { 
			t = *texture;
			t->width = width;
			t->height = height;

			R_BindImage( t );

			R_Upload32( QGL_CONTEXT_MAIN, &data, 0, 0, 0, width, height, flags, 1,
				&t->upload_width, &t->upload_height, t->samples, false, false );
		}

		// update FBO, if attached
		if( t->fbo ) {
			RFB_UnregisterObject( t->fbo );
			t->fbo = 0;
		}
		if( t->flags & IT_FRAMEBUFFER ) {
			t->fbo = RFB_RegisterObject( t->upload_width, t->upload_height, ( tags & IMAGE_TAG_BUILTIN ) != 0,
				( flags & IT_DEPTHRB ) != 0, ( flags & IT_STENCIL ) != 0 );
			RFB_AttachTextureToObject( t->fbo, t );
		}
	}
}

/*
* R_GetPortalTextureId
*/
static int R_GetPortalTextureId( const int viewportWidth, const int viewportHeight, 
	const int flags, unsigned frameNum )
{
	int i;
	int best = -1;
	int realwidth, realheight;
	int realflags = IT_SPECIAL|IT_FRAMEBUFFER|IT_DEPTHRB|flags;
	image_t *image;

	R_GetViewportTextureSize( viewportWidth, viewportHeight, r_portalmaps_maxtexsize->integer, 
		flags, &realwidth, &realheight );

	for( i = 0; i < MAX_PORTAL_TEXTURES; i++ )
	{
		image = rsh.portalTextures[i];
		if( !image )
			return i;

		if( image->framenum == frameNum ) {
			// the texture is used in the current scene
			continue;
		}

		if( image->width == realwidth && 
			image->height == realheight && 
			image->flags == realflags ) {
			// 100% match
			return i;
		}

		if( best < 0 ) {
			// in case we don't get a 100% matching texture later,
			// reuse this one
			best = i;
		}
	}

	return best;
}

/*
* R_GetPortalTexture
*/
image_t *R_GetPortalTexture( int viewportWidth, int viewportHeight, 
	int flags, unsigned frameNum )
{
	int id;

	if( glConfig.stencilBits )
		flags |= IT_STENCIL;

	id = R_GetPortalTextureId( viewportWidth, viewportHeight, flags, frameNum );
	if( id < 0 || id >= MAX_PORTAL_TEXTURES ) {
		return NULL;
	}

	R_InitViewportTexture( &rsh.portalTextures[id], "r_portaltexture", id, 
		viewportWidth, viewportHeight, r_portalmaps_maxtexsize->integer, 
		IT_SPECIAL|IT_FRAMEBUFFER|IT_DEPTHRB|flags, IMAGE_TAG_GENERIC,
		glConfig.forceRGBAFramebuffers ? 4 : 3 );

	if( rsh.portalTextures[id] ) {
		rsh.portalTextures[id]->framenum = frameNum;
	}

	return rsh.portalTextures[id];
}

/*
* R_GetShadowmapTexture
*/
image_t *R_GetShadowmapTexture( int id, int viewportWidth, int viewportHeight, int flags )
{
	int samples;

	if( id < 0 || id >= MAX_SHADOWGROUPS ) {
		return NULL;
	}

	if( glConfig.ext.shadow ) {
		// render to depthbuffer, GL_ARB_shadow path
		flags |= IT_DEPTH;
		samples = 1;
	} else {
		flags |= IT_NOFILTERING;
		samples = 3;
	}

	R_InitViewportTexture( &rsh.shadowmapTextures[id], "r_shadowmap", id, 
		viewportWidth, viewportHeight, r_shadows_maxtexsize->integer, 
		IT_SPECIAL|IT_FRAMEBUFFER|IT_DEPTHCOMPARE|flags, IMAGE_TAG_GENERIC, samples );

	return rsh.shadowmapTextures[id];
}

/*
* R_InitScreenImagePair
*/
static void R_InitScreenImagePair( const char *name, image_t **color, image_t **depth, bool stencil )
{
	char tn[128];
	int flags, colorFlags, depthFlags;

	assert( !depth || glConfig.ext.depth_texture );

	if( !glConfig.stencilBits )
		stencil = false;

	flags = IT_SPECIAL;

	colorFlags = flags | IT_FRAMEBUFFER;
	depthFlags = flags | ( IT_DEPTH|IT_NOFILTERING );
	if( !depth ) {
		colorFlags |= IT_DEPTHRB;
	}
	if( stencil ) {
		if( depth ) {
			depthFlags |= IT_STENCIL;
		} else {
			colorFlags |= IT_STENCIL;
		}
	}

	if( color ) {
		R_InitViewportTexture( color, name, 
			0, glConfig.width, glConfig.height, 0, colorFlags, IMAGE_TAG_BUILTIN,
			glConfig.forceRGBAFramebuffers ? 4 : 3 );
	}
	if( depth && *color ) {
		R_InitViewportTexture( depth, va_r( tn, sizeof( tn ), "%s_depth", name ), 
			0, glConfig.width, glConfig.height, 0, depthFlags, IMAGE_TAG_BUILTIN, 1 );
		RFB_AttachTextureToObject( (*color)->fbo, *depth );
	}
}

/*
* R_InitBuiltinScreenImages
*
 * Screen textures may only be used in or referenced from the rendering context/thread.
*/
void R_InitBuiltinScreenImages( void )
{
	if( glConfig.ext.depth_texture && glConfig.ext.fragment_precision_high && glConfig.ext.framebuffer_blit )
	{
		R_InitScreenImagePair( "r_screentex", &rsh.screenTexture, &rsh.screenDepthTexture, true );

		// Stencil is required in the copy for depth/stencil formats to match when blitting.
		R_InitScreenImagePair( "r_screentexcopy", &rsh.screenTextureCopy, &rsh.screenDepthTextureCopy, true );
	}

	R_InitScreenImagePair( "rsh.screenPPCopy0", &rsh.screenPPCopies[0], NULL, true );
	R_InitScreenImagePair( "rsh.screenPPCopy1", &rsh.screenPPCopies[1], NULL, false );
}

/*
* R_ReleaseBuiltinScreenImages
*/
void R_ReleaseBuiltinScreenImages( void )
{
	if( rsh.screenTexture ) {
		R_FreeImage( rsh.screenTexture );
	}
	if( rsh.screenDepthTexture ) {
		R_FreeImage( rsh.screenDepthTexture );
	}

	if( rsh.screenTextureCopy ) {
		R_FreeImage( rsh.screenTextureCopy );
	}
	if( rsh.screenDepthTextureCopy ) {
		R_FreeImage( rsh.screenDepthTextureCopy );
	}

	if( rsh.screenPPCopies[0] ) {
		R_FreeImage( rsh.screenPPCopies[0] );
	}
	if( rsh.screenPPCopies[1] ) {
		R_FreeImage( rsh.screenPPCopies[1] );
	}

	rsh.screenTexture = rsh.screenDepthTexture = NULL;
	rsh.screenTextureCopy = rsh.screenDepthTextureCopy = NULL;
	rsh.screenPPCopies[0] = NULL;
	rsh.screenPPCopies[1] = NULL;
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

	r_imagesPool = R_AllocPool( r_mempool, "Images" );
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
	if( image->fbo ) {
		RFB_TouchObject( image->fbo );
	}
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
		if( !image->name ) {
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

		R_FreeImage( image );
	}
}

/*
* R_FreeUnusedImages
*/
void R_FreeUnusedImages( void )
{
	R_FreeUnusedImagesByTags( ~IMAGE_TAG_BUILTIN );

	//R_FinishLoadingImages();

	memset( rsh.portalTextures, 0, sizeof( image_t * ) * MAX_PORTAL_TEXTURES );
	memset( rsh.shadowmapTextures, 0, sizeof( image_t * ) * MAX_SHADOWGROUPS );
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

 // for( i = 0; i < NUM_LOADER_THREADS; i++ ) {
 // 	R_ShutdownImageLoader( i );
 // }

	R_ReleaseBuiltinImages();

	for( i = 0, image = images; i < MAX_GLIMAGES; i++, image++ ) {
		if( !image->name ) {
			// free texture
			continue;
		}
		R_FreeImage( image );
	}

	R_FreeImageBuffers();

	ri.Mutex_Destroy( &r_imagesLock );

	R_FreePool( &r_imagesPool );

	r_screenShotBuffer = NULL;
	r_screenShotBufferSize = 0;

	memset( rsh.portalTextures, 0, sizeof( rsh.portalTextures ) );
	memset( rsh.shadowmapTextures, 0, sizeof( rsh.shadowmapTextures ) );

}

typedef struct
{
	int id;
	int self;
} loaderInitCmd_t;

typedef struct
{
	int id;
	int self;
	int pic;
} loaderPicCmd_t;

//typedef unsigned (*queueCmdHandler_t)( const void * );
//static void *R_ImageLoaderThreadProc( void *param );

//static void R_IssueInitLoaderCmd( int id )
//{
//	loaderInitCmd_t cmd;
//	cmd.id = CMD_LOADER_INIT;
//	cmd.self = id;
//	ri.BufPipe_WriteCmd( loader_queue[id], &cmd, sizeof( cmd ) );
//}
//static void R_IssueShutdownLoaderCmd( int id )
//{
//	int cmd;
//	cmd = CMD_LOADER_SHUTDOWN;
//	ri.BufPipe_WriteCmd( loader_queue[id], &cmd, sizeof( cmd ) );
//}

//static void R_IssueLoadPicLoaderCmd( int id, int pic )
//{
//	loaderPicCmd_t cmd;
//	cmd.id = CMD_LOADER_LOAD_PIC;
//	cmd.self = id;
//	cmd.pic = pic;
//	ri.BufPipe_WriteCmd( loader_queue[id], &cmd, sizeof( cmd ) );
//}
//static void R_IssueDataSyncLoaderCmd( int id )
//{
//	int cmd;
//	cmd = CMD_LOADER_DATA_SYNC;
//	ri.BufPipe_WriteCmd( loader_queue[id], &cmd, sizeof( cmd ) );
//}

/*
* R_FinishLoadingImages
*/
//void R_FinishLoadingImages( void )
//{
//	int i;
//
//	for( i = 0; i < NUM_LOADER_THREADS; i++ ) {
//		if( loader_gl_context[i] ) {
//			R_IssueDataSyncLoaderCmd( i );
//		}
//	}
//
//	for( i = 0; i < NUM_LOADER_THREADS; i++ ) {
//		if( loader_gl_context[i] ) {
//			ri.BufPipe_Finish( loader_queue[i] );
//		}
//	}
//}

///*
//* R_LoadAsyncImageFromDisk
//*/
//static bool R_LoadAsyncImageFromDisk( image_t *image )
//{
//	int id;
//
//	if( loader_gl_context[0] == NULL ) {
//		return false;
//	}
//
//	id = (image - images) % NUM_LOADER_THREADS;
//	if ( loader_gl_context[id] == NULL ) {
//		id = 0;
//	}
//
//	image->loaded = false;
//	image->missing = false;
//	
//	// Unbind and finish so that the image resource becomes available in the loader's context.
//	// Not doing finish (or only doing flush instead) causes missing textures on Nvidia and possibly other GPUs,
//	// since the loader thread is woken up pretty much instantly, and the GL calls that initialize the texture
//	// may still be processed or only queued in the main thread while the loader is trying to load the image.
//	R_UnbindImage( image );
//	qglFinish();
//
//	R_IssueLoadPicLoaderCmd( id, image - images );
//	return true;
//}

///*
//* R_ShutdownImageLoader
//*/
//static void R_ShutdownImageLoader( int id )
//{
//	void *context = loader_gl_context[id];
//	void *surface = loader_gl_surface[id];
//
//	loader_gl_context[id] = NULL;
//	loader_gl_surface[id] = NULL;
//	if( !context ) {
//		return;
//	}
//
//	R_IssueShutdownLoaderCmd( id );
//
//	ri.BufPipe_Finish( loader_queue[id] );
//
//	ri.Thread_Join( loader_thread[id] );
//	loader_thread[id] = NULL;
//
//	ri.BufPipe_Destroy( &loader_queue[id] );
//
//	GLimp_SharedContext_Destroy( context, surface );
//}


//static void *R_ImageLoaderThreadProc( void *param )
//{
////	qbufPipe_t *cmdQueue = param;
////	queueCmdHandler_t cmdHandlers[NUM_LOADER_CMDS] = 
////	{
////		(queueCmdHandler_t)R_HandleInitLoaderCmd,
////		(queueCmdHandler_t)R_HandleShutdownLoaderCmd,
////		(queueCmdHandler_t)R_HandleLoadPicLoaderCmd,
////		(queueCmdHandler_t)R_HandleDataSyncLoaderCmd,
////	};
////
////	ri.BufPipe_Wait( cmdQueue, R_ImageLoaderCmdsWaiter, cmdHandlers, Q_THREADS_WAIT_INFINITE );
// 
//	return NULL;	
//}
