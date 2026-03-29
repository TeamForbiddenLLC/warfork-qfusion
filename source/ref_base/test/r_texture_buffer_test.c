#include "r_texture_format.h"
#include "r_texture_buf.h"

#define STB_DS_IMPLEMENTATION 1
#include "stb_ds.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

void Sys_Error( const char *format, ... ){
   assert_true(false);
}

void test_alias_texture_buffer(void **state)
{
  uint8_t* c = malloc(768);
  struct texture_buf_s buffer = {0};
  struct texture_buf_desc_s texturedesc = { 
    .width = 16, 
    .height = 16, 
    .def = R_BaseFormatDef( R_FORMAT_RGB8_UNORM ), 
    .alignment = 1 
  };
  const int res = T_AliasTextureBuf( &buffer, &texturedesc, c, 768);
  assert_int_equal( TEXTURE_BUF_SUCCESS, res );
  assert_uint_equal( T_Size( &buffer ), 768 );
  assert_uint_equal( T_PixelW( &buffer ), 16 );
  assert_uint_equal( T_PixelH( &buffer ), 16 );
  assert_uint_equal( T_LogicalW( &buffer ), 16 );
  assert_uint_equal( T_LogicalH( &buffer ), 16 );
  assert_true( buffer.flags & TEX_BUF_IS_ALIASED);

  const int res2 = T_AliasTextureBuf( &buffer, &texturedesc, c, 128);
  assert_int_equal( TEXTURE_BUF_SUCCESS, res2 );
  
  T_FreeTextureBuf(&buffer);
  free(c);
}

void test_swap_texture_buffer_short_4_4_4_4_rgba_grba(void **state) {
  uint16_t src[8 * 8] = {
    0xde8b,0xd573,0x0d4b,0x872b,0x8ab0,0x8213,0x419d,0x54d4,
    0xab74,0x43d7,0x4816,0x4729,0xb58a,0x7978,0x9088,0x63a3,
    0xbeec,0x14cd,0x4085,0xe67b,0xcaa5,0x6d64,0xe6c9,0xb9ea,
    0xb39d,0x25de,0x475c,0xed16,0x6e6a,0x3d18,0x1199,0xa8a7,
    0x1307,0x47c0,0xdc38,0x7608,0x7e00,0xc468,0x65a5,0x7699,
    0x9029,0x006c,0xb6aa,0x8857,0x2693,0x3cc6,0xa686,0x0c9d,
    0x48d9,0x9a68,0x3720,0x85b0,0xc7b8,0x6eab,0x1a33,0xdcac,
    0x8e99,0xb96a,0x7e83,0x629d,0xd3d5,0x814d,0x6132,0x9b94,
  };
  uint16_t expected[8 * 8] = {
      0xed8b,0x5d73,0xd04b,0x782b,0xa8b0,0x2813,0x149d,0x45d4,
      0xba74,0x34d7,0x8416,0x7429,0x5b8a,0x9778,0x0988,0x36a3,
      0xebec,0x41cd,0x0485,0x6e7b,0xaca5,0xd664,0x6ec9,0x9bea,
      0x3b9d,0x52de,0x745c,0xde16,0xe66a,0xd318,0x1199,0x8aa7,
      0x3107,0x74c0,0xcd38,0x6708,0xe700,0x4c68,0x56a5,0x6799,
      0x0929,0x006c,0x6baa,0x8857,0x6293,0xc3c6,0x6a86,0xc09d,
      0x84d9,0xa968,0x7320,0x58b0,0x7cb8,0xe6ab,0xa133,0xcdac,
      0xe899,0x9b6a,0xe783,0x269d,0x3dd5,0x184d,0x1632,0xb994,
  };
  struct texture_buf_s buffer = {0};
  struct texture_buf_desc_s texturedesc = { 
    .width = 8, 
    .height = 8, 
    .def = R_BaseFormatDef( R_FORMAT_R4_G4_B4_A4_UNORM), 
    .alignment = 1 
  };
  const int res = T_AliasTextureBuf( &buffer, &texturedesc, (uint8_t*)src, sizeof(src));
  assert_int_equal( TEXTURE_BUF_SUCCESS, res);
  enum texture_logical_channel_e swizzle[] = {
    R_LOGICAL_C_GREEN,
    R_LOGICAL_C_RED,
    R_LOGICAL_C_BLUE,
    R_LOGICAL_C_ALPHA
  };
  T_SwizzleInplace(&buffer, swizzle);
  assert_memory_equal(src, expected, sizeof(expected));
}

void test_swizzle_texture_buffer_rgb8_unorm_grba(void **state) {
  uint8_t src[8 * 8 * 3] = {
    228, 0, 0, 238, 198, 0, 65, 52, 0, 206, 253, 0, 45, 212, 0, 138, 147, 0, 61, 151, 0, 100, 167, 0,
    23, 15, 0, 140, 212, 0, 183, 114, 0, 189, 28, 0, 249, 121, 0, 101, 225, 0, 138, 148, 0, 233, 39, 0,
    223, 97, 0, 119, 113, 0, 140, 85, 0, 20, 64, 0, 216, 205, 0, 228, 37, 0, 222, 171, 0, 66, 160, 0,
    200, 58, 0, 5, 130, 0, 239, 32, 0, 228, 65, 0, 196, 4, 0, 17, 66, 0, 130, 134, 0, 238, 26, 0,
    69, 225, 0, 247, 35, 0, 104, 91, 0, 21, 215, 0, 65, 32, 0, 196, 58, 0, 172, 223, 0, 230, 85, 0,
    32, 168, 0, 205, 49, 0, 32, 38, 0, 223, 24, 0, 149, 233, 0, 73, 39, 0, 21, 29, 0, 127, 246, 0,
    11, 196, 0, 181, 48, 0, 2, 207, 0, 105, 42, 0, 200, 238, 0, 188, 207, 0, 97, 21, 0, 54, 174, 0,
    170, 127, 0, 91, 24, 0, 243, 121, 0, 182, 91, 0, 122, 55, 0, 23, 103, 0, 94, 2, 0, 169, 217, 0
  };
  uint8_t expected[8 * 8 * 3] = {
    0, 228, 0, 198, 238, 0, 52, 65, 0, 253, 206, 0, 212, 45, 0, 147, 138, 0, 151, 61, 0, 167, 100, 0,
    15, 23, 0, 212, 140, 0, 114, 183, 0, 28, 189, 0, 121, 249, 0, 225, 101, 0, 148, 138, 0, 39, 233, 0,
    97, 223, 0, 113, 119, 0, 85, 140, 0, 64, 20, 0, 205, 216, 0, 37, 228, 0, 171, 222, 0, 160, 66, 0,
    58, 200, 0, 130, 5, 0, 32, 239, 0, 65, 228, 0, 4, 196, 0, 66, 17, 0, 134, 130, 0, 26, 238, 0,
    225, 69, 0, 35, 247, 0, 91, 104, 0, 215, 21, 0, 32, 65, 0, 58, 196, 0, 223, 172, 0, 85, 230, 0,
    168, 32, 0, 49, 205, 0, 38, 32, 0, 24, 223, 0, 233, 149, 0, 39, 73, 0, 29, 21, 0, 246, 127, 0,
    196, 11, 0, 48, 181, 0, 207, 2, 0, 42, 105, 0, 238, 200, 0, 207, 188, 0, 21, 97, 0, 174, 54, 0,
    127, 170, 0, 24, 91, 0, 121, 243, 0, 91, 182, 0, 55, 122, 0, 103, 23, 0, 2, 94, 0, 217, 169, 0
  };
  struct texture_buf_s buffer = {0};
  struct texture_buf_desc_s texturedesc = { 
    .width = 8, 
    .height = 8, 
    .def = R_BaseFormatDef( R_FORMAT_RGB8_UNORM ), 
    .alignment = 1 
  };
  const int res = T_AliasTextureBuf( &buffer, &texturedesc, src, sizeof(src));
  assert_int_equal( TEXTURE_BUF_SUCCESS, res);
  enum texture_logical_channel_e swizzle[] = {
    R_LOGICAL_C_GREEN,
    R_LOGICAL_C_RED,
    R_LOGICAL_C_BLUE,
  };
  T_SwizzleInplace(&buffer, swizzle);
  assert_memory_equal(expected, src, sizeof(expected));
}

void test_promote_textuire_buffer(void **state) {
  uint8_t* c = malloc(768);
  struct texture_buf_s buffer = {0};
  struct texture_buf_desc_s texturedesc = { 
    .width = 16, 
    .height = 16, 
    .def = R_BaseFormatDef( R_FORMAT_RGB8_UNORM ), 
    .alignment = 1 
  };
  const int res = T_AliasTextureBuf( &buffer, &texturedesc, c, 768);
  assert_int_equal( TEXTURE_BUF_SUCCESS, res );

  T_PromoteTextureBuf(&buffer);
  assert_ptr_not_equal(buffer.buffer, c );
  assert_false(buffer.flags & TEX_BUF_IS_ALIASED);

  T_FreeTextureBuf(&buffer);
  free(c);
}

int main( void )
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test( test_alias_texture_buffer ),
		cmocka_unit_test( test_swizzle_texture_buffer_rgb8_unorm_grba ),
		cmocka_unit_test( test_promote_textuire_buffer ),
		cmocka_unit_test( test_swap_texture_buffer_short_4_4_4_4_rgba_grba ),
	};

	return cmocka_run_group_tests( tests, NULL, NULL );
}
