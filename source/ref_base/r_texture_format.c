#include "r_texture_format.h"
#include "../gameshared/q_math.h"
#include <stdint.h>

static const struct base_format_def_s baseFormats[] = {
  [R_FORMAT_ETC1_R8G8B8_OES ] = {
    .format = R_FORMAT_ETC1_R8G8B8_OES, 
    .base = R_BASE_BLOCKED_COMPRESSED,
    .compressed = {
      .blockByteSize = 8,
      .blockWidth = 4,
      .blockHeight = 4
    },
  },
  [R_FORMAT_ETC2_R8G8B8_UNORM ] = {
    .format = R_FORMAT_ETC2_R8G8B8_UNORM , 
    .base = R_BASE_BLOCKED_COMPRESSED,
    .compressed = {
      .blockByteSize = 8,
      .blockWidth = 4,
      .blockHeight = 4
    },
  },

  [R_FORMAT_RGB8_UNORM] = {
    .format = R_FORMAT_RGB8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_8 = {
      .numChannels = 3,
      .channels = {R_LOGICAL_C_RED, R_LOGICAL_C_GREEN, R_LOGICAL_C_BLUE},
      .normalized = 1,
      .sign = 0
    },
  },
  [R_FORMAT_R4_G4_B4_A4_UNORM ] = {
    .format = R_FORMAT_R4_G4_B4_A4_UNORM , 
    .base = R_BASE_FORMAT_PACKED_16,
    .packed_16 = {
      .numChannels = 4,
      .bits = { 
        {.offset = 12, .mask = 0xf},
        {.offset = 8, .mask = 0xf},
        {.offset = 4, .mask = 0xf},
        {.offset = 0, .mask = 0xf}
      },
      .channels = {R_LOGICAL_C_RED, R_LOGICAL_C_GREEN, R_LOGICAL_C_BLUE, R_LOGICAL_C_ALPHA},
    },
  },
  [R_FORMAT_R5_G6_B5_UNORM ] = {
    .format = R_FORMAT_R5_G6_B5_UNORM, 
    .base = R_BASE_FORMAT_PACKED_16,
    .packed_16 = {
      .numChannels = 3,
      .bits = { 
        {.offset = 11, .mask = 0x1f},
        {.offset = 5, .mask = 0x3f},
        {.offset = 0, .mask = 0x1f},
      },
      .channels = {R_LOGICAL_C_RED, R_LOGICAL_C_GREEN, R_LOGICAL_C_BLUE},
    },
  },
  [R_FORMAT_R5_G5_B5_A1_UNORM] = {
    .format = R_FORMAT_R5_G5_B5_A1_UNORM, 
    .base = R_BASE_FORMAT_PACKED_16,
    .packed_16 = {
      .numChannels = 4,
      .bits = { 
        {.offset = 11, .mask = 0x1f},
        {.offset = 6, .mask = 0x1F},
        {.offset = 1, .mask = 0x1F},
        {.offset = 0, .mask = 0x01},
      },
      .channels = {R_LOGICAL_C_RED, R_LOGICAL_C_GREEN, R_LOGICAL_C_BLUE, R_LOGICAL_C_ALPHA},
    },
  },
  [R_FORMAT_A8_UNORM] = {
    .format = R_FORMAT_A8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_8 = {
      //.blockByteSize = 1,
      .numChannels = 1,
      .channels = {R_LOGICAL_C_ALPHA},
      .normalized = 1,
      .sign = 0
    },
  },
  [R_FORMAT_L8_UNORM] = {
    .format = R_FORMAT_L8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_8 = {
      //.blockByteSize = 1,
      .numChannels = 1,
      .channels = {R_LOGICAL_C_LUMINANCE},
      .normalized = 1,
      .sign = 0
    },
  },
  [R_FORMAT_L8_A8_UNORM] = {
    .format = R_FORMAT_L8_A8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_8 = {
      //.blockByteSize = 2,
      .numChannels = 2,
      .channels = {R_LOGICAL_C_LUMINANCE, R_LOGICAL_C_ALPHA},
      .normalized = 1,
      .sign = 0
    },
  },
  [R_FORMAT_RGBA8_UNORM] = {
    .format = R_FORMAT_RGBA8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_8 = {
      //.blockByteSize = 4,
      .numChannels = 4,
      .channels = {R_LOGICAL_C_RED, R_LOGICAL_C_GREEN, R_LOGICAL_C_BLUE, R_LOGICAL_C_ALPHA},
      .normalized = 1
    }
  },
  [R_FORMAT_R8_UNORM] = {
    .format = R_FORMAT_R8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_8 = {
      //.blockByteSize = 1,
      .numChannels = 1,
      .channels = {R_LOGICAL_C_RED},
      .normalized = 1
    }
  },
  [R_FORMAT_BGR8_UNORM] = {
    .format = R_FORMAT_BGR8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_8 = {
      //.blockByteSize = 3,
      .numChannels = 3,
      .channels = {R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED},
      .normalized = 1
    }
  },
  [R_FORMAT_BGRA8_UNORM] = {
    .format = R_FORMAT_BGRA8_UNORM, 
    .base = R_BASE_FORMAT_FIXED_8,
    .fixed_16 = {
      .numChannels = 4,
      .channels = {R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED, R_LOGICAL_C_ALPHA},
      .normalized = 1
    }
  }, 
  [R_FORMAT_RG16_UNORM] = {
    .base = R_BASE_FORMAT_FIXED_16,
    .fixed_16 = {
      .numChannels = 2,
      .channels = {R_LOGICAL_C_RED, R_LOGICAL_C_GREEN},
      .normalized = 1
    }
  },
  [R_TEXTURE_FORMAT_COUNT] = {0}
}; 

uint32_t R_FormatChannelCount(enum texture_format_e format) {
  switch(format) {
    case R_FORMAT_R8_UNORM: return 1;
    case R_FORMAT_R8_SNORM: return 1;
    case R_FORMAT_R8_UINT: return 1;
    case R_FORMAT_R8_SINT: return 1;
    case R_FORMAT_RGB8_UNORM: return 3;
    case R_FORMAT_BGRA8_UNORM: return 4;
    case R_FORMAT_BGRA8_SRGB: return 4;
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
    default:
      assert(false);
  }
  return 0;
}

uint32_t R_FormatChannelBitWidth( enum texture_format_e format, uint16_t channel ) {
  if(channel == 0) {
    switch(format) {
      case R_FORMAT_R8_UNORM: return 8;
      case R_FORMAT_R8_SNORM: return 8;
      case R_FORMAT_R8_UINT: return 8;
      case R_FORMAT_R8_SINT: return 8;
      case R_FORMAT_RGBA8_UNORM: return 8;
      case R_FORMAT_BGRA8_UNORM: return 8;
      case R_FORMAT_BGRA8_SRGB: return 8;
      case R_FORMAT_RG8_UNORM: return 8;
      case R_FORMAT_RG8_SNORM: return 8;
      case R_FORMAT_RG8_UINT: return 8;
      case R_FORMAT_RG8_SINT: return 8;
      case R_FORMAT_R16_UNORM: return 16;
      case R_FORMAT_R16_SNORM: return 16;
      case R_FORMAT_R16_UINT: return 16;
      case R_FORMAT_R16_SINT: return 16;
      case R_FORMAT_R16_SFLOAT: return 16;
      case R_FORMAT_RGBA16_UNORM: return 16;
      case R_FORMAT_RGBA16_SNORM: return 16;
      case R_FORMAT_RGBA16_UINT: return 16;
      case R_FORMAT_RGBA16_SINT: return 16;
      case R_FORMAT_RGBA16_SFLOAT: return 16;
      case R_FORMAT_R32_UINT: return 32;
      case R_FORMAT_R32_SINT: return 32;
      case R_FORMAT_R32_SFLOAT: return 32;
      case R_FORMAT_BGR8_UNORM: return 8;
      case R_FORMAT_RGB8_UNORM: return 8;
      case R_FORMAT_RG32_UINT: return 32;
      case R_FORMAT_RG32_SINT: return 32;
      case R_FORMAT_RG32_SFLOAT: return 32;
      case R_FORMAT_RGB32_UINT: return 32;
      case R_FORMAT_RGB32_SINT: return 32;
      case R_FORMAT_RGB32_SFLOAT: return 32;
      case R_FORMAT_RGBA32_UINT: return 32;
      case R_FORMAT_RGBA32_SINT: return 32;
      case R_FORMAT_RGBA32_SFLOAT: return 32;
      default:
        assert(false);
    }
  } else if(channel == 1) {

    switch(format) {
      case R_FORMAT_R8_UNORM: return 0;
      case R_FORMAT_R8_SNORM: return 0;
      case R_FORMAT_R8_UINT: return 0;
      case R_FORMAT_R8_SINT: return 0;
      case R_FORMAT_RGBA8_UNORM: return 8;
      case R_FORMAT_BGRA8_UNORM: return 8;
      case R_FORMAT_BGRA8_SRGB: return 8;
      case R_FORMAT_RG8_UNORM: return 8;
      case R_FORMAT_RG8_SNORM: return 8;
      case R_FORMAT_RG8_UINT: return 8;
      case R_FORMAT_RG8_SINT: return 8;
      case R_FORMAT_R16_UNORM: return 0;
      case R_FORMAT_R16_SNORM: return 0;
      case R_FORMAT_R16_UINT: return 0;
      case R_FORMAT_R16_SINT: return 0;
      case R_FORMAT_R16_SFLOAT: return 0;
      case R_FORMAT_RGBA16_UNORM: return 16;
      case R_FORMAT_RGBA16_SNORM: return 16;
      case R_FORMAT_RGBA16_UINT: return 16;
      case R_FORMAT_RGBA16_SINT: return 16;
      case R_FORMAT_RGBA16_SFLOAT: return 16;
      case R_FORMAT_R32_UINT: return 0;
      case R_FORMAT_R32_SINT: return 0;
      case R_FORMAT_R32_SFLOAT: return 0;
      case R_FORMAT_BGR8_UNORM: return 8;
      case R_FORMAT_RGB8_UNORM: return 8;
      case R_FORMAT_RG32_UINT: return 32;
      case R_FORMAT_RG32_SINT: return 32;
      case R_FORMAT_RG32_SFLOAT: return 32;
      case R_FORMAT_RGB32_UINT: return 32;
      case R_FORMAT_RGB32_SINT: return 32;
      case R_FORMAT_RGB32_SFLOAT: return 32;
      case R_FORMAT_RGBA32_UINT: return 32;
      case R_FORMAT_RGBA32_SINT: return 32;
      case R_FORMAT_RGBA32_SFLOAT: return 32;
      default:
        assert(false);
    }
  } else if(channel == 2) {
    switch(format) {
      case R_FORMAT_R8_UNORM: return 0;
      case R_FORMAT_R8_SNORM: return 0;
      case R_FORMAT_R8_UINT: return 0;
      case R_FORMAT_R8_SINT: return 0;
      case R_FORMAT_RGBA8_UNORM: return 8;
      case R_FORMAT_BGRA8_UNORM: return 8;
      case R_FORMAT_BGRA8_SRGB: return 8;
      case R_FORMAT_RG8_UNORM: return 0;
      case R_FORMAT_RG8_SNORM: return 0;
      case R_FORMAT_RG8_UINT: return 0;
      case R_FORMAT_RG8_SINT: return 0;
      case R_FORMAT_R16_UNORM: return 0;
      case R_FORMAT_R16_SNORM: return 0;
      case R_FORMAT_R16_UINT: return 0;
      case R_FORMAT_R16_SINT: return 0;
      case R_FORMAT_R16_SFLOAT: return 0;
      case R_FORMAT_RGBA16_UNORM: return 16;
      case R_FORMAT_RGBA16_SNORM: return 16;
      case R_FORMAT_RGBA16_UINT: return 16;
      case R_FORMAT_RGBA16_SINT: return 16;
      case R_FORMAT_RGBA16_SFLOAT: return 16;
      case R_FORMAT_R32_UINT: return 0;
      case R_FORMAT_R32_SINT: return 0;
      case R_FORMAT_R32_SFLOAT: return 0;
      case R_FORMAT_BGR8_UNORM: return 8;
      case R_FORMAT_RGB8_UNORM: return 8;
      case R_FORMAT_RG32_UINT: return 0;
      case R_FORMAT_RG32_SINT: return 0;
      case R_FORMAT_RG32_SFLOAT: return 0;
      case R_FORMAT_RGB32_UINT: return 32;
      case R_FORMAT_RGB32_SINT: return 32;
      case R_FORMAT_RGB32_SFLOAT: return 32;
      case R_FORMAT_RGBA32_UINT: return 32;
      case R_FORMAT_RGBA32_SINT: return 32;
      case R_FORMAT_RGBA32_SFLOAT: return 32;
      default:
        assert(false);
    }
  } else if(channel == 3) {
    switch(format) {
      case R_FORMAT_R8_UNORM: return 0;
      case R_FORMAT_R8_SNORM: return 0;
      case R_FORMAT_R8_UINT: return 0;
      case R_FORMAT_R8_SINT: return 0;
      case R_FORMAT_RGBA8_UNORM: return 8;
      case R_FORMAT_BGRA8_UNORM: return 8;
      case R_FORMAT_BGRA8_SRGB: return 8;
      case R_FORMAT_RG8_UNORM: return 8;
      case R_FORMAT_RG8_SNORM: return 8;
      case R_FORMAT_RG8_UINT: return 8;
      case R_FORMAT_RG8_SINT: return 8;
      case R_FORMAT_R16_UNORM: return 0;
      case R_FORMAT_R16_SNORM: return 0;
      case R_FORMAT_R16_UINT: return 0;
      case R_FORMAT_R16_SINT: return 0;
      case R_FORMAT_R16_SFLOAT: return 0;
      case R_FORMAT_RGBA16_UNORM: return 16;
      case R_FORMAT_RGBA16_SNORM: return 16;
      case R_FORMAT_RGBA16_UINT: return 16;
      case R_FORMAT_RGBA16_SINT: return 16;
      case R_FORMAT_RGBA16_SFLOAT: return 16;
      case R_FORMAT_R32_UINT: return 0;
      case R_FORMAT_R32_SINT: return 0;
      case R_FORMAT_R32_SFLOAT: return 0;
      case R_FORMAT_BGR8_UNORM: return 0;
      case R_FORMAT_RGB8_UNORM: return 0;
      case R_FORMAT_RG32_UINT: return 0;
      case R_FORMAT_RG32_SINT: return 0;
      case R_FORMAT_RG32_SFLOAT: return 0;
      case R_FORMAT_RGB32_UINT: return 0;
      case R_FORMAT_RGB32_SINT: return 0;
      case R_FORMAT_RGB32_SFLOAT: return 0;
      case R_FORMAT_RGBA32_UINT: return 32;
      case R_FORMAT_RGBA32_SINT: return 32;
      case R_FORMAT_RGBA32_SFLOAT: return 32;
      default:
        assert(false);
    }
  }
  return R_FORMAT_UNKNOWN;
}

bool R_FormatIsCompressed( enum texture_format_e format )
{
	switch( format ) {
		case R_FORMAT_ETC1_R8G8B8_OES: // R_FORMAT_ETC2_R8G8B8_UNORM is a superset
		case R_FORMAT_ETC2_R8G8B8_UNORM:
		case R_FORMAT_ETC2_R8G8B8_SRGB:
		case R_FORMAT_ETC2_R8G8B8A1_UNORM:
		case R_FORMAT_ETC2_R8G8B8A1_SRGB:
		case R_FORMAT_ETC2_R8G8B8A8_UNORM:
		case R_FORMAT_ETC2_R8G8B8A8_SRGB:
		case R_FORMAT_ETC2_EAC_R11_UNORM:
		case R_FORMAT_ETC2_EAC_R11_SNORM:
		case R_FORMAT_ETC2_EAC_R11G11_UNORM:
		case R_FORMAT_ETC2_EAC_R11G11_SNORM:
			return true;
		default:
			break;
	}
	return false;
}

uint32_t R_FormatBlockWidth( enum texture_format_e format ) {
	switch( format ) {
		case R_FORMAT_ETC1_R8G8B8_OES: return 4;
		case R_FORMAT_ETC2_R8G8B8_UNORM:return 4;
		case R_FORMAT_ETC2_R8G8B8_SRGB: return 4;
		case R_FORMAT_ETC2_R8G8B8A1_UNORM: return 4;
		case R_FORMAT_ETC2_R8G8B8A1_SRGB: return 4;
		case R_FORMAT_ETC2_R8G8B8A8_UNORM: return 4;
		case R_FORMAT_ETC2_R8G8B8A8_SRGB: return 4;
		case R_FORMAT_ETC2_EAC_R11_UNORM: return 4;
		case R_FORMAT_ETC2_EAC_R11_SNORM: return 4;
		case R_FORMAT_ETC2_EAC_R11G11_UNORM: return 4;
		case R_FORMAT_ETC2_EAC_R11G11_SNORM: return 4;
		default:
			break;
	}
	return 1;
}

uint32_t R_FormatBlockHeight( enum texture_format_e format )
{
	switch( format ) {
		case R_FORMAT_ETC1_R8G8B8_OES: return 4;
		case R_FORMAT_ETC2_R8G8B8_UNORM:return 4;
		case R_FORMAT_ETC2_R8G8B8_SRGB: return 4;
		case R_FORMAT_ETC2_R8G8B8A1_UNORM: return 4;
		case R_FORMAT_ETC2_R8G8B8A1_SRGB: return 4;
		case R_FORMAT_ETC2_R8G8B8A8_UNORM: return 4;
		case R_FORMAT_ETC2_R8G8B8A8_SRGB: return 4;
		case R_FORMAT_ETC2_EAC_R11_UNORM: return 4;
		case R_FORMAT_ETC2_EAC_R11_SNORM: return 4;
		case R_FORMAT_ETC2_EAC_R11G11_UNORM: return 4;
		case R_FORMAT_ETC2_EAC_R11G11_SNORM: return 4;
		default:
			break;
	}
	return 1;
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
   
    // compressed formats
    case R_FORMAT_ETC1_R8G8B8_OES: return 64;
	  case R_FORMAT_ETC2_R8G8B8_UNORM: return 64;
	  case R_FORMAT_ETC2_R8G8B8_SRGB: return 64;
	  case R_FORMAT_ETC2_R8G8B8A1_UNORM: return 64;
	  case R_FORMAT_ETC2_R8G8B8A1_SRGB: return 64;
	  case R_FORMAT_ETC2_R8G8B8A8_UNORM: return 64;
	  case R_FORMAT_ETC2_R8G8B8A8_SRGB: return 64;
    default:
      assert(false);
  }
  return 0;
}
const size_t RT_BlockSize(const struct base_format_def_s* def) {
	switch( def->base ) {
		case R_BASE_BLOCKED_COMPRESSED:
			return def->compressed.blockByteSize;
		case R_BASE_FORMAT_FIXED_8:
			return def->fixed_8.numChannels;
		case R_BASE_FORMAT_FIXED_16:
			return def->fixed_16.numChannels * sizeof( uint16_t );
		case R_BASE_FORMAT_FIXED_32:
			return def->fixed_32.numChannels * sizeof( uint32_t );
		case R_BASE_FORMAT_PACKED_16:
			return sizeof( uint16_t );
		default:
			break;
	}
	return 0;
}

const uint_fast16_t RT_NumberChannels(const struct base_format_def_s* def) {
	switch( def->base ) {
		case R_BASE_FORMAT_FIXED_8:
			return def->fixed_8.numChannels;
		case R_BASE_FORMAT_FIXED_16:
			return def->fixed_16.numChannels;
		case R_BASE_FORMAT_FIXED_32:
			return def->fixed_32.numChannels;
		case R_BASE_FORMAT_PACKED_16:
			return sizeof( uint16_t );
		default:
			break;
	}
	return 0;

}
const enum texture_logical_channel_e* RT_Channels(const struct base_format_def_s* def) {
	switch( def->base ) {
		case R_BASE_FORMAT_FIXED_8:
			return def->fixed_8.channels;
		case R_BASE_FORMAT_FIXED_16:
			return def->fixed_16.channels;
		case R_BASE_FORMAT_FIXED_32:
			return def->fixed_32.channels;
	 // case R_BASE_FORMAT_PACKED_16:
	 // 	return sizeof( uint16_t );
		default:
			break;
	}
	return NULL;
}

const bool RT_ExpectChannelsMatch( const struct base_format_def_s *defs, const enum texture_logical_channel_e *expect, uint16_t expectCount) {
	const uint32_t numChannels = RT_NumberChannels( defs );
	if( numChannels == 0 )
		return false;
	if( numChannels != expectCount )
		return false;
	const enum texture_logical_channel_e *channels = RT_Channels( defs );
	for( uint_fast16_t c = 0; c < numChannels; c++ )
		if( ( *( c + channels ) ) != ( *( expect + c ) ) )
			return false;

	return true;
}

const struct base_format_def_s* R_BaseFormatDef(enum texture_format_e format) {
  const struct base_format_def_s* def = &baseFormats[format];
  if(def->base == R_BASE_UNKNOWN) {
    return NULL;
  }
  return &baseFormats[format];
}





