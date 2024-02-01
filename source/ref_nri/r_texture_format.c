#include "r_texture_format.h"

NriFormat R_ToNRIFormat(enum texture_format_e format) {
  switch(format) {
    case R_FORMAT_R8_UNORM: return NriFormat_R8_UNORM;
    case R_FORMAT_R8_SNORM: return NriFormat_R8_SNORM;
    case R_FORMAT_R8_UINT: return NriFormat_R8_UINT;
    case R_FORMAT_R8_SINT: return NriFormat_R8_SINT;
    case R_FORMAT_RG8_UNORM: return NriFormat_RG8_UNORM;
    case R_FORMAT_RG8_SNORM: return NriFormat_RG8_SNORM;
    case R_FORMAT_RG8_UINT: return NriFormat_RG8_UINT;
    case R_FORMAT_RG8_SINT: return NriFormat_RG8_SINT;
    case R_FORMAT_BGRA8_UNORM: return NriFormat_BGRA8_UNORM;
    case R_FORMAT_BGRA8_SRGB: return NriFormat_BGRA8_SRGB;
    case R_FORMAT_RGBA8_UNORM: return NriFormat_RGBA8_UNORM;
    case R_FORMAT_RGBA8_SNORM: return NriFormat_RGBA8_SNORM;
    case R_FORMAT_RGBA8_UINT: return NriFormat_RGBA8_UINT;
    case R_FORMAT_RGBA8_SINT: return NriFormat_RGBA8_SINT;
    case R_FORMAT_RGBA8_SRGB: return NriFormat_RGBA8_SRGB;
    case R_FORMAT_R16_UNORM: return NriFormat_R16_UNORM;
    case R_FORMAT_R16_SNORM: return NriFormat_R16_SNORM;
    case R_FORMAT_R16_UINT: return NriFormat_R16_UINT;
    case R_FORMAT_R16_SINT: return NriFormat_R16_SINT;
    case R_FORMAT_R16_SFLOAT: return NriFormat_R16_SFLOAT;
    case R_FORMAT_RG16_UNORM: return NriFormat_RG16_UNORM;
    case R_FORMAT_RG16_SNORM: return NriFormat_RG16_SNORM;
    case R_FORMAT_RG16_UINT: return NriFormat_RG16_UINT;
    case R_FORMAT_RG16_SINT: return NriFormat_RG16_SINT;
    case R_FORMAT_RG16_SFLOAT: return NriFormat_RG16_SFLOAT;
    case R_FORMAT_RGBA16_UNORM: return NriFormat_RGBA16_UNORM;
    case R_FORMAT_RGBA16_SNORM: return NriFormat_RGBA16_SNORM;
    case R_FORMAT_RGBA16_UINT: return NriFormat_RGBA16_UINT;
    case R_FORMAT_RGBA16_SINT: return NriFormat_RGBA16_SINT;
    case R_FORMAT_RGBA16_SFLOAT: return NriFormat_RGBA16_SFLOAT;
    case R_FORMAT_R32_UINT: return NriFormat_R32_UINT;
    case R_FORMAT_R32_SINT: return NriFormat_R32_SINT;
    case R_FORMAT_R32_SFLOAT: return NriFormat_R32_SFLOAT;
    case R_FORMAT_RG32_UINT: return NriFormat_RG32_UINT;
    case R_FORMAT_RG32_SINT: return NriFormat_RG32_SINT;
    case R_FORMAT_RG32_SFLOAT: return NriFormat_RG32_SFLOAT;
    case R_FORMAT_RGB32_UINT: return NriFormat_RGB32_UINT;
    case R_FORMAT_RGB32_SINT: return NriFormat_RGB32_SINT;
    case R_FORMAT_RGB32_SFLOAT: return NriFormat_RGB32_SFLOAT;
    case R_FORMAT_RGBA32_UINT: return NriFormat_RGBA32_UINT;
    case R_FORMAT_RGBA32_SINT: return NriFormat_RGBA32_SINT;
    case R_FORMAT_RGBA32_SFLOAT: return NriFormat_RGBA32_SFLOAT;
    case R_FORMAT_R10_G10_B10_A2_UNORM: return NriFormat_R10_G10_B10_A2_UNORM;
    case R_FORMAT_R10_G10_B10_A2_UINT: return NriFormat_R10_G10_B10_A2_UINT;
    case R_FORMAT_R11_G11_B10_UFLOAT: return NriFormat_R11_G11_B10_UFLOAT;
    case R_FORMAT_R9_G9_B9_E5_UFLOAT: return NriFormat_R9_G9_B9_E5_UFLOAT;
    case R_FORMAT_BC1_RGBA_UNORM: return NriFormat_BC1_RGBA_UNORM;
    case R_FORMAT_BC1_RGBA_SRGB: return NriFormat_BC1_RGBA_SRGB;
    case R_FORMAT_BC2_RGBA_UNORM: return NriFormat_BC2_RGBA_UNORM;
    case R_FORMAT_BC2_RGBA_SRGB: return NriFormat_BC2_RGBA_SRGB;
    case R_FORMAT_BC3_RGBA_UNORM: return NriFormat_BC3_RGBA_UNORM;
    case R_FORMAT_BC3_RGBA_SRGB: return NriFormat_BC3_RGBA_SRGB;
    case R_FORMAT_BC4_R_UNORM: return NriFormat_BC4_R_UNORM;
    case R_FORMAT_BC4_R_SNORM: return NriFormat_BC4_R_SNORM;
    case R_FORMAT_BC5_RG_UNORM: return NriFormat_BC5_RG_UNORM;
    case R_FORMAT_BC5_RG_SNORM: return NriFormat_BC5_RG_SNORM;
    case R_FORMAT_BC6H_RGB_UFLOAT: return NriFormat_BC6H_RGB_UFLOAT;
    case R_FORMAT_BC6H_RGB_SFLOAT: return NriFormat_BC6H_RGB_SFLOAT;
    case R_FORMAT_BC7_RGBA_UNORM: return NriFormat_BC7_RGBA_UNORM;
    case R_FORMAT_BC7_RGBA_SRGB: return NriFormat_BC7_RGBA_SRGB;
    case R_FORMAT_D16_UNORM: return NriFormat_D16_UNORM;
    case R_FORMAT_D24_UNORM_S8_UINT: return NriFormat_D24_UNORM_S8_UINT;
    case R_FORMAT_D32_SFLOAT: return NriFormat_D32_SFLOAT;
    case R_FORMAT_D32_SFLOAT_S8_UINT_X24: return NriFormat_D32_SFLOAT_S8_UINT_X24;
    case R_FORMAT_R24_UNORM_X8: return NriFormat_R24_UNORM_X8;
    case R_FORMAT_X24_R8_UINT: return NriFormat_X24_R8_UINT;
    case R_FORMAT_X32_R8_UINT_X24: return NriFormat_X32_R8_UINT_X24;
    case R_FORMAT_R32_SFLOAT_X8_X24: return NriFormat_R32_SFLOAT_X8_X24;
    default:
      break;
  }
  return NriFormat_UNKNOWN;
}

uint32_t R_FormatChannelCount(enum texture_format_e format) {
  switch(format) {
    case R_FORMAT_R8_UNORM: return 1;
    case R_FORMAT_R8_SNORM: return 1;
    case R_FORMAT_R8_UINT: return 1;
    case R_FORMAT_R8_SINT: return 1;
    case R_FORMAT_BGRA8_UNORM: return 2;
    case R_FORMAT_BGRA8_SRGB: return 2;
    case R_FORMAT_RG8_UNORM: return 2;
    case R_FORMAT_RG8_SNORM: return 2;
    case R_FORMAT_RG8_UINT: return 2;
    case R_FORMAT_RG8_SINT: return 2;
    case R_FORMAT_R16_UNORM: return 1;
    case R_FORMAT_R16_SNORM: return 1;
    case R_FORMAT_R16_UINT: return 1;
    case R_FORMAT_R16_SINT: return 1;
    case R_FORMAT_R16_SFLOAT: return 1;
    case R_FORMAT_RGBA16_UNORM: return 4;
    case R_FORMAT_RGBA16_SNORM: return 4;
    case R_FORMAT_RGBA16_UINT: return 4;
    case R_FORMAT_RGBA16_SINT: return 4;
    case R_FORMAT_RGBA16_SFLOAT: return 4;
    case R_FORMAT_R32_UINT: return 1;
    case R_FORMAT_R32_SINT: return 1;
    case R_FORMAT_R32_SFLOAT: return 1;
    case R_FORMAT_RG32_UINT: return 2;
    case R_FORMAT_RG32_SINT: return 2;
    case R_FORMAT_RG32_SFLOAT: return 2;
    case R_FORMAT_RGB32_UINT: return 3;
    case R_FORMAT_RGB32_SINT: return 3;
    case R_FORMAT_RGB32_SFLOAT: return 3;
    case R_FORMAT_RGBA32_UINT: return 4;
    case R_FORMAT_RGBA32_SINT: return 4;
    case R_FORMAT_RGBA32_SFLOAT: return 4;
    case R_FORMAT_BGR8_UNORM: return 3;
    //case R_FORMAT_R10_G10_B10_A2_UNORM: return 4;
    //case R_FORMAT_R10_G10_B10_A2_UINT: return 4;
    //case R_FORMAT_R11_G11_B10_UFLOAT: return 4;
    //case R_FORMAT_R9_G9_B9_E5_UFLOAT: return 32;
    //case R_FORMAT_BC1_RGBA_UNORM: return 64;
    //case R_FORMAT_BC1_RGBA_SRGB: return 64;
    //case R_FORMAT_BC2_RGBA_UNORM: return 128;
    //case R_FORMAT_BC2_RGBA_SRGB: return 128;
    //case R_FORMAT_BC3_RGBA_UNORM: return 128;
    //case R_FORMAT_BC3_RGBA_SRGB: return 128;
    //case R_FORMAT_BC4_R_UNORM: return 0;
    //case R_FORMAT_BC4_R_SNORM: return 0;
    //case R_FORMAT_BC5_RG_UNORM: return 0;
    //case R_FORMAT_BC5_RG_SNORM: return 0;
    //case R_FORMAT_BC6H_RGB_UFLOAT: return 0;
    //case R_FORMAT_BC6H_RGB_SFLOAT: return 0;
    //case R_FORMAT_BC7_RGBA_UNORM: return 0;
    //case R_FORMAT_BC7_RGBA_SRGB: return 0;

    //case R_FORMAT_D16_UNORM: return 16;
    //case R_FORMAT_D24_UNORM_S8_UINT: return 32;
    //case R_FORMAT_D32_SFLOAT: return 32;
    //case R_FORMAT_D32_SFLOAT_S8_UINT_X24: return 64;

    //case R_FORMAT_R24_UNORM_X8: return 32;
    //case R_FORMAT_X24_R8_UINT: return 32;
    //case R_FORMAT_X32_R8_UINT_X24: return 64;
    //case R_FORMAT_R32_SFLOAT_X8_X24: return 64;
    default:
      assert(false);
  }
  return 0;
}

uint32_t R_FormatBitSizePerBlock(enum texture_format_e format) {
  switch(format) {
    case R_FORMAT_R8_UNORM: return 8;
    case R_FORMAT_R8_SNORM: return 8;
    case R_FORMAT_R8_UINT: return 8;
    case R_FORMAT_R8_SINT: return 8;
    case R_FORMAT_RGBA8_UNORM: return 32;
    case R_FORMAT_BGRA8_UNORM: return 32;
    case R_FORMAT_BGRA8_SRGB: return 32;
    case R_FORMAT_RG8_UNORM: return 16;
    case R_FORMAT_RG8_SNORM: return 16;
    case R_FORMAT_RG8_UINT: return 16;
    case R_FORMAT_RG8_SINT: return 16;
    case R_FORMAT_R16_UNORM: return 16;
    case R_FORMAT_R16_SNORM: return 16;
    case R_FORMAT_R16_UINT: return 16;
    case R_FORMAT_R16_SINT: return 16;
    case R_FORMAT_R16_SFLOAT: return 16;
    case R_FORMAT_RGBA16_UNORM: return 64;
    case R_FORMAT_RGBA16_SNORM: return 64;
    case R_FORMAT_RGBA16_UINT: return 64;
    case R_FORMAT_RGBA16_SINT: return 64;
    case R_FORMAT_RGBA16_SFLOAT: return 64;

    case R_FORMAT_R32_UINT: return 32;
    case R_FORMAT_R32_SINT: return 32;
    case R_FORMAT_R32_SFLOAT: return 32;

    case R_FORMAT_BGR8_UNORM: return 24;
    case R_FORMAT_RGB8_UNORM: return 24;

    case R_FORMAT_RG32_UINT: return 64;
    case R_FORMAT_RG32_SINT: return 64;
    case R_FORMAT_RG32_SFLOAT: return 64;
    case R_FORMAT_RGB32_UINT: return 96;
    case R_FORMAT_RGB32_SINT: return 96;
    case R_FORMAT_RGB32_SFLOAT: return 96;
    case R_FORMAT_RGBA32_UINT: return 128;
    case R_FORMAT_RGBA32_SINT: return 128;
    case R_FORMAT_RGBA32_SFLOAT: return 128;
    //case R_FORMAT_R10_G10_B10_A2_UNORM: return 32;
    //case R_FORMAT_R10_G10_B10_A2_UINT: return 32;
    //case R_FORMAT_R11_G11_B10_UFLOAT: return 32;
    //case R_FORMAT_R9_G9_B9_E5_UFLOAT: return 32;

    // TODO: fix these formats
    //case R_FORMAT_BC1_RGBA_UNORM: return 64;
    //case R_FORMAT_BC1_RGBA_SRGB: return 64;
    //case R_FORMAT_BC2_RGBA_UNORM: return 128;
    //case R_FORMAT_BC2_RGBA_SRGB: return 128;
    //case R_FORMAT_BC3_RGBA_UNORM: return 128;
    //case R_FORMAT_BC3_RGBA_SRGB: return 128;
    //case R_FORMAT_BC4_R_UNORM: return 0;
    //case R_FORMAT_BC4_R_SNORM: return 0;
    //case R_FORMAT_BC5_RG_UNORM: return 0;
    //case R_FORMAT_BC5_RG_SNORM: return 0;
    //case R_FORMAT_BC6H_RGB_UFLOAT: return 0;
    //case R_FORMAT_BC6H_RGB_SFLOAT: return 0;
    //case R_FORMAT_BC7_RGBA_UNORM: return 0;
    //case R_FORMAT_BC7_RGBA_SRGB: return 0;

    //case R_FORMAT_D16_UNORM: return 16;
    //case R_FORMAT_D24_UNORM_S8_UINT: return 32;
    //case R_FORMAT_D32_SFLOAT: return 32;
    //case R_FORMAT_D32_SFLOAT_S8_UINT_X24: return 64;

    //case R_FORMAT_R24_UNORM_X8: return 32;
    //case R_FORMAT_X24_R8_UINT: return 32;
    //case R_FORMAT_X32_R8_UINT_X24: return 64;
    //case R_FORMAT_R32_SFLOAT_X8_X24: return 64;
    default:
      assert(false);
  }
  return 0;
}

