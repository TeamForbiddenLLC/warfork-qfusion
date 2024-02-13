#include "r_ktx_loader.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"

#include "r_texture_buf.h"
#include "r_texture_format.h"
#include "gl_format.h"

#include "stb_ds.h"

#pragma pack( push, 1 )
struct __raw_ktx_header_s {
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
};
#pragma pack( pop )

static struct ktx_image_s *R_KTXGetImage( const struct ktx_context_s *cntx, uint32_t mipLevel, uint32_t faceIndex, uint32_t arrayOffset )
{
	assert( cntx );
	assert( cntx->textures);
	const size_t numberOfArrayElements = max( 1, cntx->numberOfArrayElements );
	const size_t numberOfFaces = max( 1, cntx->numberOfFaces );
	const size_t index = ( mipLevel * numberOfArrayElements * numberOfFaces ) + ( faceIndex * numberOfArrayElements ) + arrayOffset;
	assert( index < arrlen( cntx->textures) );
	return &cntx->textures[index];
}

struct texture_buf_s *R_KTXResolveBuffer( struct ktx_context_s *cntx, uint32_t mipLevel, uint32_t faceIndex, uint32_t arrayOffset ) {
	struct ktx_image_s *img = R_KTXGetImage( cntx, mipLevel, faceIndex, arrayOffset);
	if(!img->updated) {
		switch( cntx->desc->base ) {
			case R_BASE_FORMAT_FIXED_16:
			case R_BASE_FORMAT_FIXED_32:
			case R_BASE_FORMAT_PACKED_16:
			  T_SwapEndianness( &img->texture );
				break;
			default:
				break;
		}
		img->updated = true;
	}
	return &img->texture;
}
bool R_InitKTXContext(struct ktx_context_s *cntx, uint8_t *memory, size_t size, struct ktx_context_err_s* err) {
	assert( sizeof( struct __raw_ktx_header_s ) == 64 );
	struct __raw_ktx_header_s *rawHeader = (struct __raw_ktx_header_s *)memory;
	if( memcmp( rawHeader->identifier, "\xABKTX 11\xBB\r\n\x1A\n", 12 ) ) {
		return KTX_ERR_INVALID_IDENTIFIER;
	}
  assert(cntx->pixelDepth == 0);

	cntx->buffer = memory;
	cntx->swapEndianess = ( rawHeader->endianness == 0x01020304 ) ? true : false;
	cntx->type = cntx->swapEndianess ? LongSwap( rawHeader->type ) : rawHeader->type;
	cntx->typeSize = cntx->swapEndianess  ? LongSwap( rawHeader->typeSize ) : rawHeader->typeSize;
	cntx->format = cntx->swapEndianess ? LongSwap( rawHeader->format ) : rawHeader->format;
	cntx->internalFormat = cntx->swapEndianess ? LongSwap( rawHeader->internalFormat ) : rawHeader->internalFormat;
	cntx->baseInternalFormat = cntx->swapEndianess ? LongSwap( rawHeader->baseInternalFormat ) : rawHeader->baseInternalFormat;
	cntx->pixelWidth = cntx->swapEndianess ? LongSwap( rawHeader->pixelWidth ) : rawHeader->pixelWidth;
	cntx->pixelHeight = cntx->swapEndianess ? LongSwap( rawHeader->pixelHeight ) : rawHeader->pixelHeight;
	cntx->pixelDepth = cntx->swapEndianess ? LongSwap( rawHeader->pixelDepth ) : rawHeader->pixelDepth;
	cntx->numberOfArrayElements = cntx->swapEndianess ? LongSwap( rawHeader->numberOfArrayElements ) : rawHeader->numberOfArrayElements;
	cntx->numberOfFaces = cntx->swapEndianess ? LongSwap( rawHeader->numberOfFaces ) : rawHeader->numberOfFaces;
	cntx->numberOfMipmapLevels = cntx->swapEndianess ? LongSwap( rawHeader->numberOfMipmapLevels ) : rawHeader->numberOfMipmapLevels;
	cntx->bytesOfKeyValueData = cntx->swapEndianess ? LongSwap( rawHeader->bytesOfKeyValueData ) : rawHeader->bytesOfKeyValueData;

  if((cntx->pixelWidth <= 0 ) || ( cntx->pixelHeight <= 0)) {
  	return KTX_ERR_ZER_TEXTURE_SIZE;
  }

	const size_t numberOfArrayElements = max( 1, cntx->numberOfArrayElements );
	const size_t numberOfFaces = max( 1, cntx->numberOfFaces );
	const size_t numberOfMips = max( 1, cntx->numberOfMipmapLevels );

	//const size_t keyValueOffset = sizeof(struct __raw_ktx_header_s);
	const size_t dataOffset = sizeof( struct __raw_ktx_header_s ) + cntx->bytesOfKeyValueData;
	arrsetlen( cntx->textures, numberOfArrayElements * numberOfFaces * numberOfMips );
	if( cntx->type == 0 ) {
		switch( cntx->internalFormat ) {
			case GL_ETC1_RGB8_OES:
				cntx->desc = R_BaseFormatDef( R_FORMAT_ETC1_R8G8B8_OES );
				break;
			case GL_COMPRESSED_RGB8_ETC2:
				cntx->desc  = R_BaseFormatDef( R_FORMAT_ETC2_R8G8B8_UNORM );
				break;
			default:
				err->type = KTX_ERR_UNHANDLED_TEXTURE_TYPE;
				err->errTextureType.type = cntx->type;
				err->errTextureType.type = cntx->baseInternalFormat;
				goto error;
		}
	} else {
		switch( cntx->type ) {
			case GL_UNSIGNED_SHORT_4_4_4_4:
				cntx->desc = R_BaseFormatDef( R_FORMAT_R4_G4_B4_A4_UNORM );
				break;
			case GL_UNSIGNED_SHORT_5_6_5:
				cntx->desc = R_BaseFormatDef( R_FORMAT_R5_G6_B5_UNORM );
				break;
			case GL_UNSIGNED_SHORT_5_5_5_1:
				cntx->desc = R_BaseFormatDef( R_FORMAT_R5_G5_B5_A1_UNORM );
				break;
			case GL_UNSIGNED_BYTE: {
				switch( cntx->baseInternalFormat ) {
					case GL_RGBA:
				    cntx->desc = R_BaseFormatDef( R_FORMAT_RGBA8_UNORM );
						break;
					case GL_BGRA:
				    cntx->desc = R_BaseFormatDef( R_FORMAT_BGRA8_UNORM );
						break;
					case GL_RGB:
				    cntx->desc = R_BaseFormatDef( R_FORMAT_RGB8_UNORM );
						break;
					case GL_BGR:
				    cntx->desc = R_BaseFormatDef( R_FORMAT_BGR8_UNORM );
						break;
					case GL_LUMINANCE_ALPHA:
				    cntx->desc = R_BaseFormatDef( R_FORMAT_L8_A8_UNORM );
						break;
					case GL_LUMINANCE:
				    cntx->desc = R_BaseFormatDef( R_FORMAT_L8_UNORM );
						break;
					case GL_ALPHA:
				    cntx->desc = R_BaseFormatDef( R_FORMAT_A8_UNORM );
						break;
					default:
						err->type = KTX_ERR_UNHANDLED_TEXTURE_TYPE;
						err->errTextureType.type = cntx->type;
						err->errTextureType.type = cntx->baseInternalFormat;
						goto error;
				}
				break;
			}
			default:
				err->type = KTX_ERR_UNHANDLED_TEXTURE_TYPE;
				err->errTextureType.type = cntx->type;
				err->errTextureType.type = cntx->baseInternalFormat;
			  goto error;
		}
	}

	// mip and faces are aligned 4
	// KTX File Format Spec: https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html
	uint32_t width = cntx->pixelWidth;
	uint32_t height = cntx->pixelHeight;
	size_t offset = dataOffset;
	for( uint_fast16_t mipLevel = 0; mipLevel < numberOfMips; mipLevel++ ) {
		size_t faceLodSize  = *((uint32_t *)&cntx->buffer[offset]);
		faceLodSize = (cntx->swapEndianess == true) ? LongSwap(faceLodSize) : faceLodSize;
		faceLodSize = ALIGN(faceLodSize, 4); // pad buffer to multiple of 4
		offset += sizeof( uint32_t );
	  if( faceLodSize + offset > size ) {
		err->type = KTX_ERR_TRUNCATED;
		err->errTruncated.size = size;
		err->errTruncated.expected = faceLodSize + offset;
		goto error;
	  }

		for( size_t faceIdx = 0; faceIdx < numberOfFaces; faceIdx++ ) {
			size_t arrByteOffset = 0;
			for( size_t arrayIdx = 0; arrayIdx < numberOfArrayElements; arrayIdx++ ) {
				struct ktx_image_s *img = R_KTXGetImage( cntx, mipLevel, faceIdx, arrayIdx );
				memset( img, 0, sizeof( struct ktx_image_s ) );
				struct texture_buf_desc_s desc = { .width = width, .height = height, .alignment = 4, .def = cntx->desc };
				T_AliasTextureBuf( &img->texture, &desc, ( cntx->buffer + offset + arrByteOffset), 0 );
				arrByteOffset += img->texture.size;
			}
			if(faceLodSize + offset > size) {
				err->type = KTX_ERR_TRUNCATED;
				err->errTruncated.size = size;
				err->errTruncated.expected = faceLodSize + offset;
				goto error;
			}
			offset += faceLodSize;
		}

		width = max( 1, width >> 1 );
		height = max( 1, height >> 1 );
	}
  return true;
error:
  return false;
}
uint16_t R_KTXGetNumberMips( const struct ktx_context_s *cntx )
{
	assert(cntx);
	return max( 1, cntx->numberOfMipmapLevels );
}
uint16_t R_KTXGetNumberFaces( const struct ktx_context_s *cntx )
{
	assert(cntx);
	return max( 1, cntx->numberOfFaces );
}
uint16_t R_KTXGetNumberArrayElements( const struct ktx_context_s *cntx )
{
	assert(cntx);
	return max( 1, cntx->numberOfArrayElements );
}

void R_KTXFreeContext( struct ktx_context_s *context )
{
	assert( context );
	arrfree( context->textures );
	context->textures = 0;
}

bool R_KTXIsCompressed(struct ktx_context_s* cntx) {
	assert( cntx);
	return cntx->type == 0;
}

uint16_t R_KTXWidth(const struct ktx_context_s* cntx) {
	assert( cntx);
	return cntx->pixelWidth;
}
uint16_t R_KTXHeight(const struct ktx_context_s* cntx) {
	assert( cntx);
	return cntx->pixelHeight;
}
