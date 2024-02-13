#include "r_texture_buf.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"

#include "r_texture_decode.h"
#include "r_texture_format.h"
#include "r_texture_buf.h"

uint16_t T_LogicalH(const  struct texture_buf_s *buf )
{
	switch( buf->def->base ) {
		case R_BASE_BLOCKED_COMPRESSED:
			return ( buf->height / buf->def->compressed.blockWidth ) + ( (buf->height  % buf->def->compressed.blockHeight) == 0 ? 0 : 1 );
		default:
			break;
	}
	return buf->height;
}
uint16_t T_LogicalW(const  struct texture_buf_s *buf )
{
	switch( buf->def->base ) {
		case R_BASE_BLOCKED_COMPRESSED:
			return ( buf->width / buf->def->compressed.blockWidth ) + ( ( buf->width % buf->def->compressed.blockWidth ) == 0 ? 0 : 1 );
		default:
			break;
	}
	return buf->height;
}
uint16_t T_PixelW(const  struct texture_buf_s *buf )
{
	return buf->width;
}
uint16_t T_PixelH(const  struct texture_buf_s *buf )
{
	return buf->height;
}

static void T_ConfigureTextureBuf( struct texture_buf_s *buf, const struct base_format_def_s *def, uint32_t width, uint32_t height, uint16_t align )
{
	buf->def = def;
	switch( def->base ) {
		case R_BASE_BLOCKED_COMPRESSED: {
			buf->width = width;
			buf->height = height;
			buf->rowAlign = align;
			buf->rowPitch = ALIGN( T_LogicalW( buf ) * def->compressed.blockByteSize, align );
			buf->size = ( buf->rowPitch * T_LogicalH( buf ) );
			buf->def = def;
			break;
		}
		case R_BASE_FORMAT_FIXED_8: {
			buf->rowAlign = align;
			buf->rowPitch = ALIGN( width * def->fixed_8.numChannels, align );
			buf->size = ( buf->rowPitch * height );
			buf->width = width;
			buf->height = height;
			buf->def = def;
			break;
		}
		case R_BASE_FORMAT_FIXED_16: {
			buf->rowAlign = align;
			buf->rowPitch = ALIGN( width * def->fixed_16.numChannels * sizeof( uint16_t ), align );
			buf->size = ( buf->rowPitch * height );
			buf->width = width;
			buf->height = height;
			buf->def = def;
			break;
		}
		case R_BASE_FORMAT_FIXED_32: {
			buf->rowAlign = align;
			buf->rowPitch = ALIGN( width * def->fixed_32.numChannels * sizeof( uint32_t ), align );
			buf->size = ( buf->rowPitch * height );
			buf->width = width;
			buf->height = height;
			buf->def = def;
			break;
		}
		case R_BASE_FORMAT_PACKED_16: {
			buf->rowAlign = align;
			buf->rowPitch = ALIGN( width * sizeof( uint16_t ), align );
			buf->size = ( buf->rowPitch * height );
			buf->width = width;
			buf->height = height;
			buf->def = def;
			break;
		}
		default:
			assert( 0 );
			break;
	}
}

size_t T_Size( struct texture_buf_s *buf )
{
	return buf->size;
}

void T_PromteTextureBuf( struct texture_buf_s *tex )
{
	if( ( tex->flags & TEX_BUF_IS_ALIASED ) > 0 ) {
		uint8_t *aliasBuffer = tex->buffer;
		tex->buffer = malloc( tex->size );
		memcpy( tex->buffer, aliasBuffer, tex->size );
	}
}

void T_ReallocTextureBuf( struct texture_buf_s *buf, const struct texture_buf_desc_s *desc)
{
  // we stash the previous size and the current buffer to transfer
  T_ConfigureTextureBuf( buf, desc->def, desc->width, desc->height, desc->alignment );
  if( ( buf->flags & TEX_BUF_IS_ALIASED ) > 0 ) {
	  buf->buffer = NULL; // if the buffer is aliased we don't own the memory
	  buf->capacity = buf->size;
  } else {
	  while( buf->size > buf->capacity ) {
		  buf->capacity = ( buf->capacity == 0 ) ? buf->size : ( buf->capacity >> 1 ) + buf->capacity;
	  }
  }
  buf->flags = (buf->flags & ~TEX_BUF_IS_ALIASED);
	buf->buffer = realloc( buf->buffer, buf->capacity );
}

void T_AliasTextureBuf( struct texture_buf_s *buf, const struct texture_buf_desc_s *desc, uint8_t *buffer, size_t size)
{
  T_ConfigureTextureBuf( buf, desc->def, desc->width, desc->height, desc->alignment );
	if( ( buf->flags & TEX_BUF_IS_ALIASED ) == 0 ) {
		free( buf->buffer ); // we free the underlying memory
	}
	buf->flags |= TEX_BUF_IS_ALIASED;
	if(size == 0 ){
		buf->capacity = buf->size;
	} else {
		buf->capacity = size;
		assert( buf->capacity > size );
	}
	buf->buffer = buffer;
}



void T_SwapEndianness( struct texture_buf_s *tex) {
	const uint32_t width = T_LogicalW( tex );
	const uint32_t height = T_LogicalH( tex );
	switch( tex->def->base ) {
		case R_BASE_FORMAT_FIXED_32: {
		  for( size_t row = 0; row < height; row++ ) {
			  for( size_t column = 0; column < width; column++ ) {
				  for( size_t c = 0; c < tex->def->fixed_32.numChannels; c++ ) {
					  uint32_t *const channel = ( (uint32_t *)&tex->buffer[( tex->rowPitch * row ) + ( column * RT_BlockSize( tex->def ) )] ) + c;
					  ( *channel ) = LongSwap( *channel );
				  }
			  }
			}
			break;
		}
		case R_BASE_FORMAT_FIXED_16: {
		  for( size_t row = 0; row < height; row++ ) {
			  for( size_t column = 0; column < width; column++ ) {
				  for( size_t c = 0; c < tex->def->fixed_16.numChannels; c++ ) {
					  uint16_t *const channel = ( (uint16_t *)&tex->buffer[( tex->rowPitch * row ) + ( column * RT_BlockSize( tex->def ) )] ) + c;
					  ( *channel ) = ShortSwap( *channel );
				  }
				}
			}
			break;
		}
		default:
			break;
	}
}

void T_MipMapQuarterInPlace( struct texture_buf_s *tex) {
	const uint32_t halfWidth = ( T_LogicalW(tex) >> 1 );
	const uint32_t halfHeight = ( T_LogicalH(tex) >> 1 );
  const struct base_format_def_s* def = tex->def;
  switch( tex->def->base ) {
	  case R_BASE_FORMAT_FIXED_8: {
      assert(def->fixed_8.normalized);
		  for( size_t row = 0; row < halfHeight; row++ ) {
			  for( size_t column = 0; column < halfWidth; column++ ) {
				  const uint8_t *b1 = &tex->buffer[tex->rowPitch * ( row * 2 + 0 ) + ( ( column * 2 + 0 ) * def->fixed_8.numChannels )];
				  const uint8_t *b2 = &tex->buffer[tex->rowPitch * ( row * 2 + 1 ) + ( ( column * 2 + 0 ) * def->fixed_8.numChannels )];
				  const uint8_t *b3 = &tex->buffer[tex->rowPitch * ( row * 2 + 0 ) + ( ( column * 2 + 1 ) * def->fixed_8.numChannels )];
				  const uint8_t *b4 = &tex->buffer[tex->rowPitch * ( row * 2 + 1 ) + ( ( column * 2 + 1 ) * def->fixed_8.numChannels )];
				  for( size_t c = 0; c < def->fixed_8.numChannels; c++ ) {
					  tex->buffer[( tex->rowPitch * row ) + column * def->fixed_8.numChannels] = ( b1[c] + b2[c] + b3[c] + b4[c] ) >> 2;
				  }
			  }
		  }
		  break;
	  }
	  default:
		  assert( 0 );
		  break;
  }
}

void T_FreeTextureBuf( struct texture_buf_s *tex )
{
	assert(tex);
	if( ( tex->flags & TEX_BUF_IS_ALIASED ) == 0 ) {
		free( tex->buffer );
	}
}

struct texture_buf_s *T_CurrentPogoTex( struct texture_buf_pogo_s *pogo )
{
	assert(pogo);
	return &pogo->textures[pogo->index];
}

struct texture_buf_s *T_NextPogoTex( struct texture_buf_pogo_s *pogo )
{
	assert(pogo);
	return &pogo->textures[( pogo->index + 1 ) % 2];
}

void T_IncrPogoTex(struct texture_buf_pogo_s* pogo) {
	assert(pogo);
	pogo->index = (pogo->index + 1) % 2;
}

void T_FreePogoBuffer( struct texture_buf_pogo_s *pogo )
{
	assert(pogo);
	T_FreeTextureBuf( &pogo->textures[0] );
	T_FreeTextureBuf( &pogo->textures[1] );
}

void T_BlockDecodeETC1( const struct texture_buf_s *src, struct texture_buf_s *dest )
{
	assert( dest );
	assert( src );
	assert( src->def );
	assert( dest->def->base == R_BASE_FORMAT_FIXED_8 ); 

	assert( T_PixelW( src ) == T_PixelW( dest ) );
	assert( T_PixelH( src ) == T_PixelH( dest ) );

	const uint32_t logicalWidth = T_LogicalW( src );
	const uint32_t logicalHeight = T_LogicalH( src );

	const uint32_t width = T_PixelW(src);
	const uint32_t height = T_PixelH(src);

	struct uint_8_4 decodeColors[ETC1_BLOCK_WIDTH * ETC1_BLOCK_HEIGHT];
	for( size_t row = 0; row < logicalHeight; row++ ) {
		for( size_t column = 0; column < logicalWidth; column++ ) {
			uint8_t *block = &src->buffer[(src->rowPitch * row) + ( column * RT_BlockSize(src->def))];
			R_ETC1DecodeBlock_RGBA8( block, decodeColors );
			for( uint16_t subRow = 0; subRow < ETC1_BLOCK_HEIGHT; subRow++ ) {
				for( uint16_t subCol = 0; subCol < ETC1_BLOCK_WIDTH; subCol++ ) {
					const uint32_t destX = ( ( column * ETC1_BLOCK_WIDTH ) + subCol );
					const uint32_t destY = ( ( row * ETC1_BLOCK_HEIGHT ) + subRow );
					if(destX >= width || destY >= height)
						continue;
					const struct uint_8_4* color = &decodeColors[( subRow * ETC1_BLOCK_HEIGHT ) + subCol];
					uint8_t *const destBlock = &dest->buffer[( dest->rowPitch * destY ) + ( destX * RT_BlockSize( dest->def ) )];
					for( uint8_t c = 0; c < dest->def->fixed_8.numChannels; c++ ) {
						switch( dest->def->fixed_8.channels[c] ) {
							case R_LOGICAL_C_RED:
								( *(destBlock + c) ) = color->r;
								break;
							case R_LOGICAL_C_GREEN:
								( *(destBlock + c)) = color->g;
								break;
							case R_LOGICAL_C_BLUE:
								( *(destBlock + c) ) = color->b;
								break;
							default:
								( *(destBlock + c)) = 0;
								break;
						}
					}
				}
			}
		}
	}
}

void T_SwizzleInplace(struct texture_buf_s* tex, enum texture_logical_channel_e* channels) {
	// can't swizzle compressed formats
	const uint32_t logicalWidth = T_LogicalW( tex);
	const uint32_t logicalHeight = T_LogicalH( tex);
	
	switch(tex->def->base) {
		case R_BASE_FORMAT_FIXED_8: {
		  uint8_t values[R_LOGICAL_C_MAX];	
			for( size_t row = 0; row < logicalHeight; row++ ) {
				for( size_t column = 0; column < logicalWidth; column++ ) {
					uint8_t * const block = &tex->buffer[(tex->rowPitch * row) + ( column * RT_BlockSize(tex->def))];
					// save the values
					for( size_t c = 0; c < tex->def->fixed_8.numChannels; c++ ) {
						assert(tex->def->fixed_8.channels[c] < R_LOGICAL_C_MAX);
						values[tex->def->fixed_8.channels[c]] = *(block + c);
					}
					// write the new values to the block
					for(size_t c  = 0; c < tex->def->fixed_8.numChannels; c++) {
						assert(channels[c] < R_LOGICAL_C_MAX);
						(*(block + c)) = values[channels[c]];
					}
				}
			}
			break;
		}
		case R_BASE_FORMAT_FIXED_16: {
		  uint16_t values[R_LOGICAL_C_MAX];	
			for( size_t row = 0; row < logicalHeight; row++ ) {
				for( size_t column = 0; column < logicalWidth; column++ ) {
					uint16_t * const block = (uint16_t*)(&tex->buffer[(tex->rowPitch * row) + ( column * RT_BlockSize(tex->def))]);
					// save the values 
					for(size_t  c  = 0; c < tex->def->fixed_8.numChannels; c++) {
						assert(tex->def->fixed_16.channels[c] < R_LOGICAL_C_MAX);
						values[tex->def->fixed_16.channels[c]] = *(block + c); 
					}
					// write the new values to the block
					for(size_t c  = 0; c < tex->def->fixed_8.numChannels; c++) {
						assert(channels[c] < R_LOGICAL_C_MAX);
						(*(block + c)) = values[channels[c]];
					}
				}
			}
			break;
		}
		default:
			assert( 0 );
			break;
	}


}
