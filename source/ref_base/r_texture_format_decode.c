#include "r_texture_format_decode.h"
#include "../gameshared/q_math.h"
#include "r_image_buf.h"

void R_DecodeLogicalBlockF( uint8_t *block, enum texture_format_e fmt, float *out )
{
	switch( fmt ) {
		case R_FORMAT_L8_UNORM: // not sure where luminance sits
		case R_FORMAT_R8_UNORM:
			out[0] = ( (float)( *block ) ) / 255.0f;
			break;
		case R_FORMAT_L8_A8_UNORM:
			out[0] = (uint8_t)( block[0] * 255.0f );
			out[1] = (uint8_t)( block[1] * 255.0f );
			break;
		case R_FORMAT_RG8_UNORM:
			out[0] = (uint8_t)( block[0] * 255.0f );
			out[1] = (uint8_t)( block[1] * 255.0f );
			break;
		case R_FORMAT_RGB8_UNORM:
			out[0] = (uint8_t)( block[0] * 255.0f );
			out[1] = (uint8_t)( block[1] * 255.0f );
			out[2] = (uint8_t)( block[2] * 255.0f );
			break;
		case R_FORMAT_RGBA8_UNORM:
			out[0] = (uint8_t)( block[0] * 255.0f );
			out[1] = (uint8_t)( block[1] * 255.0f );
			out[2] = (uint8_t)( block[2] * 255.0f );
			out[3] = (uint8_t)( block[3] * 255.0f );
			break;
		case R_FORMAT_BGRA8_UNORM:
			out[0] = (uint8_t)( block[2] * 255.0f );
			out[1] = (uint8_t)( block[1] * 255.0f );
			out[2] = (uint8_t)( block[0] * 255.0f );
			out[3] = (uint8_t)( block[3] * 255.0f );
			break;
		case R_FORMAT_BGR8_UNORM:
			out[0] = (uint8_t)( block[3] * 255.0f );
			out[1] = (uint8_t)( block[2] * 255.0f );
			out[2] = (uint8_t)( block[1] * 255.0f );
			break;
		case R_FORMAT_ETC1_R8G8B8_OES:
		case R_FORMAT_ETC2_R8G8B8_UNORM:
			break;
		default:
			assert( 0 );
			break;
	}
}

void R_EncodeLogicalBlockF( uint8_t *block, enum texture_format_e fmt, float *in )
{
	switch( fmt ) {
		case R_FORMAT_L8_UNORM:
		case R_FORMAT_A8_UNORM:
			block[0] = (uint8_t)( in[0] * 255.0f );
			break;
		case R_FORMAT_L8_A8_UNORM:
			block[0] = (uint8_t)( in[0] * 255.0f );
			block[1] = (uint8_t)( in[1] * 255.0f );
			break;
		case R_FORMAT_RG8_UNORM:
			assert( 0 ); // I don't think this is used
			break;
		case R_FORMAT_RGB8_UNORM:
			block[0] = (uint8_t)( in[0] * 255.0f );
			block[1] = (uint8_t)( in[1] * 255.0f );
			block[2] = (uint8_t)( in[2] * 255.0f );
			break;
		case R_FORMAT_RGBA8_UNORM:
			block[0] = (uint8_t)( in[0] * 255.0f );
			block[1] = (uint8_t)( in[1] * 255.0f );
			block[2] = (uint8_t)( in[2] * 255.0f );
			block[3] = (uint8_t)( in[3] * 255.0f );
			break;
		case R_FORMAT_R8_UNORM:
			block[0] = (uint8_t)( in[0] * 255.0f );
			break;
		case R_FORMAT_BGRA8_UNORM:
			block[0] = (uint8_t)( in[2] * 255.0f );
			block[1] = (uint8_t)( in[1] * 255.0f );
			block[2] = (uint8_t)( in[0] * 255.0f );
			block[3] = (uint8_t)( in[3] * 255.0f );
			break;
		case R_FORMAT_BGR8_UNORM:
			block[0] = (uint8_t)( in[2] * 255.0f );
			block[1] = (uint8_t)( in[1] * 255.0f );
			block[2] = (uint8_t)( in[0] * 255.0f );
			break;
		case R_FORMAT_RGBA16_UNORM:
		case R_FORMAT_R16_UNORM:
		case R_FORMAT_RG16_UNORM:
			break;
		default:
			assert( 0 );
			break;
	}
}

void R_ETC1DecodeBlock_RGB8(uint8_t* block, uint8_t* colors) {

  // defined in table: https://registry.khronos.org/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
  // BCF -- Base Color Flag

  // no diff bit is set
  #define BCF_C1_R1_NO_DIFF(value) ((value >> 28) & 0xf) // 4 bits 
  #define BCF_C2_R2_NO_DIFF(value) ((value >> 24) & 0xf) // 4 bits
  #define BCF_C1_G1_NO_DIFF(value) ((value >> 20) & 0xf) // 4 bits
  #define BCF_C2_G2_NO_DIFF(value) ((value >> 16) & 0xf) // 4 bits
  #define BCF_C1_B1_NO_DIFF(value) ((value >> 12) & 0xf) // 4 bits 
  #define BCF_C2_B2_NO_DIFF(value) ((value >> 8) & 0xf)  // 4 bits

  // diff bit is set
  #define BCF_C1_R1_DIFF(value) ((value >> 27) & 0x1F) // 5 bits 
  #define BCF_C2_DR2_DIFF(value) ((value >> 24) & 0x7) // 3 bits
  #define BCF_C1_G1_DIFF(value) ((value >> 19) & 0x1F) // 5 bits
  #define BCF_C2_DG2_DIFF(value) ((value >> 16) & 0x7) // 3 bits
  #define BCF_C1_B1_DIFF(value) ((value >> 11) & 0x1F) // 5 bits 
  #define BCF_C2_DB2_DIFF(value) ((value >> 8) & 0x7) // 3 bits

  // these bits are always avaliable 
  #define BCF_DIFF_SET(value) ((value & 2) > 0)
  #define BCF_FLIP_SET(value) ((value & 1) > 0) 
  #define BCF_CW1(value) ((value >> 2) & 7) 
  #define BCF_CW2(value) ((value >> 5) & 7) 

  // sub pixel colors
  static const int ETC1_ModifierTable[] =
  {
	  2, 8, -2, -8,
	  5, 17, -5, -17,
	  9, 29, -9, -29,
	  13, 42, -13, -42,
	  18, 60, -18, -60,
	  24, 80, -24, -80,
	  33, 106, -33, -106,
	  47, 183, -47, -183
  };
  static const int ETC1_Lookup[] = { 0, 1, 2, 3, -4, -3, -2, -1 };

	const uint64_t baseColorsAndFlags = ( block[0] << 24 ) | ( block[1] << 16 ) | ( block[2] << 8 ) | block[3];
	const uint64_t pixels = ( block[4] << 24 ) | ( block[5] << 16 ) | ( block[6] << 8 ) | block[7];
	int r1, r2, g1, g2, b1, b2;
	if( BCF_DIFF_SET( baseColorsAndFlags ) ) {
    // we are constructing an 8 bit word
    // copy the top 3 bits to the bottom order bits
    // for the second half we use the etc1lookup to look up the intensity
    r1 = (BCF_C1_R1_DIFF(baseColorsAndFlags) << 3) | (BCF_C1_R1_DIFF(baseColorsAndFlags) >> 2);
    const int r2_ = (BCF_C1_R1_DIFF(baseColorsAndFlags) + ETC1_Lookup[BCF_C2_DR2_DIFF(baseColorsAndFlags)]) & 0x1F;
    r2 = (r2_ << 3) | (r2_ >> 2);

    g1 = (BCF_C1_G1_DIFF(baseColorsAndFlags) << 3) | (BCF_C1_G1_DIFF(baseColorsAndFlags) >> 2); 
    const int g2_ = (BCF_C1_G1_DIFF(baseColorsAndFlags) + ETC1_Lookup[BCF_C2_DG2_DIFF(baseColorsAndFlags)]) & 0x1F;
    g2 = (g2_ << 3) | (g2_ >> 2);
    
    b1 = (BCF_C1_B1_DIFF(baseColorsAndFlags) << 3) | (BCF_C1_B1_DIFF(baseColorsAndFlags) >> 2); 
    const int b2_ = (BCF_C1_B1_DIFF(baseColorsAndFlags) + ETC1_Lookup[BCF_C2_DB2_DIFF(baseColorsAndFlags)]) & 0x1F;
    b2 = (b2_ << 3) | (b2_ >> 2); 
	} else {
	  // indvidual mode
	  //by replicating the four higher orderbits in the four lower order bits
    r1 = (BCF_C1_R1_NO_DIFF(baseColorsAndFlags) << 4) | (BCF_C1_R1_NO_DIFF(baseColorsAndFlags));
    r2 = (BCF_C2_R2_NO_DIFF(baseColorsAndFlags) << 4) | (BCF_C2_R2_NO_DIFF(baseColorsAndFlags));
    g1 = (BCF_C1_G1_NO_DIFF(baseColorsAndFlags) << 4) | (BCF_C1_G1_NO_DIFF(baseColorsAndFlags));
    g2 = (BCF_C2_G2_NO_DIFF(baseColorsAndFlags) << 4) | (BCF_C2_G2_NO_DIFF(baseColorsAndFlags));
    b1 = (BCF_C1_B1_NO_DIFF(baseColorsAndFlags) << 4) | (BCF_C1_B1_NO_DIFF(baseColorsAndFlags));
    b2 = (BCF_C2_B2_NO_DIFF(baseColorsAndFlags) << 4) | (BCF_C2_B2_NO_DIFF(baseColorsAndFlags));
	}

	#define CHANNEL_COUNT 3 
	for( size_t index = 0; index < 8; index++ ) {
		
		const uint32_t x = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index >> 1 ) : ( index >> 2 ) ); 
		const uint32_t y = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index & 1 ) : ( index & 3 ) );
		const uint32_t k = y + ( x * 4 );
		const uint32_t delta = (ETC1_ModifierTable + (BCF_CW1(baseColorsAndFlags) << 2))[( ( pixels >> k ) & 1 ) | ( ( pixels >> ( k + 15 ) ) & 2 )];
		
		colors[( ( y * 4 * CHANNEL_COUNT ) ) + ( x * CHANNEL_COUNT + 0 )] = bound( 0, r1 + delta, 255 );
		colors[( ( y * 4 * CHANNEL_COUNT ) ) + ( x * CHANNEL_COUNT + 1 )] = bound( 0, g1 + delta, 255 );
		colors[( ( y * 4 * CHANNEL_COUNT ) ) + ( x * CHANNEL_COUNT + 2 )] = bound( 0, b1 + delta, 255 );
	}

	for( size_t index = 0; index < 8; index++ ) {
		const uint32_t x = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index >> 1 ) : ( index >> 2 ) + 2 );
		const uint32_t y = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index & 1 ) + 2 : ( index & 3 ) );
		const uint32_t k = y + ( x * 4 );
		const uint32_t delta = (ETC1_ModifierTable + (BCF_CW2(baseColorsAndFlags) << 2))[( ( pixels >> k ) & 1 ) | ( ( pixels >> ( k + 15 ) ) & 2 )];

		colors[( ( y * 4 * CHANNEL_COUNT ) ) + ( x * CHANNEL_COUNT + 0 )] = bound( 0, r2 + delta, 255 );
		colors[( ( y * 4 * CHANNEL_COUNT ) ) + ( x * CHANNEL_COUNT + 1 )] = bound( 0, g2 + delta, 255 );
		colors[( ( y * 4 * CHANNEL_COUNT ) ) + ( x * CHANNEL_COUNT + 2 )] = bound( 0, b2 + delta, 255 );
	}

#undef CHANNEL_COUNT
#undef BCF_C1_R1_NO_DIFF
#undef BCF_C2_R2_NO_DIFF
#undef BCF_C1_G1_NO_DIFF
#undef BCF_C2_G2_NO_DIFF
#undef BCF_C1_B1_NO_DIFF
#undef BCF_C2_B2_NO_DIFF

#undef BCF_C1_R1_DIFF
#undef BCF_C2_DR2_DIFF
#undef BCF_C1_G1_DIFF
#undef BCF_C2_DG2_DIFF
#undef BCF_C1_B1_DIFF
#undef BCF_C2_DB2_DIFF

#undef BCF_DIFF_SET
#undef BCF_FLIP_SET
#undef BCF_CW1
#undef BCF_CW2
}
