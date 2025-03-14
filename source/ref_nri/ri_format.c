#include "ri_format.h"
#include "../gameshared/q_shared.h"

static const struct RIFormatProps_s riFormats[] = {
	[RI_FORMAT_UNKNOWN] = { 
		.name = "UNKNOWN", 
		.format = RI_FORMAT_UNKNOWN 
	},

	[RI_FORMAT_L8_A8_UNORM] = { 
	  .name = "L8_A8_UNORM", 
	  .format = RI_FORMAT_L8_A8_UNORM,
	  .alphaBits = 8,
	  .luminanceBits = 8,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_L8_UNORM] = {
	  .name = "L8_UNORM",
	  .luminanceBits = 8,
	  .stride = 1,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_A8_UNORM] = {
	  .name = "A8_UNORM",
	  .alphaBits = 8,
	  .stride = 1,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},

	[RI_FORMAT_D16_UNORM_S8_UINT] = {
	  .name = "D16_UNORM_S8_UINT",
	  .depthBits = 16,
	  .stencilBits = 8,
	  .stride = 3,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isDepth = 1,
	  .isStencil = 1,
	},
	[RI_FORMAT_D24_UNORM_S8_UINT] = {
	  .name = "D24_UNORM_S8_UINT",
	  .depthBits = 24,
	  .stencilBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isDepth = 1,
	  .isStencil = 1,
	},
	[RI_FORMAT_D32_SFLOAT_S8_UINT] = {
	  .name = "D32_SFLOAT_S8_UINT",
	  .depthBits = 24,
	  .stencilBits = 8,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isDepth = 1,
	  .isStencil = 1,
	  .isFloat = 1
	},
	[RI_FORMAT_R8_UNORM] = {
	  .name = "R8_UNORM",
	  .redBits = 8,
	  .stride = 1,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_R8_SNORM] = {
	  .name = "R8_SNORM",
	  .redBits = 8,
	  .stride = 1,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isSigned = 1
	},
	[RI_FORMAT_R8_UINT] = {
	  .name = "R8_UINT",
	  .redBits = 8,
	  .stride = 1,
	  .blockWidth = 1,
	  .blockHeight = 1,
	},
	[RI_FORMAT_R8_SINT] = {
	  .name = "R8_SINT",
	  .redBits = 8,
	  .stride = 1,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_RG8_UNORM] = {
	  .name = "RG8_UNORM",
	  .redBits = 8,
	  .greenBits = 8,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_RG8_SNORM] = {
	  .name = "RG8_SNORM",
	  .redBits = 8,
	  .greenBits = 8,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_RG8_UINT] = {
	  .name = "RG8_UINT",
	  .redBits = 8,
	  .greenBits = 8,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	},
	[RI_FORMAT_RG8_SINT] = {
	  .name = "RG8_SINT",
	  .redBits = 8,
	  .greenBits = 8,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_BGRA8_UNORM] = {
	  .name = "BGRA8_UNORM",
	  .blueBits = 8,
	  .greenBits = 8,
	  .redBits = 8,
	  .alphaBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isBgr = 1,
	},
	[RI_FORMAT_BGRA8_SRGB] = {
	  .name = "BGRA8_SRGB",
	  .blueBits = 8,
	  .greenBits = 8,
	  .redBits = 8,
	  .alphaBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isSrgb = 1,
	  .isBgr = 1,
	},
	[RI_FORMAT_BGR8_UNORM] = {
	  .name = "BGR8_UNORM",
	  .blueBits = 8,
	  .greenBits = 8,
	  .redBits = 8,
	  .stride = 3,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isBgr = 1,
	},
	[RI_FORMAT_RGB8_UNORM] = {
	  .name = "RGB8_UNORM",
	  .redBits = 8,
	  .greenBits = 8,
	  .blueBits = 8,
	  .stride = 3,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_RGBA8_UNORM] = {
	  .name = "RGBA8_UNORM",
	  .redBits = 8,
	  .greenBits = 8,
	  .blueBits = 8,
	  .alphaBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_RGBA8_SNORM] = {
	  .name = "RGBA8_SNORM",
	  .redBits = 8,
	  .greenBits = 8,
	  .blueBits = 8,
	  .alphaBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_RGBA8_UINT] = {
	  .name = "RGBA8_UINT",
	  .redBits = 8,
	  .greenBits = 8,
	  .blueBits = 8,
	  .alphaBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_RGBA8_SINT] = {
	  .name = "RGBA8_SINT",
	  .redBits = 8,
	  .greenBits = 8,
	  .blueBits = 8,
	  .alphaBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_RGBA8_SRGB] = {
	  .name = "RGBA8_SRGB",
	  .redBits = 8,
	  .greenBits = 8,
	  .blueBits = 8,
	  .alphaBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_R16_UNORM] = {
	  .name = "R16_UNORM",
	  .redBits = 16,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_R16_SNORM] = {
	  .name = "R16_SNORM",
	  .redBits = 16,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_R16_UINT] = {
	  .name = "R16_UINT",
	  .redBits = 16,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .stride = 2,
	},
	[RI_FORMAT_R16_SINT] = {
	  .name = "R16_SINT",
	  .redBits = 16,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_R16_SFLOAT] = {
	  .name = "R16_SFLOAT",
	  .redBits = 16,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_RG16_UNORM] = {
	  .name = "RG16_UNORM",
	  .redBits = 16,
	  .greenBits = 16,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_RG16_SNORM] = {
	  .name = "RG16_SNORM",
	  .redBits = 16,
	  .greenBits = 16,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_RG16_UINT] = {
	  .name = "RG16_UINT",
	  .redBits = 16,
	  .greenBits = 16,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	},
	[RI_FORMAT_RG16_SINT] = {
	  .name = "RG16_SINT",
	  .redBits = 16,
	  .greenBits = 16,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_RG16_SFLOAT] = {
	  .name = "RG16_SFLOAT",
	  .redBits = 16,
	  .greenBits = 16,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_RGBA16_UNORM] = {
	  .name = "RGBA16_UNORM",
	  .redBits = 16,
	  .greenBits = 16,
	  .blueBits = 16,
	  .alphaBits = 16,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_RGBA16_SNORM] = {
	  .name = "RGBA16_SNORM",
	  .redBits = 16,
	  .greenBits = 16,
	  .blueBits = 16,
	  .alphaBits = 16,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_RGBA16_UINT] = {
	  .name = "RGBA16_UINT",
	  .redBits = 16,
	  .greenBits = 16,
	  .blueBits = 16,
	  .alphaBits = 16,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	},
	[RI_FORMAT_RGBA16_SINT] = {
	  .name = "RGBA16_SINT",
	  .redBits = 16,
	  .greenBits = 16,
	  .blueBits = 16,
	  .alphaBits = 16,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_RGBA16_SFLOAT] = {
	  .name = "RGBA16_SFLOAT",
	  .redBits = 16,
	  .greenBits = 16,
	  .blueBits = 16,
	  .alphaBits = 16,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_R32_UINT] = {
	  .name = "R32_UINT",
	  .redBits = 32,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .stride = 4,
	},
	[RI_FORMAT_R32_SINT] = {
	  .name = "R32_SINT",
	  .redBits = 32,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_R32_SFLOAT] = {
	  .name = "R32_SFLOAT",
	  .redBits = 32,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_RG32_UINT] = {
	  .name = "RG32_UINT",
	  .redBits = 32,
	  .greenBits = 32,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .stride = 8,
	},
	[RI_FORMAT_RG32_SINT] = {
	  .name = "RG32_SINT",
	  .redBits = 32,
	  .greenBits = 32,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_RG32_SFLOAT] = {
	  .name = "RG32_SFLOAT",
	  .redBits = 32,
	  .greenBits = 32,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_RGB32_UINT] = {
	  .name = "RGB32_UINT",
	  .redBits = 32,
	  .greenBits = 32,
	  .blueBits = 32,
	  .stride = 12,
	  .blockWidth = 1,
	  .blockHeight = 1,
	},
	[RI_FORMAT_RGB32_SINT] = {
	  .name = "RGB32_SINT",
	  .redBits = 32,
	  .greenBits = 32,
	  .blueBits = 32,
	  .stride = 12,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_RGB32_SFLOAT] = {
	  .name = "RGB32_SFLOAT",
	  .redBits = 32,
	  .greenBits = 32,
	  .blueBits = 32,
	  .stride = 12,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_RGBA32_UINT] = {
	  .name = "RGBA32_UINT",
	  .redBits = 32,
	  .greenBits = 32,
	  .blueBits = 32,
	  .alphaBits = 32,
	  .stride = 16,
	  .blockWidth = 1,
	  .blockHeight = 1,
	},
	[RI_FORMAT_RGBA32_SINT] = {
	  .name = "RGBA32_SINT",
	  .redBits = 32,
	  .greenBits = 32,
	  .blueBits = 32,
	  .alphaBits = 32,
	  .stride = 16,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isInteger = 1,
	},
	[RI_FORMAT_RGBA32_SFLOAT] = {
	  .name = "RGBA32_SFLOAT",
	  .redBits = 32,
	  .greenBits = 32,
	  .blueBits = 32,
	  .alphaBits = 32,
	  .stride = 16,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_R10_G10_B10_A2_UNORM] = {
	  .name = "R10_G10_B10_A2_UNORM",
	  .redBits = 10,
	  .greenBits = 10,
	  .blueBits = 10,
	  .alphaBits = 2,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_R10_G10_B10_A2_UINT] = {
	  .name = "R10_G10_B10_A2_UINT",
	  .redBits = 10,
	  .greenBits = 10,
	  .blueBits = 10,
	  .alphaBits = 2,
	  .stride = 4,
	},
	[RI_FORMAT_R11_G11_B10_UFLOAT] = {
	  .name = "R11_G11_B10_UFLOAT",
	  .redBits = 11,
	  .greenBits = 11,
	  .blueBits = 10,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_R9_G9_B9_E5_UNORM] = {
	  .name = "R9_G9_B9_E5_UNORM",
	  .redBits = 9,
	  .greenBits = 9,
	  .blueBits = 9,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_R5_G6_B5_UNORM] = {
	  .name = "R5_G6_B5_UNORM",
	  .redBits = 5,
	  .greenBits = 6,
	  .blueBits = 5,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_R5_G5_B5_A1_UNORM] = {
	  .name = "R5_G5_B5_A1_UNORM",
	  .redBits = 5,
	  .greenBits = 5,
	  .blueBits = 5,
	  .alphaBits = 1,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_R4_G4_B4_A4_UNORM] = {
	  .name = "R4_G4_B4_A4_UNORM",
	  .redBits = 4,
	  .greenBits = 4,
	  .blueBits = 4,
	  .alphaBits = 4,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_BC1_RGBA_UNORM] = {
	  .name = "BC1_RGBA_UNORM",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_BC1_RGBA_SRGB] = {
	  .name = "BC1_RGBA_SRGB",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_BC2_RGBA_UNORM] = {
	  .name = "BC2_RGBA_UNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_BC2_RGBA_SRGB] = {
	  .name = "BC2_RGBA_SRGB",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_BC3_RGBA_UNORM] = {
	  .name = "BC3_RGBA_UNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_BC3_RGBA_SRGB] = {
	  .name = "BC3_RGBA_SRGB",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_BC4_R_UNORM] = {
	  .name = "BC4_R_UNORM",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_BC4_R_SNORM] = {
	  .name = "BC4_R_SNORM",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_BC5_RG_UNORM] = {
	  .name = "BC5_RG_UNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_BC5_RG_SNORM] = {
	  .name = "BC5_RG_SNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_BC6H_RGB_UFLOAT] = {
	  .name = "BC6H_RGB_UFLOAT",
	  .redBits = 16,
	  .greenBits = 16,
	  .blueBits = 16,
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_BC6H_RGB_SFLOAT] = {
	  .name = "BC6H_RGB_SFLOAT",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_BC7_RGBA_UNORM] = {
	  .name = "BC7_RGBA_UNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_BC7_RGBA_SRGB] = {
	  .name = "BC7_RGBA_SRGB",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_D16_UNORM] = {
	  .name = "D16_UNORM",
	  .depthBits = 16,
	  .stride = 2,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	  .isDepth = 1,
	},
	[RI_FORMAT_D32_SFLOAT] = {
	  .name = "D32_SFLOAT",
	  .depthBits = 32,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isDepth = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_D32_SFLOAT_S8_UINT_X24] = {
	  .name = "D32_SFLOAT_S8_UINT_X24",
	  .depthBits = 32,
	  .stencilBits = 8,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isDepth = 1,
	  .isStencil = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_R24_UNORM_X8] = {
	  .name = "R24_UNORM_X8",
	  .redBits = 24,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_X24_R8_UINT] = {
	  .name = "X24_R8_UINT",
	  .redBits = 8,
	  .stride = 4,
	  .blockWidth = 1,
	  .blockHeight = 1,
	},
	[RI_FORMAT_X32_R8_UINT_X24] = {
	  .name = "X32_R8_UINT_X24",
	  .redBits = 8,
	  .stride = 8,
	},
	[RI_FORMAT_R32_SFLOAT_X8_X24] = {
	  .name = "R32_SFLOAT_X8_X24",
	  .redBits = 32,
	  .stride = 8,
	  .blockWidth = 1,
	  .blockHeight = 1,
	  .isFloat = 1,
	},
	[RI_FORMAT_ETC1_R8G8B8_OES] = {
	  .name = "ETC1_R8G8B8_OES",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	},
	[RI_FORMAT_ETC2_R8G8B8_UNORM] = {
	  .name = "ETC2_R8G8B8_UNORM",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_ETC2_R8G8B8_SRGB] = {
	  .name = "ETC2_R8G8B8_SRGB",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_ETC2_R8G8B8A1_UNORM] = {
	  .name = "ETC2_R8G8B8A1_UNORM",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_ETC2_R8G8B8A1_SRGB] = {
	  .name = "ETC2_R8G8B8A1_SRGB",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_ETC2_R8G8B8A8_UNORM] = {
	  .name = "ETC2_R8G8B8A8_UNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_ETC2_R8G8B8A8_SRGB] = {
	  .name = "ETC2_R8G8B8A8_SRGB",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isSrgb = 1,
	},
	[RI_FORMAT_ETC2_EAC_R11_UNORM] = {
	  .name = "ETC2_EAC_R11_UNORM",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_ETC2_EAC_R11_SNORM] = {
	  .name = "ETC2_EAC_R11_SNORM",
	  .stride = 8,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_FORMAT_ETC2_EAC_R11G11_UNORM] = {
	  .name = "ETC2_EAC_R11G11_UNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	},
	[RI_FORMAT_ETC2_EAC_R11G11_SNORM] = {
	  .name = "ETC2_EAC_R11G11_SNORM",
	  .stride = 16,
	  .blockWidth = 4,
	  .blockHeight = 4,
	  .isCompressed = 1,
	  .isNorm = 1,
	  .isSigned = 1,
	},
	[RI_TEXTURE_FORMAT_COUNT] = {},

};

const struct RIFormatProps_s *GetRIFormatProps( uint32_t format ) {
  assert( format < RI_TEXTURE_FORMAT_COUNT );
  return riFormats + format;
}


#if DEVICE_IMPL_VULKAN 
const VkFormat RIFormatToVK(uint32_t format) {
	switch (format) {
		case RI_FORMAT_UNKNOWN: return VK_FORMAT_UNDEFINED;
		case RI_FORMAT_L8_A8_UNORM: return VK_FORMAT_R8G8_UNORM;
		case RI_FORMAT_L8_UNORM: return VK_FORMAT_R8_UNORM;
		case RI_FORMAT_A8_UNORM: return VK_FORMAT_R8_UNORM;
		case RI_FORMAT_D16_UNORM_S8_UINT: return VK_FORMAT_D16_UNORM_S8_UINT;
		case RI_FORMAT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
		case RI_FORMAT_D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case RI_FORMAT_R8_UNORM: return VK_FORMAT_R8_UNORM;
		case RI_FORMAT_R8_SNORM: return VK_FORMAT_R8_SNORM;
		case RI_FORMAT_R8_UINT: return VK_FORMAT_R8_UINT;
		case RI_FORMAT_R8_SINT: return VK_FORMAT_R8_SINT;
		case RI_FORMAT_RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
		case RI_FORMAT_RG8_SNORM: return VK_FORMAT_R8G8_SNORM;
		case RI_FORMAT_RG8_UINT: return VK_FORMAT_R8G8_UINT;
		case RI_FORMAT_RG8_SINT: return VK_FORMAT_R8G8_SINT;
		case RI_FORMAT_BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
		case RI_FORMAT_BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case RI_FORMAT_BGR8_UNORM: return VK_FORMAT_B8G8R8_UNORM;
		case RI_FORMAT_RGB8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
		case RI_FORMAT_RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case RI_FORMAT_RGBA8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
		case RI_FORMAT_RGBA8_UINT: return VK_FORMAT_R8G8B8A8_UINT;
		case RI_FORMAT_RGBA8_SINT: return VK_FORMAT_R8G8B8A8_SINT;
		case RI_FORMAT_RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case RI_FORMAT_R16_UNORM: return VK_FORMAT_R16_UNORM;
		case RI_FORMAT_R16_SNORM: return VK_FORMAT_R16_SNORM;
		case RI_FORMAT_R16_UINT: return VK_FORMAT_R16_UINT;
		case RI_FORMAT_R16_SINT: return VK_FORMAT_R16_SINT;
		case RI_FORMAT_R16_SFLOAT: return VK_FORMAT_R16_SFLOAT;
		case RI_FORMAT_RG16_UNORM: return VK_FORMAT_R16G16_UNORM;
		case RI_FORMAT_RG16_SNORM: return VK_FORMAT_R16G16_SNORM;
		case RI_FORMAT_RG16_UINT: return VK_FORMAT_R16G16_UINT;
		case RI_FORMAT_RG16_SINT: return VK_FORMAT_R16G16_SINT;
		case RI_FORMAT_RG16_SFLOAT: return VK_FORMAT_R16G16_SFLOAT;
		case RI_FORMAT_RGBA16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
		case RI_FORMAT_RGBA16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM;
		case RI_FORMAT_RGBA16_UINT: return VK_FORMAT_R16G16B16A16_UINT;
		case RI_FORMAT_RGBA16_SINT: return VK_FORMAT_R16G16B16A16_SINT;
		case RI_FORMAT_RGBA16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case RI_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;
		case RI_FORMAT_R32_SINT: return VK_FORMAT_R32_SINT;
		case RI_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
		case RI_FORMAT_RG32_UINT: return VK_FORMAT_R32G32_UINT;
		case RI_FORMAT_RG32_SINT: return VK_FORMAT_R32G32_SINT;
		case RI_FORMAT_RG32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
		case RI_FORMAT_RGB32_UINT: return VK_FORMAT_R32G32B32_UINT;
		case RI_FORMAT_RGB32_SINT: return VK_FORMAT_R32G32B32_SINT;
		case RI_FORMAT_RGB32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
		case RI_FORMAT_RGBA32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
		case RI_FORMAT_RGBA32_SINT: return VK_FORMAT_R32G32B32A32_SINT;
		case RI_FORMAT_RGBA32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case RI_FORMAT_R10_G10_B10_A2_UNORM: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case RI_FORMAT_R10_G10_B10_A2_UINT: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case RI_FORMAT_R11_G11_B10_UFLOAT: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case RI_FORMAT_R9_G9_B9_E5_UNORM: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case RI_FORMAT_R5_G6_B5_UNORM: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case RI_FORMAT_R5_G5_B5_A1_UNORM: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case RI_FORMAT_R4_G4_B4_A4_UNORM: return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
		case RI_FORMAT_BC1_RGBA_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case RI_FORMAT_BC1_RGBA_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case RI_FORMAT_BC2_RGBA_UNORM: return VK_FORMAT_BC2_UNORM_BLOCK;
		case RI_FORMAT_BC2_RGBA_SRGB: return VK_FORMAT_BC2_SRGB_BLOCK;
		case RI_FORMAT_BC3_RGBA_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK;
		case RI_FORMAT_BC3_RGBA_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;
		case RI_FORMAT_BC4_R_UNORM: return VK_FORMAT_BC4_UNORM_BLOCK;
		case RI_FORMAT_BC4_R_SNORM: return VK_FORMAT_BC4_SNORM_BLOCK;
		case RI_FORMAT_BC5_RG_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK;
		case RI_FORMAT_BC5_RG_SNORM: return VK_FORMAT_BC5_SNORM_BLOCK;
		case RI_FORMAT_BC6H_RGB_UFLOAT: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case RI_FORMAT_BC6H_RGB_SFLOAT: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case RI_FORMAT_BC7_RGBA_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK;
		case RI_FORMAT_BC7_RGBA_SRGB: return VK_FORMAT_BC7_SRGB_BLOCK;
		case RI_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
		case RI_FORMAT_D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
		case RI_FORMAT_D32_SFLOAT_S8_UINT_X24: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case RI_FORMAT_R24_UNORM_X8: return VK_FORMAT_X8_D24_UNORM_PACK32;
		case RI_FORMAT_X24_R8_UINT: return VK_FORMAT_X8_D24_UNORM_PACK32;
		case RI_FORMAT_X32_R8_UINT_X24: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case RI_FORMAT_R32_SFLOAT_X8_X24: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case RI_FORMAT_ETC1_R8G8B8_OES: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		case RI_FORMAT_ETC2_R8G8B8_UNORM: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		case RI_FORMAT_ETC2_R8G8B8_SRGB: return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
		case RI_FORMAT_ETC2_R8G8B8A1_UNORM: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
		case RI_FORMAT_ETC2_R8G8B8A1_SRGB: return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
		case RI_FORMAT_ETC2_R8G8B8A8_UNORM: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		case RI_FORMAT_ETC2_R8G8B8A8_SRGB: return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
		case RI_FORMAT_ETC2_EAC_R11_UNORM: return VK_FORMAT_EAC_R11_UNORM_BLOCK;
		case RI_FORMAT_ETC2_EAC_R11_SNORM: return VK_FORMAT_EAC_R11_SNORM_BLOCK;
		case RI_FORMAT_ETC2_EAC_R11G11_UNORM: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
		case RI_FORMAT_ETC2_EAC_R11G11_SNORM: return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
		default: {
			const struct RIFormatProps_s* fmt =  GetRIFormatProps(format);
			Com_Printf("Unhandled RI_Format to VK %d\n", fmt->name);
			break;
		}
	}
	return VK_FORMAT_UNDEFINED;
}


const enum RI_Format_e VKToRIFormat(VkFormat format) {
switch (format) {
		case VK_FORMAT_UNDEFINED: return  RI_FORMAT_UNKNOWN;
		case VK_FORMAT_R8G8_UNORM: return  RI_FORMAT_L8_A8_UNORM;
		case VK_FORMAT_R8_UNORM: return  RI_FORMAT_A8_UNORM;
		case VK_FORMAT_D16_UNORM_S8_UINT: return  RI_FORMAT_D16_UNORM_S8_UINT;
		case VK_FORMAT_D24_UNORM_S8_UINT: return  RI_FORMAT_D24_UNORM_S8_UINT;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return  RI_FORMAT_D32_SFLOAT_S8_UINT;
		case VK_FORMAT_R8_SNORM: return  RI_FORMAT_R8_SNORM;
		case VK_FORMAT_R8_UINT: return  RI_FORMAT_R8_UINT;
		case VK_FORMAT_R8_SINT: return  RI_FORMAT_R8_SINT;
		case VK_FORMAT_R8G8_SNORM: return  RI_FORMAT_RG8_SNORM;
		case VK_FORMAT_R8G8_UINT: return  RI_FORMAT_RG8_UINT;
		case VK_FORMAT_R8G8_SINT: return  RI_FORMAT_RG8_SINT;
		case VK_FORMAT_B8G8R8A8_UNORM: return  RI_FORMAT_BGRA8_UNORM;
		case VK_FORMAT_B8G8R8A8_SRGB: return  RI_FORMAT_BGRA8_SRGB;
		case VK_FORMAT_B8G8R8_UNORM: return  RI_FORMAT_BGR8_UNORM;
		case VK_FORMAT_R8G8B8_UNORM: return  RI_FORMAT_RGB8_UNORM;
		case VK_FORMAT_R8G8B8A8_UNORM: return  RI_FORMAT_RGBA8_UNORM;
		case VK_FORMAT_R8G8B8A8_SNORM: return  RI_FORMAT_RGBA8_SNORM;
		case VK_FORMAT_R8G8B8A8_UINT: return  RI_FORMAT_RGBA8_UINT;
		case VK_FORMAT_R8G8B8A8_SINT: return  RI_FORMAT_RGBA8_SINT;
		case VK_FORMAT_R8G8B8A8_SRGB: return  RI_FORMAT_RGBA8_SRGB;
		case VK_FORMAT_R16_UNORM: return  RI_FORMAT_R16_UNORM;
		case VK_FORMAT_R16_SNORM: return  RI_FORMAT_R16_SNORM;
		case VK_FORMAT_R16_UINT: return  RI_FORMAT_R16_UINT;
		case VK_FORMAT_R16_SINT: return  RI_FORMAT_R16_SINT;
		case VK_FORMAT_R16_SFLOAT: return  RI_FORMAT_R16_SFLOAT;
		case VK_FORMAT_R16G16_UNORM: return  RI_FORMAT_RG16_UNORM;
		case VK_FORMAT_R16G16_SNORM: return  RI_FORMAT_RG16_SNORM;
		case VK_FORMAT_R16G16_UINT: return  RI_FORMAT_RG16_UINT;
		case VK_FORMAT_R16G16_SINT: return  RI_FORMAT_RG16_SINT;
		case VK_FORMAT_R16G16_SFLOAT: return  RI_FORMAT_RG16_SFLOAT;
		case VK_FORMAT_R16G16B16A16_UNORM: return  RI_FORMAT_RGBA16_UNORM;
		case VK_FORMAT_R16G16B16A16_SNORM: return  RI_FORMAT_RGBA16_SNORM;
		case VK_FORMAT_R16G16B16A16_UINT: return  RI_FORMAT_RGBA16_UINT;
		case VK_FORMAT_R16G16B16A16_SINT: return  RI_FORMAT_RGBA16_SINT;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return  RI_FORMAT_RGBA16_SFLOAT;
		case VK_FORMAT_R32_UINT: return  RI_FORMAT_R32_UINT;
		case VK_FORMAT_R32_SINT: return  RI_FORMAT_R32_SINT;
		case VK_FORMAT_R32_SFLOAT: return  RI_FORMAT_R32_SFLOAT;
		case VK_FORMAT_R32G32_UINT: return  RI_FORMAT_RG32_UINT;
		case VK_FORMAT_R32G32_SINT: return  RI_FORMAT_RG32_SINT;
		case VK_FORMAT_R32G32_SFLOAT: return  RI_FORMAT_RG32_SFLOAT;
		case VK_FORMAT_R32G32B32_UINT: return  RI_FORMAT_RGB32_UINT;
		case VK_FORMAT_R32G32B32_SINT: return  RI_FORMAT_RGB32_SINT;
		case VK_FORMAT_R32G32B32_SFLOAT: return  RI_FORMAT_RGB32_SFLOAT;
		case VK_FORMAT_R32G32B32A32_UINT: return  RI_FORMAT_RGBA32_UINT;
		case VK_FORMAT_R32G32B32A32_SINT: return  RI_FORMAT_RGBA32_SINT;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return  RI_FORMAT_RGBA32_SFLOAT;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return  RI_FORMAT_R10_G10_B10_A2_UNORM;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32: return  RI_FORMAT_R10_G10_B10_A2_UINT;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return  RI_FORMAT_R11_G11_B10_UFLOAT;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return  RI_FORMAT_R9_G9_B9_E5_UNORM;
		case VK_FORMAT_R5G6B5_UNORM_PACK16: return  RI_FORMAT_R5_G6_B5_UNORM;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return  RI_FORMAT_R5_G5_B5_A1_UNORM;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return  RI_FORMAT_R4_G4_B4_A4_UNORM;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return  RI_FORMAT_BC1_RGBA_UNORM;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return  RI_FORMAT_BC1_RGBA_SRGB;
		case VK_FORMAT_BC2_UNORM_BLOCK: return  RI_FORMAT_BC2_RGBA_UNORM;
		case VK_FORMAT_BC2_SRGB_BLOCK: return  RI_FORMAT_BC2_RGBA_SRGB;
		case VK_FORMAT_BC3_UNORM_BLOCK: return  RI_FORMAT_BC3_RGBA_UNORM;
		case VK_FORMAT_BC3_SRGB_BLOCK: return  RI_FORMAT_BC3_RGBA_SRGB;
		case VK_FORMAT_BC4_UNORM_BLOCK: return  RI_FORMAT_BC4_R_UNORM;
		case VK_FORMAT_BC4_SNORM_BLOCK: return  RI_FORMAT_BC4_R_SNORM;
		case VK_FORMAT_BC5_UNORM_BLOCK: return  RI_FORMAT_BC5_RG_UNORM;
		case VK_FORMAT_BC5_SNORM_BLOCK: return  RI_FORMAT_BC5_RG_SNORM;
		case VK_FORMAT_BC6H_UFLOAT_BLOCK: return  RI_FORMAT_BC6H_RGB_UFLOAT;
		case VK_FORMAT_BC6H_SFLOAT_BLOCK: return  RI_FORMAT_BC6H_RGB_SFLOAT;
		case VK_FORMAT_BC7_UNORM_BLOCK: return  RI_FORMAT_BC7_RGBA_UNORM;
		case VK_FORMAT_BC7_SRGB_BLOCK: return  RI_FORMAT_BC7_RGBA_SRGB;
		case VK_FORMAT_D16_UNORM: return  RI_FORMAT_D16_UNORM;
		case VK_FORMAT_D32_SFLOAT: return  RI_FORMAT_D32_SFLOAT;
		// case VK_FORMAT_D32_SFLOAT_S8_UINT: return  RI_FORMAT_D32_SFLOAT_S8_UINT_X24;
		case VK_FORMAT_X8_D24_UNORM_PACK32: return  RI_FORMAT_R24_UNORM_X8;
		// case VK_FORMAT_X8_D24_UNORM_PACK32: return  RI_FORMAT_X24_R8_UINT;
		// case VK_FORMAT_D32_SFLOAT_S8_UINT: return  RI_FORMAT_X32_R8_UINT_X24;
		// case VK_FORMAT_D32_SFLOAT_S8_UINT: return  RI_FORMAT_R32_SFLOAT_X8_X24;
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return  RI_FORMAT_ETC1_R8G8B8_OES;
		// case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return  RI_FORMAT_ETC2_R8G8B8_UNORM;
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return  RI_FORMAT_ETC2_R8G8B8_SRGB;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return  RI_FORMAT_ETC2_R8G8B8A1_UNORM;
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return  RI_FORMAT_ETC2_R8G8B8A1_SRGB;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return  RI_FORMAT_ETC2_R8G8B8A8_UNORM;
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return  RI_FORMAT_ETC2_R8G8B8A8_SRGB;
		case VK_FORMAT_EAC_R11_UNORM_BLOCK: return  RI_FORMAT_ETC2_EAC_R11_UNORM;
		case VK_FORMAT_EAC_R11_SNORM_BLOCK: return  RI_FORMAT_ETC2_EAC_R11_SNORM;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return  RI_FORMAT_ETC2_EAC_R11G11_UNORM;
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return  RI_FORMAT_ETC2_EAC_R11G11_SNORM;
		default: {
			const struct RIFormatProps_s* fmt =  GetRIFormatProps(format);
			Com_Printf("Unhandled RI_Format to VK %d\n", fmt->name);
			break;
		}
	}
	return RI_FORMAT_UNKNOWN;
}
#endif
