#ifndef R_TEXTURE_FORMAT_DECODE_H
#define R_TEXTURE_FORMAT_DECODE_H

#include "../gameshared/q_arch.h"

#define ETC1_BLOCK_WIDTH 4
#define ETC1_BLOCK_HEIGHT 4

struct uint_8_4 {
  union {
    uint8_t v[4];
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t a;
    };
  };
};

/**
* decodes a 4x4 block into 4x4x3 R_FORMAT_RGB8_UNORM
*
* the block is in row column order
**/
void R_ETC1DecodeBlock_RGBA8(uint8_t* block, struct uint_8_4 colors[ETC1_BLOCK_WIDTH * ETC1_BLOCK_HEIGHT]);

#endif
