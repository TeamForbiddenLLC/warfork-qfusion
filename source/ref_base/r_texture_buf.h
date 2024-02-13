#ifndef R_TEXTURE_BUF_H
#define R_TEXTURE_BUF_H

#include "r_texture_format.h"

enum texture_buf_flags_e {
	TEX_BUF_IS_ALIASED = 0x1,
	TEX_BUF_MAX = 0xffff
};

struct texture_buf_s {
	enum texture_buf_flags_e flags;
  const struct base_format_def_s* def;
	
	uint32_t rowPitch; // the number of bytes in a row of pixels including any alignment
  uint16_t rowAlign;
  
  uint16_t width;
  uint16_t height;

  size_t capacity;
  size_t size;
  uint8_t *buffer;
};

struct texture_buf_desc_s {
	uint32_t width;
	uint32_t height;
	uint16_t alignment;
	const struct base_format_def_s *def;
};
size_t T_Size( struct texture_buf_s *buf );
uint16_t T_LogicalH( const struct texture_buf_s *buf );
uint16_t T_LogicalW( const struct texture_buf_s *buf );
uint16_t T_PixelW( const struct texture_buf_s *buf);
uint16_t T_PixelH( const struct texture_buf_s *buf);

/**
* If a buffer is aliased then it will allocate and copy the alised memory ovecr
**/
void T_PromteTextureBuf( struct texture_buf_s *buf);
void T_ReallocTextureBuf( struct texture_buf_s *buf, const struct texture_buf_desc_s *desc);
void T_AliasTextureBuf( struct texture_buf_s *buf, const struct texture_buf_desc_s *desc, uint8_t *buffer, size_t size);

void T_FreeTextureBuf(struct texture_buf_s * tex);

struct texture_buf_pogo_s {
	uint8_t index;
	struct texture_buf_s textures[2];
};

struct texture_buf_s* T_CurrentPogoTex(struct texture_buf_pogo_s* pogo);
struct texture_buf_s* T_NextPogoTex(struct texture_buf_pogo_s* pogo);
void T_IncrPogoTex(struct texture_buf_pogo_s* pogo);
void T_FreePogoBuffer(struct texture_buf_pogo_s* pogo);

void T_BlockDecodeETC1( const struct texture_buf_s *src, struct texture_buf_s *dest );
void T_MipMapQuarterInPlace( struct texture_buf_s *tex);
void T_SwapEndianness( struct texture_buf_s *tex);
/**
*
* swizzle channels in place make sure the number of channels in the definition matches the 
* number of channels submitted
**/
void T_SwizzleInplace(struct texture_buf_s* tex, enum texture_logical_channel_e* channels);

#endif
