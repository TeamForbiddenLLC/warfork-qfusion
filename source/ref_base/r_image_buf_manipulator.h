#ifndef R_IMAGE_MANIPULATOR_H
#define R_IMAGE_MANIPULATOR_H

#include "r_image_buf.h"

bool R_SwapChannelInPlace( struct image_buffer_s *target, uint16_t c1, uint16_t c2 );
void R_ResizeImage( const struct image_buffer_s *src, uint16_t scaledWidth, uint16_t scaledHeight, struct image_buffer_s *dest );
bool R_FlipTexture( struct image_buffer_s *src, struct image_buffer_s *dest, bool flipx, bool flipy, bool flipdiagonal );

// quarter the buffer in place
bool R_MipMapQuarterInPlace(struct image_buffer_s *src);
#endif
