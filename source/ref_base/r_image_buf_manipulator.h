#ifndef R_IMAGE_MANIPULATOR_H
#define R_IMAGE_MANIPULATOR_H

#include "r_image_buf.h"
#include "r_texture_format.h"

void R_Buf_SwapChannelInPlace( struct image_buffer_s *target, uint16_t c1, uint16_t c2 );
void R_Buf_ResizeImage( const struct image_buffer_s *src, uint16_t scaledWidth, uint16_t scaledHeight, struct image_buffer_s *dest );
void R_Buf_FlipTexture( struct image_buffer_s *src, struct image_buffer_s *dest, bool flipx, bool flipy, bool flipdiagonal );
void R_Buf_SwapEndianess( struct image_buffer_s *target );

void R_Buf_MipMapQuarterInPlaceU8(struct image_buffer_s *src);
void R_Buf_MipMapQuarterInPlace_Fallback( struct image_buffer_s *src );
void R_Buf_MipMapQuarterInPlace(struct image_buffer_s *src);

void R_Buf_UncompressImage_ETC1_RGB8(const struct image_buffer_s *src, struct image_buffer_s *dest, uint16_t rowAlignment);
#endif
