#ifndef R_IMAGE_BUF_H
#define R_IMAGE_BUF_H

#include "../gameshared/q_arch.h"
#include "r_texture_format.h"

// https://github.com/microsoft/DirectXTex/wiki/Image
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubresourceLayout.html
//

struct image_buffer_layout_s {
	enum texture_format_e format; // format of the data 

  size_t size;
  
  // logical blocks for compressed formats blocks can contain more then one pixel                
  uint16_t width;
  uint16_t height;

  // logical blocks for compressed formats blocks 
  //    else this is equal to the size of the images 
	uint16_t logicalWidth;  
	uint16_t logicalHeight;
	uint32_t rowPitch; // the number of bytes in a row of pixels including any alignment
  uint16_t rowAlignment;
};

enum image_buffer_flags_e {
	IMAGE_BUF_IS_ALIASED = 0x1,
	IMAGE_BUF_IS_BIG_ENDIAN = 0x2,
	IMAGE_BUF_MAX = 0xffff
};

struct image_buffer_s {
	enum image_buffer_flags_e flags;
	struct image_buffer_layout_s layout;
	uint8_t *data;
};

// pogo between two buffers
struct image_buffer_pogo_s {
	uint8_t index;
	struct image_buffer_s buffers[2];
};

// pogo buffer
void R_ImagePogoIncrement(struct image_buffer_pogo_s* pogo);
struct image_buffer_s* R_ImagePogoCurrent(struct image_buffer_pogo_s* pogo);
struct image_buffer_s* R_ImagePogoNext(struct image_buffer_pogo_s* pogo);

// image_buffer_s
void R_CalcImageBufferLayout(uint16_t pixWidth, uint16_t pixHeight, enum texture_format_e format, uint16_t rowAlignment, struct image_buffer_layout_s* layout); 
void R_ConfigureImageBuffer(struct image_buffer_s* buffer, struct image_buffer_layout_s* layout );
void R_ConfigureBufferSize(struct image_buffer_s* buffer,size_t size); // update the intternal length 
void R_FreeImageBuffer(struct image_buffer_s* buffer);


#endif
