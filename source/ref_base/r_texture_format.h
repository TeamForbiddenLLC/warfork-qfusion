#ifndef R_TEXFORMAT_H
#define R_TEXFORMAT_H

#include "../gameshared/q_arch.h"

enum texture_logical_channel_e {
	R_LOGICAL_C_RED = 0,
	R_LOGICAL_C_GREEN = 1,
	R_LOGICAL_C_BLUE = 2,
	R_LOGICAL_C_ALPHA = 3,
	R_LOGICAL_C_DEPTH = 0,
	R_LOGICAL_C_STENCIL = 1,
	R_LOGICAL_C_LUMINANCE = 0,
	R_LOGICAL_C_MAX = 6
};

enum texture_base_format_e {
	R_BASE_UNKNOWN, 
	R_BASE_BLOCKED_COMPRESSED,
	R_BASE_FORMAT_FIXED_8,
	R_BASE_FORMAT_FIXED_16,
	R_BASE_FORMAT_FIXED_32,
	R_BASE_FORMAT_PACKED_16
};

enum texture_format_e {
	R_FORMAT_UNKNOWN,
	
	// opengl specific 
	R_FORMAT_L8_A8_UNORM,
	R_FORMAT_L8_UNORM,
	R_FORMAT_A8_UNORM,

	R_FORMAT_D16_UNORM_S8_UINT,
	R_FORMAT_D24_UNORM_S8_UINT,
	R_FORMAT_D32_SFLOAT_S8_UINT,
	
	R_FORMAT_R8_UNORM,
	R_FORMAT_R8_SNORM,
	R_FORMAT_R8_UINT,
	R_FORMAT_R8_SINT,
	R_FORMAT_RG8_UNORM,
	R_FORMAT_RG8_SNORM,
	R_FORMAT_RG8_UINT,
	R_FORMAT_RG8_SINT,
	R_FORMAT_BGRA8_UNORM,
	R_FORMAT_BGRA8_SRGB,
	R_FORMAT_BGR8_UNORM,
	R_FORMAT_RGB8_UNORM,
	R_FORMAT_RGBA8_UNORM,
	R_FORMAT_RGBA8_SNORM,
	R_FORMAT_RGBA8_UINT,
	R_FORMAT_RGBA8_SINT,
	R_FORMAT_RGBA8_SRGB,
	R_FORMAT_R16_UNORM,
	R_FORMAT_R16_SNORM,
	R_FORMAT_R16_UINT,
	R_FORMAT_R16_SINT,
	R_FORMAT_R16_SFLOAT,
	R_FORMAT_RG16_UNORM,
	R_FORMAT_RG16_SNORM,
	R_FORMAT_RG16_UINT,
	R_FORMAT_RG16_SINT,
	R_FORMAT_RG16_SFLOAT,
	R_FORMAT_RGBA16_UNORM,
	R_FORMAT_RGBA16_SNORM,
	R_FORMAT_RGBA16_UINT,
	R_FORMAT_RGBA16_SINT,
	R_FORMAT_RGBA16_SFLOAT,
	R_FORMAT_R32_UINT,
	R_FORMAT_R32_SINT,
	R_FORMAT_R32_SFLOAT,
	R_FORMAT_RG32_UINT,
	R_FORMAT_RG32_SINT,
	R_FORMAT_RG32_SFLOAT,
	R_FORMAT_RGB32_UINT,
	R_FORMAT_RGB32_SINT,
	R_FORMAT_RGB32_SFLOAT,
	R_FORMAT_RGBA32_UINT,
	R_FORMAT_RGBA32_SINT,
	R_FORMAT_RGBA32_SFLOAT,
	R_FORMAT_R10_G10_B10_A2_UNORM,
	R_FORMAT_R10_G10_B10_A2_UINT,
	R_FORMAT_R11_G11_B10_UFLOAT,
	R_FORMAT_R9_G9_B9_E5_UNORM,
	R_FORMAT_R5_G6_B5_UNORM,
	R_FORMAT_R5_G5_B5_A1_UNORM,
	R_FORMAT_R4_G4_B4_A4_UNORM,
	R_FORMAT_BC1_RGBA_UNORM,
	R_FORMAT_BC1_RGBA_SRGB,
	R_FORMAT_BC2_RGBA_UNORM,
	R_FORMAT_BC2_RGBA_SRGB,
	R_FORMAT_BC3_RGBA_UNORM,
	R_FORMAT_BC3_RGBA_SRGB,
	R_FORMAT_BC4_R_UNORM,
	R_FORMAT_BC4_R_SNORM,
	R_FORMAT_BC5_RG_UNORM,
	R_FORMAT_BC5_RG_SNORM,
	R_FORMAT_BC6H_RGB_UFLOAT,
	R_FORMAT_BC6H_RGB_SFLOAT,
	R_FORMAT_BC7_RGBA_UNORM,
	R_FORMAT_BC7_RGBA_SRGB,
	R_FORMAT_D16_UNORM,
	R_FORMAT_D32_SFLOAT,
	R_FORMAT_D32_SFLOAT_S8_UINT_X24,
	R_FORMAT_R24_UNORM_X8,
	R_FORMAT_X24_R8_UINT,
	R_FORMAT_X32_R8_UINT_X24,
	R_FORMAT_R32_SFLOAT_X8_X24,

	R_FORMAT_ETC1_R8G8B8_OES, // R_FORMAT_ETC2_R8G8B8_UNORM is a superset

	R_FORMAT_ETC2_R8G8B8_UNORM,
	R_FORMAT_ETC2_R8G8B8_SRGB,
	R_FORMAT_ETC2_R8G8B8A1_UNORM,
	R_FORMAT_ETC2_R8G8B8A1_SRGB,
	R_FORMAT_ETC2_R8G8B8A8_UNORM,
	R_FORMAT_ETC2_R8G8B8A8_SRGB,
	R_FORMAT_ETC2_EAC_R11_UNORM,
	R_FORMAT_ETC2_EAC_R11_SNORM,
	R_FORMAT_ETC2_EAC_R11G11_UNORM,
	R_FORMAT_ETC2_EAC_R11G11_SNORM,

	R_TEXTURE_FORMAT_COUNT
};

struct texture_format_def_fixed_8_s {
	int_fast16_t numChannels;
	enum texture_logical_channel_e channels[R_LOGICAL_C_MAX];
  uint32_t normalized: 1;
  uint32_t sign: 1;
  uint32_t srgb: 1;
  uint32_t isInteger: 1;
};

struct texture_format_packed_def_16_s {
	int_fast16_t numChannels;
	struct {
		size_t offset;
		size_t mask;
	} bits[R_LOGICAL_C_MAX];
	enum texture_logical_channel_e channels[R_LOGICAL_C_MAX];
};

struct texture_format_fixed_def_16_s {
	int_fast16_t numChannels;
	enum texture_logical_channel_e channels[R_LOGICAL_C_MAX];
  uint32_t normalized: 1;
  uint32_t sign: 1;
  uint32_t srgb: 1;
  uint32_t isInteger: 1;
};

struct texture_format_fixed_def_32_s {
	int_fast16_t numChannels;
	enum texture_logical_channel_e channels[R_LOGICAL_C_MAX];
  uint32_t normalized: 1;
  uint32_t sign: 1;
  uint32_t srgb: 1;
  uint32_t isInteger: 1;
};

struct texture_format_def_compressed_s {
	uint8_t blockWidth;
	uint8_t blockHeight;
  int_fast16_t blockByteSize;
};

struct base_format_def_s {
	enum texture_format_e format;  
	enum texture_base_format_e base; 
	union {
		struct texture_format_def_compressed_s compressed;
		struct texture_format_def_fixed_8_s fixed_8;
		struct texture_format_fixed_def_16_s fixed_16;
		struct texture_format_fixed_def_32_s fixed_32;
	  struct texture_format_packed_def_16_s packed_16; 
	};
};

const struct base_format_def_s* R_BaseFormatDef(enum texture_format_e format);
const size_t RT_BlockSize(const struct base_format_def_s* defs);
const uint_fast16_t RT_NumberChannels(const struct base_format_def_s* defs);
const enum texture_logical_channel_e* RT_Channels(const struct base_format_def_s* defs);
const bool RT_ExpectChannelsMatch( const struct base_format_def_s *defs, const enum texture_logical_channel_e *channels, uint16_t numChannels );

bool R_FormatIsCompressed(enum texture_format_e format);
uint32_t R_FormatBlockWidth( enum texture_format_e format );
uint32_t R_FormatBlockHeight( enum texture_format_e format );
uint32_t R_FormatChannelCount( enum texture_format_e format );
uint32_t R_FormatBitSizePerBlock( enum texture_format_e format );
uint32_t R_FormatChannelBitWidth( enum texture_format_e format, uint16_t channel );
#endif

