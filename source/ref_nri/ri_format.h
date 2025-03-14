#ifndef RI_FORMAT_H
#define RI_FORMAT_H

#include <qtypes.h>
#include "ri_types.h"

enum RI_Format_e {
	RI_FORMAT_UNKNOWN,
	
	// opengl specific 
	RI_FORMAT_L8_A8_UNORM,
	RI_FORMAT_L8_UNORM,
	RI_FORMAT_A8_UNORM,

	RI_FORMAT_D16_UNORM_S8_UINT,
	RI_FORMAT_D24_UNORM_S8_UINT,
	RI_FORMAT_D32_SFLOAT_S8_UINT,
	
	RI_FORMAT_R8_UNORM,
	RI_FORMAT_R8_SNORM,
	RI_FORMAT_R8_UINT,
	RI_FORMAT_R8_SINT,
	RI_FORMAT_RG8_UNORM,
	RI_FORMAT_RG8_SNORM,
	RI_FORMAT_RG8_UINT,
	RI_FORMAT_RG8_SINT,
	RI_FORMAT_BGRA8_UNORM,
	RI_FORMAT_BGRA8_SRGB,
	RI_FORMAT_BGR8_UNORM,
	RI_FORMAT_RGB8_UNORM,
	RI_FORMAT_RGBA8_UNORM,
	RI_FORMAT_RGBA8_SNORM,
	RI_FORMAT_RGBA8_UINT,
	RI_FORMAT_RGBA8_SINT,
	RI_FORMAT_RGBA8_SRGB,
	RI_FORMAT_R16_UNORM,
	RI_FORMAT_R16_SNORM,
	RI_FORMAT_R16_UINT,
	RI_FORMAT_R16_SINT,
	RI_FORMAT_R16_SFLOAT,
	RI_FORMAT_RG16_UNORM,
	RI_FORMAT_RG16_SNORM,
	RI_FORMAT_RG16_UINT,
	RI_FORMAT_RG16_SINT,
	RI_FORMAT_RG16_SFLOAT,
	RI_FORMAT_RGBA16_UNORM,
	RI_FORMAT_RGBA16_SNORM,
	RI_FORMAT_RGBA16_UINT,
	RI_FORMAT_RGBA16_SINT,
	RI_FORMAT_RGBA16_SFLOAT,
	RI_FORMAT_R32_UINT,
	RI_FORMAT_R32_SINT,
	RI_FORMAT_R32_SFLOAT,
	RI_FORMAT_RG32_UINT,
	RI_FORMAT_RG32_SINT,
	RI_FORMAT_RG32_SFLOAT,
	RI_FORMAT_RGB32_UINT,
	RI_FORMAT_RGB32_SINT,
	RI_FORMAT_RGB32_SFLOAT,
	RI_FORMAT_RGBA32_UINT,
	RI_FORMAT_RGBA32_SINT,
	RI_FORMAT_RGBA32_SFLOAT,
	RI_FORMAT_R10_G10_B10_A2_UNORM,
	RI_FORMAT_R10_G10_B10_A2_UINT,
	RI_FORMAT_R11_G11_B10_UFLOAT,
	RI_FORMAT_R9_G9_B9_E5_UNORM,
	RI_FORMAT_R5_G6_B5_UNORM,
	RI_FORMAT_R5_G5_B5_A1_UNORM,
	RI_FORMAT_R4_G4_B4_A4_UNORM,
	RI_FORMAT_BC1_RGBA_UNORM,
	RI_FORMAT_BC1_RGBA_SRGB,
	RI_FORMAT_BC2_RGBA_UNORM,
	RI_FORMAT_BC2_RGBA_SRGB,
	RI_FORMAT_BC3_RGBA_UNORM,
	RI_FORMAT_BC3_RGBA_SRGB,
	RI_FORMAT_BC4_R_UNORM,
	RI_FORMAT_BC4_R_SNORM,
	RI_FORMAT_BC5_RG_UNORM,
	RI_FORMAT_BC5_RG_SNORM,
	RI_FORMAT_BC6H_RGB_UFLOAT,
	RI_FORMAT_BC6H_RGB_SFLOAT,
	RI_FORMAT_BC7_RGBA_UNORM,
	RI_FORMAT_BC7_RGBA_SRGB,
	RI_FORMAT_D16_UNORM,
	RI_FORMAT_D32_SFLOAT,
	RI_FORMAT_D32_SFLOAT_S8_UINT_X24,
	RI_FORMAT_R24_UNORM_X8,
	RI_FORMAT_X24_R8_UINT,
	RI_FORMAT_X32_R8_UINT_X24,
	RI_FORMAT_R32_SFLOAT_X8_X24,

	RI_FORMAT_ETC1_R8G8B8_OES, // R_FORMAT_ETC2_R8G8B8_UNORM is a superset

	RI_FORMAT_ETC2_R8G8B8_UNORM,
	RI_FORMAT_ETC2_R8G8B8_SRGB,
	RI_FORMAT_ETC2_R8G8B8A1_UNORM,
	RI_FORMAT_ETC2_R8G8B8A1_SRGB,
	RI_FORMAT_ETC2_R8G8B8A8_UNORM,
	RI_FORMAT_ETC2_R8G8B8A8_SRGB,
	RI_FORMAT_ETC2_EAC_R11_UNORM,
	RI_FORMAT_ETC2_EAC_R11_SNORM,
	RI_FORMAT_ETC2_EAC_R11G11_UNORM,
	RI_FORMAT_ETC2_EAC_R11G11_SNORM,

	RI_TEXTURE_FORMAT_COUNT
};
typedef uint16_t RI_Format;

enum RI_LogicalChannel_e {
	RI_LOGICAL_C_RED = 0,
	RI_LOGICAL_C_GREEN = 1,
	RI_LOGICAL_C_BLUE = 2,
	RI_LOGICAL_C_ALPHA = 3,
	RI_LOGICAL_C_DEPTH = 0,
	RI_LOGICAL_C_STENCIL = 1,
	RI_LOGICAL_C_LUMINANCE = 0,
	RI_LOGICAL_C_MAX = 6
};

struct RIFormatProps_s {
	const char *name;		       // format name
	uint32_t format;		       // self
	union {
	  uint8_t redBits;	         // R bits
	  uint8_t depthBits;	       // D bits
	};
	union {
	  uint8_t greenBits;	       // G (or stencil) bits (0 if channels < 2)
	  uint8_t stencilBits;	       // G (or stencil) bits (0 if channels < 2)
	};
	uint8_t blueBits;	         // B bits (0 if channels < 3)
	uint8_t alphaBits;	       // A (or shared exponent) bits (0 if channels < 4)
	uint8_t luminanceBits;     // L luminance
	uint32_t stride : 6;	     // block size in bytes
	uint32_t blockWidth : 4;   // 1 for plain formats, >1 for compressed
	uint32_t blockHeight : 4;  // 1 for plain formats, >1 for compressed
	uint32_t isBgr : 1;		     // reversed channels (RGBA => BGRA)
	uint32_t isCompressed : 1; // block-compressed format
	uint32_t isDepth : 1;	     // has depth component
	uint32_t isExpShared : 1;  // shared exponent in alpha channel
	uint32_t isFloat : 1;	     // floating point
	uint32_t isPacked : 1;	   // 16- or 32- bit packed
	uint32_t isInteger : 1;	   // integer
	uint32_t isNorm : 1;	     // [0; 1] normalized
	uint32_t isSigned : 1;	   // signed
	uint32_t isSrgb : 1;	     // sRGB
	uint32_t isStencil : 1;	   // has stencil component
};

const struct RIFormatProps_s* GetRIFormatProps(uint32_t format);

#if DEVICE_IMPL_VULKAN 
const VkFormat RIFormatToVK(uint32_t format);
const enum RI_Format_e VKToRIFormat(VkFormat);  
#endif

#endif
