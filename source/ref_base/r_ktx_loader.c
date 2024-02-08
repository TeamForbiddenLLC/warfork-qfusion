#include "r_ktx_loader.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "stb_ds.h"

#include "r_image_buf_manipulator.h"
#include "r_texture_format.h"

#include "gl_format.h"

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

struct ktx_image_s *R_KTXGetImage( const struct ktx_context_s *cntx, uint32_t mipLevel, uint32_t faceIndex, uint32_t arrayOffset )
{
	assert( cntx );
	assert( cntx->images);
	const size_t numberOfArrayElements = max( 1, cntx->numberOfArrayElements );
	const size_t numberOfFaces = max( 1, cntx->numberOfFaces );
	const size_t index = ( mipLevel * numberOfArrayElements * numberOfFaces ) + ( faceIndex * numberOfArrayElements ) + arrayOffset;
	assert( index < arrlen( cntx->images) );
	return &cntx->images[index];
}

enum ktx_context_result_e  R_InitKTXContext( uint8_t *memory, size_t size, struct ktx_context_s *cntx ) {
	assert( sizeof( struct __raw_ktx_header_s ) == 64 );
	struct __raw_ktx_header_s *rawHeader = (struct __raw_ktx_header_s *)memory;
	if( memcmp( rawHeader->identifier, "\xABKTX 11\xBB\r\n\x1A\n", 12 ) ) {
		return KTX_ERR_INVALID_IDENTIFIER;
	}
  assert(cntx->pixelDepth == 0);

	const bool swapEndian = ( rawHeader->endianness == 0x01020304 ) ? true : false;
	cntx->pixel = memory;
	cntx->swapEndianess = swapEndian;
	cntx->type = swapEndian ? LongSwap( rawHeader->type ) : rawHeader->type;
	cntx->typeSize = swapEndian ? LongSwap( rawHeader->typeSize ) : rawHeader->typeSize;
	cntx->format = swapEndian ? LongSwap( rawHeader->format ) : rawHeader->format;
	cntx->internalFormat = swapEndian ? LongSwap( rawHeader->internalFormat ) : rawHeader->internalFormat;
	cntx->baseInternalFormat = swapEndian ? LongSwap( rawHeader->baseInternalFormat ) : rawHeader->baseInternalFormat;
	cntx->pixelWidth = swapEndian ? LongSwap( rawHeader->pixelWidth ) : rawHeader->pixelWidth;
	cntx->pixelHeight = swapEndian ? LongSwap( rawHeader->pixelHeight ) : rawHeader->pixelHeight;
	cntx->pixelDepth = swapEndian ? LongSwap( rawHeader->pixelDepth ) : rawHeader->pixelDepth;
	cntx->numberOfArrayElements = swapEndian ? LongSwap( rawHeader->numberOfArrayElements ) : rawHeader->numberOfArrayElements;
	cntx->numberOfFaces = swapEndian ? LongSwap( rawHeader->numberOfFaces ) : rawHeader->numberOfFaces;
	cntx->numberOfMipmapLevels = swapEndian ? LongSwap( rawHeader->numberOfMipmapLevels ) : rawHeader->numberOfMipmapLevels;
	cntx->bytesOfKeyValueData = swapEndian ? LongSwap( rawHeader->bytesOfKeyValueData ) : rawHeader->bytesOfKeyValueData;

	const size_t numberOfArrayElements = max( 1, cntx->numberOfArrayElements );
	const size_t numberOfFaces = max( 1, cntx->numberOfFaces );
	const size_t numberOfMips = max( 1, cntx->numberOfMipmapLevels );

	const size_t keyValueOffset = sizeof(struct __raw_ktx_header_s);
	const size_t dataOffset = sizeof(struct __raw_ktx_header_s) + cntx->bytesOfKeyValueData;
	arrsetlen( cntx->images, numberOfArrayElements * numberOfFaces * numberOfMips );
 
  enum texture_format_e format = R_FORMAT_UNKNOWN;
	if( cntx->type == 0 ) {
		switch( cntx->internalFormat ) {
			case GL_ETC1_RGB8_OES:
				format = R_FORMAT_ETC1_R8G8B8_OES;
				break;
			case GL_COMPRESSED_RGB8_ETC2:
				format = R_FORMAT_ETC2_R8G8B8_UNORM;
				break;
			default:
				// this is not handled
//				ri.Com_Error( ERR_FATAL, "unhandled internalFormat: %d", cntx->internalFormat );
				assert(false);
				return KTX_ERR_UNHANDLED_TYPE;
		}
	} else {
		switch( cntx->type ) {
			case GL_UNSIGNED_SHORT_4_4_4_4:
				format = R_FORMAT_R4_G4_B4_A4_UNORM;
				break;
			case GL_UNSIGNED_SHORT_5_6_5:
				format = R_FORMAT_R5_G6_B5_UNORM;
				break;
			case GL_UNSIGNED_SHORT_5_5_5_1:
				format = R_FORMAT_R5_G5_B5_A1_UNORM;
				break;
			case GL_UNSIGNED_BYTE: {
				switch( cntx->baseInternalFormat ) {
					case GL_RGBA:
						format = R_FORMAT_RGBA8_UNORM;
						break;
					case GL_BGRA:
						format = R_FORMAT_BGRA8_UNORM;
						break;
					case GL_RGB:
						format = R_FORMAT_RGB8_UNORM;
						break;
					case GL_BGR:
						format = R_FORMAT_BGR8_UNORM;
						break;
					case GL_LUMINANCE_ALPHA:
						format = R_FORMAT_L8_A8_UNORM;
						break;
					case GL_LUMINANCE:
						format = R_FORMAT_L8_UNORM;
						break;
					case GL_ALPHA:
						format = R_FORMAT_A8_UNORM;
						break;
					default:
				    assert(false);
						return KTX_ERR_UNHANDLED_TYPE;
				}
				break;
			}
			default:
				assert(false);
				return KTX_ERR_UNHANDLED_TYPE;
		}
	}


	// mip and faces are aligned 4
	// KTX File Format Spec: https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html
	uint32_t width = cntx->pixelWidth;
	uint32_t height = cntx->pixelHeight;

	size_t bufferOffset = dataOffset;
	for( size_t mipLevel = 0; mipLevel < numberOfMips; mipLevel++ ) {
		//const uint32_t imageBufLen = swapEndian ? LongSwap( *( (uint32_t *)( memory + bufferOffset ) ) ) : *( (uint32_t *)( memory + bufferOffset ) );
		bufferOffset += sizeof( uint32_t );
		for( size_t arrayIdx = 0; arrayIdx < numberOfArrayElements; arrayIdx++ ) {
			for( size_t faceIdx = 0; faceIdx < numberOfFaces; faceIdx++ ) {
				struct ktx_image_s *img = R_KTXGetImage( cntx, mipLevel, faceIdx, arrayIdx );
				img->layout.width = width;
				img->layout.height = height;
				img->layout.format = format;
				switch( format ) {
					case R_FORMAT_ETC1_R8G8B8_OES:
					case R_FORMAT_ETC2_R8G8B8_UNORM: {
						const uint32_t blockWidth = R_FormatBlockWidth(format);
						const uint32_t blockHeight = R_FormatBlockHeight(format);
						const uint32_t blockSizeBytes = R_FormatBitSizePerBlock(format) / 8;
						img->layout.logicalWidth = ( width / blockWidth ) + ( width % blockWidth == 0 ? 0 : 1 );
						img->layout.logicalHeight = ( height / blockHeight ) + ( height % blockHeight == 0 ? 0 : 1 );

						img->layout.rowPitch =  ALIGN( img->layout.logicalWidth * blockSizeBytes, 4); // not sure I need alignment 
						img->layout.size = img->layout.rowPitch * img->layout.logicalHeight;  
						break;
					}
					case R_FORMAT_R5_G5_B5_A1_UNORM:
					case R_FORMAT_R5_G6_B5_UNORM:
					case R_FORMAT_R4_G4_B4_A4_UNORM:
						
						img->layout.logicalWidth = width;
						img->layout.logicalHeight = height;

						img->layout.rowPitch = ALIGN(img->layout.logicalWidth * sizeof( short ), 4);
						img->layout.size = img->layout.rowPitch * img->layout.logicalHeight;  
						break;
					case R_FORMAT_RGBA8_UNORM:
						img->layout.logicalWidth = width;
						img->layout.logicalHeight = height;

						img->layout.rowPitch = ALIGN(img->layout.logicalWidth * 4, 4);
						img->layout.size = img->layout.rowPitch * img->layout.logicalHeight;  
						break;
					case R_FORMAT_RG8_UNORM:
					case R_FORMAT_L8_A8_UNORM:
						img->layout.logicalWidth = width;
						img->layout.logicalHeight = height;
						
						img->layout.rowPitch = ALIGN(img->layout.logicalWidth * 2, 4);
						img->layout.size = img->layout.rowPitch * img->layout.logicalHeight;  
						break;
					case R_FORMAT_RGB8_UNORM:
					case R_FORMAT_BGR8_UNORM:
						img->layout.logicalWidth = width;
						img->layout.logicalHeight = height;
						
						img->layout.rowPitch =  ALIGN(img->layout.logicalWidth * 3, 4);
						img->layout.size = img->layout.rowPitch * img->layout.logicalHeight;  
						break;
					case R_FORMAT_R8_UNORM:
					case R_FORMAT_L8_UNORM:
					case R_FORMAT_A8_UNORM:
						img->layout.logicalWidth = width;
						img->layout.logicalHeight = height;
						
						img->layout.rowPitch = ALIGN(img->layout.logicalWidth * 1, 4);
						img->layout.size = img->layout.rowPitch * img->layout.logicalHeight;  
						break;
					default:
						assert( false );
						break;
				}
				img->layout.rowAlignment = 4;
				img->offset = bufferOffset;

				bufferOffset += img->layout.size;
				if( numberOfFaces >= 1 ) {
					bufferOffset  = ALIGN( bufferOffset, 4 );
				}
			}
		}
	 // if( chunkOffset > imageBufLen ) {
	 // 	ri.Com_Printf( S_COLOR_YELLOW "iamge buffer size greater %d expected: %d\n", chunkOffset, imageBufLen );
	 // }
		bufferOffset = ALIGN( bufferOffset, 4 );
		width = max( 1, width >> 1 );
		height = max( 1, height >> 1 );
	}

  if(bufferOffset > size ) {
    return KTX_ERR_TRUNCATED; 
  	//ri.Com_Printf( S_COLOR_YELLOW "file size does not match size: %d expected: %d\n", bufferOffset , size );
  }
  return KTX_ERR_NONE;
}
uint16_t R_KTXGetNumberMips( const struct ktx_context_s *cntx )
{
	return max( 1, cntx->numberOfMipmapLevels );
}
uint16_t R_KTXGetNumberFaces( const struct ktx_context_s *cntx )
{
	return max( 1, cntx->numberOfFaces );
}
uint16_t R_KTXGetNumberArrayElements( const struct ktx_context_s *cntx )
{
	return max( 1, cntx->numberOfArrayElements );
}
void R_KTXFillBuffer( const struct ktx_context_s *cntx, struct image_buffer_s *image, uint32_t faceIndex, uint32_t arrOffset, uint16_t mipLevel ) {

	assert( image );

	struct ktx_image_s *img = R_KTXGetImage( cntx, mipLevel, faceIndex, arrOffset );
	assert(img);
	
	R_ConfigureImageBuffer( image, &img->layout);
	memcpy( image->data, (cntx->pixel + img->offset), img->layout.size);
	if( !R_FormatIsCompressed( img->layout.format) && cntx->swapEndianess ) {
		R_Buf_SwapEndianess( image );
	}
}

