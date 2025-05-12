#ifndef RI_TEXTURE_BUF_H
#define RI_TEXTURE_BUF_H

#include "qarch.h"
#include "ri_format.h"
#include "qhash.h"
struct RIFormatProps_s;

enum ri_texture_buf_flags_e {
	TEX_BUF_IS_ALIASED = 0x1,
	TEX_BUF_MAX = 0xffff
};

enum ri_texture_buf_res_e {
	TEXTURE_BUF_SUCCESS,
	TEXTURE_BUF_INVALID_CAP = -1 // the capacity of the buffer is invalid 
};
struct ri_texture_buf_s;
typedef void (*ri_free_hander_t)(void* p);
																					
struct ri_texture_buf_s {
  enum ri_texture_buf_flags_e flags;
  const struct RIFormatProps_s* def;

  void* freeParam;
  ri_free_hander_t freeHandler;

  uint32_t rowPitch; // the number of bytes in a row of pixels including any alignment
  uint16_t rowAlign;
  
  uint16_t width;
  uint16_t height;

  size_t capacity;
  size_t size;
  uint8_t *buffer;
};

struct ri_texture_buf_desc_s {
	uint32_t width;
	uint32_t height;
	uint16_t alignment;
	const struct RIFormatProps_s *def;
};
size_t ri_texture_buf_size( struct ri_texture_buf_s *buf );
uint16_t ri_texture_buf_logical_h( const struct ri_texture_buf_s *buf );
uint16_t ri_texture_buf_logical_w( const struct ri_texture_buf_s *buf );
uint16_t ri_texture_buf_w( const struct ri_texture_buf_s *buf);
uint16_t ri_texture_buf_h( const struct ri_texture_buf_s *buf);

/**
* If a buffer is aliased then it will allocate and copy the alised memory ovecr
**/
void ri_promote_texture_buf( struct ri_texture_buf_s *buf );
void ri_realloc_texture_buf( struct ri_texture_buf_s *buf, const struct ri_texture_buf_desc_s *desc );
int ri_alias_texture_buf( struct ri_texture_buf_s *buf, const struct ri_texture_buf_desc_s *desc, uint8_t *buffer, size_t size );

/**
 * texture buffer can be created with a custom de-allocator
 **/
int ri_alias_texture_buf_free( struct ri_texture_buf_s *buf, const struct ri_texture_buf_desc_s *desc, uint8_t *buffer, size_t size, void* param, ri_free_hander_t freeFn);

void ri_free_texture_buf(struct ri_texture_buf_s * tex);

struct ri_texture_buf_pogo_s {
	uint8_t index;
	struct ri_texture_buf_s textures[2];
};

struct ri_texture_buf_s* ri_current_pogo_bug(struct ri_texture_buf_pogo_s* pogo);
struct ri_texture_buf_s* ri_get_next_pogo_buf(struct ri_texture_buf_pogo_s* pogo);
void ri_incr_pogo_buf(struct ri_texture_buf_pogo_s* pogo);
void ri_free_pogo_buf(struct ri_texture_buf_pogo_s* pogo);

//void ri_block_decode_etc1( const struct ri_texture_buf_s *src, struct ri_texture_buf_s *dest );
void ri_texture_buf_quarter_in_place( struct ri_texture_buf_s *tex);
void ri_texture_buf_swap_endianness( struct ri_texture_buf_s *tex);
/**
*
* swizzle channels in place make sure the number of channels in the definition matches the 
* number of channels submitted
**/
void ri_texture_buf_swizzle_in_place(struct ri_texture_buf_s* tex, enum RI_LogicalChannel_e* channels);

#endif

