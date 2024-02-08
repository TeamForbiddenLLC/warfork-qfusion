#ifndef R_KTX_LOADER_H
#define R_KTX_LOADER_H

#include "../gameshared/q_arch.h"
#include "r_image_buf.h"
#include "r_texture_format.h"

struct ktx_image_s {
	struct image_buffer_layout_s layout;
	size_t offset; // offset into data
};

enum ktx_context_result_e {
  KTX_ERR_NONE = 0,
  KTX_ERR_INVALID_IDENTIFIER = -1,
  KTX_ERR_UNHANDLED_TYPE = -2,
  KTX_ERR_TRUNCATED = -3,
};

struct ktx_context_s {
	bool swapEndianess;
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

  uint8_t* pixel;
	struct ktx_image_s* images;
};

enum ktx_context_result_e R_InitKTXContext( uint8_t *memory, size_t size, struct ktx_context_s *cntx );
uint16_t R_KTXGetNumberMips( const struct ktx_context_s *cntx );
uint16_t R_KTXGetNumberFaces( const struct ktx_context_s *cntx );
uint16_t R_KTXGetNumberArrayElements( const struct ktx_context_s *cntx );
//bool R_KTXIsCompressedFormat( const struct ktx_context_s *cntx );
void R_KTXFillBuffer( const struct ktx_context_s *cntx, struct image_buffer_s *image, uint32_t faceIndex, uint32_t arrOffset, uint16_t mipLevel );
struct ktx_image_s* R_KTXGetImage(const struct ktx_context_s *cntx, uint32_t mipLevel, uint32_t faceIndex, uint32_t arrayOffset );

#endif
