#include "r_texture_decode.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_arch.h"

void R_ETC1DecodeBlock_RGBA8( uint8_t *block, struct uint_8_4 colors[ETC1_BLOCK_WIDTH * ETC1_BLOCK_HEIGHT] )
{
// implementation: https://registry.khronos.org/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
// BCF -- Base Color Flag

// no diff bit is set
//     63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
//   -----------------------------------------------
//  | base col1 | base col2 | base col1 | base col2 |
//  | R1 (4bits)| R2 (4bits)| G1 (4bits)| G2 (4bits)|
//   -----------------------------------------------

//   47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
//   ---------------------------------------------------
//  | base col1 | base col2 | table  | table  |diff|flip|
//  | B1 (4bits)| B2 (4bits)| cw 1   | cw 2   |bit |bit |
//   ---------------------------------------------------
#define BCF_C1_R1_NO_DIFF( value ) ( ( value >> 28 ) & 0xf ) // 4 bits
#define BCF_C2_R2_NO_DIFF( value ) ( ( value >> 24 ) & 0xf ) // 4 bits
#define BCF_C1_G1_NO_DIFF( value ) ( ( value >> 20 ) & 0xf ) // 4 bits
#define BCF_C2_G2_NO_DIFF( value ) ( ( value >> 16 ) & 0xf ) // 4 bits
#define BCF_C1_B1_NO_DIFF( value ) ( ( value >> 12 ) & 0xf ) // 4 bits
#define BCF_C2_B2_NO_DIFF( value ) ( ( value >> 8 ) & 0xf )	 // 4 bits

// diff bit is set
//  63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
//  -----------------------------------------------
// | base col1    | dcol 2 | base col1    | dcol 2 |
// | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    |
//  -----------------------------------------------
//
//  47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
//  ---------------------------------------------------
// | base col 1   | dcol 2 | table  | table  |diff|flip|
// | B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
//  ---------------------------------------------------
#define BCF_C1_R1_DIFF( value ) ( ( value >> 27 ) & 0x1F ) // 5 bits
#define BCF_C2_DR2_DIFF( value ) ( ( value >> 24 ) & 0x7 ) // 3 bits
#define BCF_C1_G1_DIFF( value ) ( ( value >> 19 ) & 0x1F ) // 5 bits
#define BCF_C2_DG2_DIFF( value ) ( ( value >> 16 ) & 0x7 ) // 3 bits
#define BCF_C1_B1_DIFF( value ) ( ( value >> 11 ) & 0x1F ) // 5 bits
#define BCF_C2_DB2_DIFF( value ) ( ( value >> 8 ) & 0x7 )  // 3 bits

// these bits are always avaliable
#define BCF_DIFF_SET( value ) ( ( value & 2 ) > 0 )
#define BCF_FLIP_SET( value ) ( ( value & 1 ) > 0 )
#define BCF_CW2( value ) ( ( value >> 2 ) & 7 )
#define BCF_CW1( value ) ( ( value >> 5 ) & 7 )

	// sub pixel colors
	static const int_fast16_t ETC1_ModifierTable[] = { 
		2, 8, -2, -8, 
		5, 17, -5, -17, 
		9, 29, -9, -29, 
		13, 42, -13, -42, 
		18, 60, -18, -60, 
		24, 80, -24, -80, 
		33, 106, -33, -106, 
		47, 183, -47, -183 };
	static const int_fast16_t ETC1_Lookup[] = { 0, 1, 2, 3, -4, -3, -2, -1 };

	const uint_fast64_t baseColorsAndFlags = ( block[0] << 24 ) | ( block[1] << 16 ) | ( block[2] << 8 ) | block[3];
	const uint_fast64_t pixels = ( block[4] << 24 ) | ( block[5] << 16 ) | ( block[6] << 8 ) | block[7];
	int_fast32_t r1, r2, g1, g2, b1, b2;
	if( BCF_DIFF_SET( baseColorsAndFlags ) ) {
		// we are constructing an 8 bit word
		// copy the top 3 bits to the bottom order bits
		// for the second half we use the etc1lookup to look up the intensity
		r1 = ( BCF_C1_R1_DIFF( baseColorsAndFlags ) << 3 ) | ( BCF_C1_R1_DIFF( baseColorsAndFlags ) >> 2 );
		const int r2_ = ( BCF_C1_R1_DIFF( baseColorsAndFlags ) + ETC1_Lookup[BCF_C2_DR2_DIFF( baseColorsAndFlags )] ) & 0x1F;
		r2 = ( r2_ << 3 ) | ( r2_ >> 2 );

		g1 = ( BCF_C1_G1_DIFF( baseColorsAndFlags ) << 3 ) | ( BCF_C1_G1_DIFF( baseColorsAndFlags ) >> 2 );
		const int g2_ = ( BCF_C1_G1_DIFF( baseColorsAndFlags ) + ETC1_Lookup[BCF_C2_DG2_DIFF( baseColorsAndFlags )] ) & 0x1F;
		g2 = ( g2_ << 3 ) | ( g2_ >> 2 );

		b1 = ( BCF_C1_B1_DIFF( baseColorsAndFlags ) << 3 ) | ( BCF_C1_B1_DIFF( baseColorsAndFlags ) >> 2 );
		const int b2_ = ( BCF_C1_B1_DIFF( baseColorsAndFlags ) + ETC1_Lookup[BCF_C2_DB2_DIFF( baseColorsAndFlags )] ) & 0x1F;
		b2 = ( b2_ << 3 ) | ( b2_ >> 2 );
	} else {
		// indvidual mode
		// by replicating the four higher orderbits in the four lower order bits
		r1 = ( BCF_C1_R1_NO_DIFF( baseColorsAndFlags ) << 4 ) | ( BCF_C1_R1_NO_DIFF( baseColorsAndFlags ) );
		r2 = ( BCF_C2_R2_NO_DIFF( baseColorsAndFlags ) << 4 ) | ( BCF_C2_R2_NO_DIFF( baseColorsAndFlags ) );
		g1 = ( BCF_C1_G1_NO_DIFF( baseColorsAndFlags ) << 4 ) | ( BCF_C1_G1_NO_DIFF( baseColorsAndFlags ) );
		g2 = ( BCF_C2_G2_NO_DIFF( baseColorsAndFlags ) << 4 ) | ( BCF_C2_G2_NO_DIFF( baseColorsAndFlags ) );
		b1 = ( BCF_C1_B1_NO_DIFF( baseColorsAndFlags ) << 4 ) | ( BCF_C1_B1_NO_DIFF( baseColorsAndFlags ) );
		b2 = ( BCF_C2_B2_NO_DIFF( baseColorsAndFlags ) << 4 ) | ( BCF_C2_B2_NO_DIFF( baseColorsAndFlags ) );
	}

	for( size_t index = 0; index < 8; index++ ) {
		const uint_fast16_t x = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index >> 1 ) : ( index >> 2 ) );
		const uint_fast16_t y = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index & 1 ) : ( index & 3 ) );
		const uint_fast16_t k = y + ( x * 4 );
		const uint_fast16_t delta = ( ETC1_ModifierTable + ( BCF_CW1( baseColorsAndFlags ) << 2 ) )[( ( pixels >> k ) & 1 ) | ( ( pixels >> ( k + 15 ) ) & 2 )];
		assert( x < ETC1_BLOCK_WIDTH );
		assert( y < ETC1_BLOCK_HEIGHT );

		colors[( y * ETC1_BLOCK_WIDTH ) + x].r = bound( 0, r1 + delta, 255 );
		colors[( y * ETC1_BLOCK_WIDTH ) + x].g = bound( 0, g1 + delta, 255 );
		colors[( y * ETC1_BLOCK_WIDTH ) + x].b = bound( 0, b1 + delta, 255 );
		colors[( y * ETC1_BLOCK_WIDTH ) + x].a = 0;
	}

	for( size_t index = 0; index < 8; index++ ) {
		const uint_fast16_t x = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index >> 1 ) : ( index >> 2 ) + 2 );
		const uint_fast16_t y = ( BCF_FLIP_SET( baseColorsAndFlags ) ? ( index & 1 ) + 2 : ( index & 3 ) );
		const uint_fast16_t k = y + ( x * 4 );
		const uint_fast16_t delta = ( ETC1_ModifierTable + ( BCF_CW2( baseColorsAndFlags ) << 2 ) )[( ( pixels >> k ) & 1 ) | ( ( pixels >> ( k + 15 ) ) & 2 )];
		assert( x < ETC1_BLOCK_WIDTH );
		assert( y < ETC1_BLOCK_HEIGHT );

		colors[( y * ETC1_BLOCK_WIDTH ) + x].r = bound( 0, r2 + delta, 255 );
		colors[( y * ETC1_BLOCK_WIDTH ) + x].g = bound( 0, g2 + delta, 255 );
		colors[( y * ETC1_BLOCK_WIDTH ) + x].b = bound( 0, b2 + delta, 255 );
		colors[( y * ETC1_BLOCK_WIDTH ) + x].a = 0;
	}

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
