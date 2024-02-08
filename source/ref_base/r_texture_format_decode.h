#ifndef R_TEXTURE_FORMAT_DECODE_H
#define R_TEXTURE_FORMAT_DECODE_H

#include "../gameshared/q_arch.h"
#include "r_texture_format.h"
#include "r_image_buf.h"

static const struct image_buffer_layout_s RGB_ETC1_BLOCK_DECODE = {
  .format = R_FORMAT_RGB8_UNORM,
  .width = 4,
  .height = 4,

  .logicalWidth = 4,
  .logicalHeight = 4,
  .rowPitch = (3 * 4),
  .rowAlignment = 1
};

// encoding decoding
void R_DecodeLogicalBlockF(uint8_t* block, enum texture_format_e fmt, float* out);
void R_EncodeLogicalBlockF(uint8_t* block, enum texture_format_e fmt, float* out);

/**
* decodes a 4x4 block into 4x4x3 R_FORMAT_RGB8_UNORM
*
* the block is in row column order
**/
void R_ETC1DecodeBlock_RGB8(uint8_t* block, uint8_t* colors);
#endif
