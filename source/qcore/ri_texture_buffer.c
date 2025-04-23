#include "ri_texture_buffer.h"
#include "ri_format.h"
#include "../gameshared/q_shared.h"

uint16_t ri_texture_buf_logical_h(const struct ri_texture_buf_s *buf) {
  return (buf->height / buf->def->blockHeight) +
         ((buf->height % buf->def->blockHeight) == 0 ? 0 : 1);
}
uint16_t ri_texture_buf_logical_w(const struct ri_texture_buf_s *buf) {
  return (buf->width / buf->def->blockWidth) +
         ((buf->width % buf->def->blockWidth) == 0 ? 0 : 1);
}
uint16_t ri_texture_buf_w(const struct ri_texture_buf_s *buf) { return buf->width; }
uint16_t ri_texture_buf_h(const struct ri_texture_buf_s *buf) { return buf->height; }

size_t ri_texture_buf_size(struct ri_texture_buf_s *buf) { return buf->size; }

static void T_ConfigureTextureBuf(struct ri_texture_buf_s *buf,
                                  const struct RIFormatProps_s *def,
                                  uint32_t width, uint32_t height,
                                  uint16_t align) {
    buf->width = width;
    buf->height = height;
    buf->rowAlign = align;
    buf->rowPitch = Q_ALIGN_TO(ri_texture_buf_logical_w(buf) * def->stride, align);
    buf->size = (buf->rowPitch * ri_texture_buf_logical_h(buf));
    buf->def = def;
}

void ri_promote_texture_buf( struct ri_texture_buf_s *tex) {
  assert(tex);
  if ((tex->flags & TEX_BUF_IS_ALIASED) > 0) {
    uint8_t *aliasBuffer = tex->buffer;
    tex->buffer = malloc(tex->size);
    memcpy(tex->buffer, aliasBuffer, tex->size);

    // free the alias buffer
    if (tex->freeHandler) {
      tex->freeHandler(tex->freeParam);
    }
    tex->freeHandler = NULL;
  }
  tex->flags = (tex->flags & (~TEX_BUF_IS_ALIASED));
}

static void __T_ResetBuffer(struct ri_texture_buf_s *buf) {
  if ((buf->flags & TEX_BUF_IS_ALIASED) > 0) {
    if (buf->freeHandler) {
      buf->freeHandler(buf->freeParam);
    }
    buf->freeHandler = NULL;
    buf->flags = (buf->flags & ~TEX_BUF_IS_ALIASED); // we clear the alias flag
  } else if (buf->buffer) {
    free(buf->buffer);
  }
  buf->buffer = NULL;
}

void ri_realloc_texture_buf(struct ri_texture_buf_s *buf,
                            const struct ri_texture_buf_desc_s *desc) {
  // we stash the previous size and the current buffer to transfer
  T_ConfigureTextureBuf(buf, desc->def, desc->width, desc->height,
                        desc->alignment);
  if ((buf->flags & TEX_BUF_IS_ALIASED) > 0) {
    __T_ResetBuffer(buf);
    buf->capacity = buf->size;
  } else {
    while (buf->size > buf->capacity) {
      buf->capacity = (buf->capacity == 0)
                          ? buf->size
                          : (buf->capacity >> 1) + buf->capacity;
    }
  }
  buf->buffer = realloc(buf->buffer, buf->capacity);
}

int ri_alias_texture_buf(struct ri_texture_buf_s *buf,
                         const struct ri_texture_buf_desc_s *desc,
                         uint8_t *buffer, size_t size) {
  T_ConfigureTextureBuf(buf, desc->def, desc->width, desc->height,
                        desc->alignment);
  __T_ResetBuffer(buf);
  buf->flags |= TEX_BUF_IS_ALIASED;
  buf->buffer = buffer;
  if (size == 0) {
    buf->capacity = buf->size;
  } else {
    buf->capacity = size;
    if (buf->capacity < size) {
      return TEXTURE_BUF_INVALID_CAP;
    }
  }
  return TEXTURE_BUF_SUCCESS;
}

int ri_alias_texture_buf_free(struct ri_texture_buf_s *buf,
                              const struct ri_texture_buf_desc_s *desc,
                              uint8_t *buffer, size_t size, void *param,
                              ri_free_hander_t freeFn) {
  assert(freeFn);
  T_ConfigureTextureBuf(buf, desc->def, desc->width, desc->height,
                        desc->alignment);
  __T_ResetBuffer(buf);
  buf->flags |= TEX_BUF_IS_ALIASED;
  buf->buffer = buffer;
  if (size == 0) {
    buf->capacity = buf->size;
  } else {
    buf->capacity = size;
    if (buf->capacity < size)
      return TEXTURE_BUF_INVALID_CAP;
  }
  buf->freeHandler = freeFn;
  buf->freeParam = param;
  return TEXTURE_BUF_SUCCESS;
}

void ri_texture_buf_swap_endianness(struct ri_texture_buf_s *tex) {
  const uint32_t width = ri_texture_buf_logical_w(tex);
  const uint32_t height = ri_texture_buf_logical_h(tex);
  switch (tex->def->format) {
  case RI_FORMAT_RGBA32_SINT:
  case RI_FORMAT_RGB32_SFLOAT:
  case RI_FORMAT_RGBA32_UINT: {
    for (size_t row = 0; row < height; row++) {
      for (size_t column = 0; column < width; column++) {
        for (size_t c = 0; c < (tex->def->stride / 4); c++) {
          uint32_t *const channel =
              ((uint32_t *)&tex->buffer[(tex->rowPitch * row) +
                                        (column * tex->def->stride)]) +
              c;
          (*channel) = LongSwap(*channel);
        }
      }
    }
    break;
  }
  case RI_FORMAT_RG16_SFLOAT:
  case RI_FORMAT_RGBA16_SINT:
  case RI_FORMAT_RGBA16_SNORM:
  case RI_FORMAT_RGBA16_UINT: {
    for (size_t row = 0; row < height; row++) {
      for (size_t column = 0; column < width; column++) {
        for (size_t c = 0; c < (tex->def->stride / 4); c++) {
          uint32_t *const channel =
              ((uint32_t *)&tex->buffer[(tex->rowPitch * row) +
                                        (column * tex->def->stride)]) +
              c;
          (*channel) = ShortSwap(*channel);
        }
      }
    }
    break;
  }
  default:
    assert(0);
    break;
  }
}

void ri_texture_buf_quarter_in_place(struct ri_texture_buf_s *tex) {
  const uint32_t halfWidth = (ri_texture_buf_logical_w(tex) >> 1);
  const uint32_t halfHeight = (ri_texture_buf_logical_h(tex) >> 1);
  switch (tex->def->format) {
  case RI_FORMAT_RGBA8_UNORM:
  case RI_FORMAT_RGB8_UNORM: {
    for (size_t row = 0; row < halfHeight; row++) {
      for (size_t column = 0; column < halfWidth; column++) {
        const uint8_t *b1 = &tex->buffer[tex->rowPitch * (row * 2 + 0) +
                                         ((column * 2 + 0) * tex->def->stride)];
        const uint8_t *b2 = &tex->buffer[tex->rowPitch * (row * 2 + 1) +
                                         ((column * 2 + 0) * tex->def->stride)];
        const uint8_t *b3 = &tex->buffer[tex->rowPitch * (row * 2 + 0) +
                                         ((column * 2 + 1) * tex->def->stride)];
        const uint8_t *b4 = &tex->buffer[tex->rowPitch * (row * 2 + 1) +
                                         ((column * 2 + 1) * tex->def->stride)];
        for (size_t c = 0; c < tex->def->stride; c++) {
          tex->buffer[(tex->rowPitch * row) + column * tex->def->stride] =
              (b1[c] + b2[c] + b3[c] + b4[c]) >> 2;
        }
      }
    }
    break;
  }
  default:
    assert(0);
    break;
  }
}

void ri_free_texture_buf(struct ri_texture_buf_s *tex) {
  assert(tex);
  __T_ResetBuffer(tex);
  tex->flags = 0;
}

struct ri_texture_buf_s *
ri_current_pogo_bug(struct ri_texture_buf_pogo_s *pogo) {
  assert(pogo);
  return &pogo->textures[pogo->index];
}

struct ri_texture_buf_s* ri_get_next_pogo_buf(struct ri_texture_buf_pogo_s* pogo)
{
	assert(pogo);
	return &pogo->textures[( pogo->index + 1 ) % 2];
}

void ri_incr_pogo_buf(struct ri_texture_buf_pogo_s* pogo) {
	assert(pogo);
	pogo->index = (pogo->index + 1) % 2;
}

void ri_free_pogo_buf(struct ri_texture_buf_pogo_s* pogo)
{
	assert(pogo);
	ri_free_texture_buf( &pogo->textures[0] );
	ri_free_texture_buf( &pogo->textures[1] );
}

//void ri_block_decode_etc1( const struct ri_texture_buf_s *src, struct ri_texture_buf_s *dest )
//{
//	assert( dest );
//	assert( src );
//	assert( src->def );
//	assert( dest->def->base == R_BASE_FORMAT_FIXED_8 ); 
//
//	assert( T_PixelW( src ) == T_PixelW( dest ) );
//	assert( T_PixelH( src ) == T_PixelH( dest ) );
//
//	const uint32_t logicalWidth = T_LogicalW( src );
//	const uint32_t logicalHeight = T_LogicalH( src );
//
//	const uint32_t width = T_PixelW(src);
//	const uint32_t height = T_PixelH(src);
//
//	struct uint_8_4 decodeColors[ETC1_BLOCK_WIDTH * ETC1_BLOCK_HEIGHT];
//	for( size_t row = 0; row < logicalHeight; row++ ) {
//		for( size_t column = 0; column < logicalWidth; column++ ) {
//			uint8_t *block = &src->buffer[(src->rowPitch * row) + ( column * RT_BlockSize(src->def))];
//			R_ETC1DecodeBlock_RGBA8( block, decodeColors );
//			for( uint16_t subRow = 0; subRow < ETC1_BLOCK_HEIGHT; subRow++ ) {
//				for( uint16_t subCol = 0; subCol < ETC1_BLOCK_WIDTH; subCol++ ) {
//					const uint32_t destX = ( ( column * ETC1_BLOCK_WIDTH ) + subCol );
//					const uint32_t destY = ( ( row * ETC1_BLOCK_HEIGHT ) + subRow );
//					if(destX >= width || destY >= height)
//						continue;
//					const struct uint_8_4* color = &decodeColors[( subRow * ETC1_BLOCK_HEIGHT ) + subCol];
//					uint8_t *const destBlock = &dest->buffer[( dest->rowPitch * destY ) + ( destX * RT_BlockSize( dest->def ) )];
//					for( uint8_t c = 0; c < dest->def->fixed_8.numChannels; c++ ) {
//						switch( dest->def->fixed_8.channels[c] ) {
//							case R_LOGICAL_C_RED:
//								( *(destBlock + c) ) = color->r;
//								break;
//							case R_LOGICAL_C_GREEN:
//								( *(destBlock + c)) = color->g;
//								break;
//							case R_LOGICAL_C_BLUE:
//								( *(destBlock + c) ) = color->b;
//								break;
//							default:
//								( *(destBlock + c)) = 0;
//								break;
//						}
//					}
//				}
//			}
//		}
//	}
//}
void ri_texture_buf_swizzle_in_place(struct ri_texture_buf_s* tex, enum RI_LogicalChannel_e* channels) {
	// can't swizzle compressed formats
  const uint32_t logicalWidth = ri_texture_buf_logical_w(tex); 
  const uint32_t logicalHeight = ri_texture_buf_logical_h(tex);

  switch (tex->def->format) {
  case RI_FORMAT_RGBA8_UNORM:
  case RI_FORMAT_RGB8_UNORM: {
    uint8_t values[RI_LOGICAL_C_MAX];
    for (size_t row = 0; row < logicalHeight; row++) {
      for (size_t column = 0; column < logicalWidth; column++) {
        uint8_t *const block =
            &tex->buffer[(tex->rowPitch * row) + (column * tex->def->stride)];
        // save the values
        for (size_t c = 0; c < tex->def->stride; c++) {
          values[c] = block[c];
        }
        // write the new values to the block
        for (size_t c = 0; c < tex->def->stride; c++) {
          assert(channels[c] < RI_LOGICAL_C_MAX);
          block[c] = values[channels[c]];
        }
      }
    }
    break;
  }
  case RI_FORMAT_RG16_SFLOAT:
  case RI_FORMAT_RGBA16_SINT:
  case RI_FORMAT_RGBA16_SNORM:
  case RI_FORMAT_RGBA16_UINT: {
    for (size_t row = 0; row < logicalHeight; row++) {
      for (size_t column = 0; column < logicalWidth; column++) {
        uint16_t *const block = (uint16_t *)&tex->buffer[(tex->rowPitch * row) + (column * tex->def->stride)];
        uint16_t values[RI_LOGICAL_C_MAX];
        for (size_t c = 0; c < (tex->def->stride / sizeof(uint16_t)); c++) {
          values[c] = block[c];
        }
        // write the new values to the block
        for (size_t c = 0; c < (tex->def->stride / sizeof(uint16_t)); c++) {
          assert(channels[c] < RI_LOGICAL_C_MAX);
          block[c] = values[channels[c]];
        }
      }
    }
    break;
  }
  default:
    assert(0);
    break;
  }
}
