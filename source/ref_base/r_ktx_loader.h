#ifndef R_KTX_LOADER_H
#define R_KTX_LOADER_H

#include "r_texture_buf.h"
#include "r_texture_format.h"

struct ktx_image_s {
  bool updated;
  struct texture_buf_s texture;
};

enum ktx_context_result_type_e {
  KTX_ERR_INVALID_IDENTIFIER,
  KTX_ERR_UNHANDLED_TEXTURE_TYPE,
  KTX_ERR_TRUNCATED,
  KTX_ERR_ZER_TEXTURE_SIZE 
};

struct ktx_context_err_s {
	enum ktx_context_result_type_e type;
	union {
		struct {
			uint32_t type;
			uint32_t internalFormat;
		} errTextureType;
		struct {
			size_t size;
			size_t expected;
		} errTruncated;
	};
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

	uint8_t *buffer;
	const struct base_format_def_s *desc;
	struct ktx_image_s *textures;
};

bool R_InitKTXContext(struct ktx_context_s *cntx, uint8_t *memory, size_t size, struct ktx_context_err_s* err);
uint16_t R_KTXGetNumberMips( const struct ktx_context_s *cntx );
uint16_t R_KTXGetNumberFaces( const struct ktx_context_s *cntx );
uint16_t R_KTXGetNumberArrayElements( const struct ktx_context_s *cntx );
struct texture_buf_s *R_KTXResolveBuffer( struct ktx_context_s *cntx, uint32_t mipLevel, uint32_t faceIndex, uint32_t arrayOffset );
void R_KTXFreeContext(struct ktx_context_s* cntx);
bool R_KTXIsCompressed(struct ktx_context_s* cntx);

uint16_t R_KTXWidth(const struct ktx_context_s* cntx);
uint16_t R_KTXHeight(const struct ktx_context_s* cntx);


#endif
