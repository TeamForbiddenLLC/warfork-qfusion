#include "r_image_buf.h"
#include "r_texture_format.h"
#include "stb_ds.h"
#include "../gameshared/q_math.h"



//void R_CalcImageBufferLayout(uint16_t width, uint16_t height, enum texture_format_e, struct image_buffer_layout_s* layout) {

void R_CalcImageBufferLayout( uint16_t width, uint16_t height, enum texture_format_e format, uint16_t rowAlignment, struct image_buffer_layout_s *layout )
{
	layout->width = width;
	layout->height = height;
	layout->rowAlignment = rowAlignment;
  layout->format = format;

	if( R_FormatIsCompressed( format ) ) {
		const uint32_t blockWidth = R_FormatBlockWidth( format );
		const uint32_t blockHeight = R_FormatBlockHeight( format );
		const uint32_t blockSizeBytes = R_FormatBitSizePerBlock( format ) / 8;
		layout->logicalWidth = ( width / blockWidth ) + ( width % blockWidth == 0 ? 0 : 1 );
		layout->logicalHeight = ( height / blockHeight ) + ( height % blockHeight == 0 ? 0 : 1 );
		layout->rowPitch = ALIGN( layout->logicalWidth * blockSizeBytes, rowAlignment ); // not sure I need alignment
		layout->size = layout->rowPitch * layout->logicalHeight;
	} else {
		const size_t bytesPerBlock = R_FormatBitSizePerBlock( format ) / 8;
		layout->logicalWidth = layout->width;
		layout->logicalHeight = layout->height;
		layout->rowPitch = ALIGN( bytesPerBlock * layout->logicalWidth, layout->rowAlignment );
		layout->size = layout->rowPitch * layout->logicalHeight;
	}
}
void R_ConfigureBufferSize( struct image_buffer_s *buffer, size_t size ) {
	arrsetlen(buffer->data, size);
}

void R_ConfigureImageBuffer(struct image_buffer_s *buffer, struct image_buffer_layout_s *layout )
{
  buffer->layout = (*layout);
  arrsetlen(buffer->data, layout->size);
}


struct image_buffer_s* R_NextPogoBuffer(struct image_buffer_pogo* pogo) {
	pogo->index = (pogo->index + 1) % 2;
	return &pogo->buffers[pogo->index]; 
}

void R_FreeImageBuffer( struct image_buffer_s *buffer )
{
	arrfree( buffer->data );
	buffer->data = NULL;
}
