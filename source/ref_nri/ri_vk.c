#include "ri_vk.h"
#include "../qcommon/qcommon.h"

#if DEVICE_IMPL_VULKAN

// Single source of truth for the RI <-> VK format mapping. FMT rows are bidirectional and define the
// canonical VK->RI reverse mapping; FWD rows are forward-only aliases (RI->VK) whose VkFormat maps
// back to a different RI format via that format's FMT row. Row order matches the historical switch for
// review. The reverse direction is generated from FMT rows only, so an accidental duplicate VkFormat
// on the FMT side is a compile error (duplicate case label) -- a free consistency check.
#define RI_VK_FORMAT_MAP( FMT, FWD )                                                \
	FMT( RI_FORMAT_UNKNOWN,               VK_FORMAT_UNDEFINED )                      \
	FMT( RI_FORMAT_L8_A8_UNORM,           VK_FORMAT_R8G8_UNORM )                     \
	FWD( RI_FORMAT_L8_UNORM,              VK_FORMAT_R8_UNORM )                       \
	FMT( RI_FORMAT_A8_UNORM,              VK_FORMAT_R8_UNORM )                       \
	FMT( RI_FORMAT_D16_UNORM_S8_UINT,     VK_FORMAT_D16_UNORM_S8_UINT )             \
	FMT( RI_FORMAT_D24_UNORM_S8_UINT,     VK_FORMAT_D24_UNORM_S8_UINT )             \
	FMT( RI_FORMAT_D32_SFLOAT_S8_UINT,    VK_FORMAT_D32_SFLOAT_S8_UINT )            \
	FWD( RI_FORMAT_R8_UNORM,              VK_FORMAT_R8_UNORM )                       \
	FMT( RI_FORMAT_R8_SNORM,              VK_FORMAT_R8_SNORM )                       \
	FMT( RI_FORMAT_R8_UINT,               VK_FORMAT_R8_UINT )                        \
	FMT( RI_FORMAT_R8_SINT,               VK_FORMAT_R8_SINT )                        \
	FWD( RI_FORMAT_RG8_UNORM,             VK_FORMAT_R8G8_UNORM )                     \
	FMT( RI_FORMAT_RG8_SNORM,             VK_FORMAT_R8G8_SNORM )                     \
	FMT( RI_FORMAT_RG8_UINT,              VK_FORMAT_R8G8_UINT )                      \
	FMT( RI_FORMAT_RG8_SINT,              VK_FORMAT_R8G8_SINT )                      \
	FMT( RI_FORMAT_BGRA8_UNORM,           VK_FORMAT_B8G8R8A8_UNORM )                 \
	FMT( RI_FORMAT_BGRA8_SRGB,            VK_FORMAT_B8G8R8A8_SRGB )                  \
	FMT( RI_FORMAT_BGR8_UNORM,            VK_FORMAT_B8G8R8_UNORM )                   \
	FMT( RI_FORMAT_RGB8_UNORM,            VK_FORMAT_R8G8B8_UNORM )                   \
	FMT( RI_FORMAT_RGBA8_UNORM,           VK_FORMAT_R8G8B8A8_UNORM )                 \
	FMT( RI_FORMAT_RGBA8_SNORM,           VK_FORMAT_R8G8B8A8_SNORM )                 \
	FMT( RI_FORMAT_RGBA8_UINT,            VK_FORMAT_R8G8B8A8_UINT )                  \
	FMT( RI_FORMAT_RGBA8_SINT,            VK_FORMAT_R8G8B8A8_SINT )                  \
	FMT( RI_FORMAT_RGBA8_SRGB,            VK_FORMAT_R8G8B8A8_SRGB )                  \
	FMT( RI_FORMAT_R16_UNORM,             VK_FORMAT_R16_UNORM )                      \
	FMT( RI_FORMAT_R16_SNORM,             VK_FORMAT_R16_SNORM )                      \
	FMT( RI_FORMAT_R16_UINT,              VK_FORMAT_R16_UINT )                       \
	FMT( RI_FORMAT_R16_SINT,              VK_FORMAT_R16_SINT )                       \
	FMT( RI_FORMAT_R16_SFLOAT,            VK_FORMAT_R16_SFLOAT )                     \
	FMT( RI_FORMAT_RG16_UNORM,            VK_FORMAT_R16G16_UNORM )                   \
	FMT( RI_FORMAT_RG16_SNORM,            VK_FORMAT_R16G16_SNORM )                   \
	FMT( RI_FORMAT_RG16_UINT,             VK_FORMAT_R16G16_UINT )                    \
	FMT( RI_FORMAT_RG16_SINT,             VK_FORMAT_R16G16_SINT )                    \
	FMT( RI_FORMAT_RG16_SFLOAT,           VK_FORMAT_R16G16_SFLOAT )                  \
	FMT( RI_FORMAT_RGBA16_UNORM,          VK_FORMAT_R16G16B16A16_UNORM )             \
	FMT( RI_FORMAT_RGBA16_SNORM,          VK_FORMAT_R16G16B16A16_SNORM )             \
	FMT( RI_FORMAT_RGBA16_UINT,           VK_FORMAT_R16G16B16A16_UINT )              \
	FMT( RI_FORMAT_RGBA16_SINT,           VK_FORMAT_R16G16B16A16_SINT )              \
	FMT( RI_FORMAT_RGBA16_SFLOAT,         VK_FORMAT_R16G16B16A16_SFLOAT )            \
	FMT( RI_FORMAT_R32_UINT,              VK_FORMAT_R32_UINT )                       \
	FMT( RI_FORMAT_R32_SINT,              VK_FORMAT_R32_SINT )                       \
	FMT( RI_FORMAT_R32_SFLOAT,            VK_FORMAT_R32_SFLOAT )                     \
	FMT( RI_FORMAT_RG32_UINT,             VK_FORMAT_R32G32_UINT )                    \
	FMT( RI_FORMAT_RG32_SINT,             VK_FORMAT_R32G32_SINT )                    \
	FMT( RI_FORMAT_RG32_SFLOAT,           VK_FORMAT_R32G32_SFLOAT )                  \
	FMT( RI_FORMAT_RGB32_UINT,            VK_FORMAT_R32G32B32_UINT )                 \
	FMT( RI_FORMAT_RGB32_SINT,            VK_FORMAT_R32G32B32_SINT )                 \
	FMT( RI_FORMAT_RGB32_SFLOAT,          VK_FORMAT_R32G32B32_SFLOAT )               \
	FMT( RI_FORMAT_RGBA32_UINT,           VK_FORMAT_R32G32B32A32_UINT )              \
	FMT( RI_FORMAT_RGBA32_SINT,           VK_FORMAT_R32G32B32A32_SINT )              \
	FMT( RI_FORMAT_RGBA32_SFLOAT,         VK_FORMAT_R32G32B32A32_SFLOAT )            \
	FMT( RI_FORMAT_R10_G10_B10_A2_UNORM,  VK_FORMAT_A2R10G10B10_UNORM_PACK32 )       \
	FMT( RI_FORMAT_R10_G10_B10_A2_UINT,   VK_FORMAT_A2R10G10B10_UINT_PACK32 )        \
	FMT( RI_FORMAT_R11_G11_B10_UFLOAT,    VK_FORMAT_B10G11R11_UFLOAT_PACK32 )        \
	FMT( RI_FORMAT_R9_G9_B9_E5_UNORM,     VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 )         \
	FMT( RI_FORMAT_R5_G6_B5_UNORM,        VK_FORMAT_R5G6B5_UNORM_PACK16 )            \
	FMT( RI_FORMAT_R5_G5_B5_A1_UNORM,     VK_FORMAT_A1R5G5B5_UNORM_PACK16 )          \
	FMT( RI_FORMAT_R4_G4_B4_A4_UNORM,     VK_FORMAT_R4G4B4A4_UNORM_PACK16 )          \
	FMT( RI_FORMAT_BC1_RGBA_UNORM,        VK_FORMAT_BC1_RGBA_UNORM_BLOCK )           \
	FMT( RI_FORMAT_BC1_RGBA_SRGB,         VK_FORMAT_BC1_RGBA_SRGB_BLOCK )            \
	FMT( RI_FORMAT_BC2_RGBA_UNORM,        VK_FORMAT_BC2_UNORM_BLOCK )                \
	FMT( RI_FORMAT_BC2_RGBA_SRGB,         VK_FORMAT_BC2_SRGB_BLOCK )                 \
	FMT( RI_FORMAT_BC3_RGBA_UNORM,        VK_FORMAT_BC3_UNORM_BLOCK )                \
	FMT( RI_FORMAT_BC3_RGBA_SRGB,         VK_FORMAT_BC3_SRGB_BLOCK )                 \
	FMT( RI_FORMAT_BC4_R_UNORM,           VK_FORMAT_BC4_UNORM_BLOCK )                \
	FMT( RI_FORMAT_BC4_R_SNORM,           VK_FORMAT_BC4_SNORM_BLOCK )                \
	FMT( RI_FORMAT_BC5_RG_UNORM,          VK_FORMAT_BC5_UNORM_BLOCK )                \
	FMT( RI_FORMAT_BC5_RG_SNORM,          VK_FORMAT_BC5_SNORM_BLOCK )                \
	FMT( RI_FORMAT_BC6H_RGB_UFLOAT,       VK_FORMAT_BC6H_UFLOAT_BLOCK )              \
	FMT( RI_FORMAT_BC6H_RGB_SFLOAT,       VK_FORMAT_BC6H_SFLOAT_BLOCK )             \
	FMT( RI_FORMAT_BC7_RGBA_UNORM,        VK_FORMAT_BC7_UNORM_BLOCK )                \
	FMT( RI_FORMAT_BC7_RGBA_SRGB,         VK_FORMAT_BC7_SRGB_BLOCK )                 \
	FMT( RI_FORMAT_D16_UNORM,             VK_FORMAT_D16_UNORM )                      \
	FMT( RI_FORMAT_D32_SFLOAT,            VK_FORMAT_D32_SFLOAT )                     \
	FWD( RI_FORMAT_D32_SFLOAT_S8_UINT_X24, VK_FORMAT_D32_SFLOAT_S8_UINT )            \
	FMT( RI_FORMAT_R24_UNORM_X8,          VK_FORMAT_X8_D24_UNORM_PACK32 )            \
	FWD( RI_FORMAT_X24_R8_UINT,           VK_FORMAT_X8_D24_UNORM_PACK32 )            \
	FWD( RI_FORMAT_X32_R8_UINT_X24,       VK_FORMAT_D32_SFLOAT_S8_UINT )            \
	FWD( RI_FORMAT_R32_SFLOAT_X8_X24,     VK_FORMAT_D32_SFLOAT_S8_UINT )            \
	FMT( RI_FORMAT_ETC1_R8G8B8_OES,       VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK )        \
	FWD( RI_FORMAT_ETC2_R8G8B8_UNORM,     VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK )        \
	FMT( RI_FORMAT_ETC2_R8G8B8_SRGB,      VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK )         \
	FMT( RI_FORMAT_ETC2_R8G8B8A1_UNORM,   VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK )      \
	FMT( RI_FORMAT_ETC2_R8G8B8A1_SRGB,    VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK )       \
	FMT( RI_FORMAT_ETC2_R8G8B8A8_UNORM,   VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK )      \
	FMT( RI_FORMAT_ETC2_R8G8B8A8_SRGB,    VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK )       \
	FMT( RI_FORMAT_ETC2_EAC_R11_UNORM,    VK_FORMAT_EAC_R11_UNORM_BLOCK )            \
	FMT( RI_FORMAT_ETC2_EAC_R11_SNORM,    VK_FORMAT_EAC_R11_SNORM_BLOCK )            \
	FMT( RI_FORMAT_ETC2_EAC_R11G11_UNORM, VK_FORMAT_EAC_R11G11_UNORM_BLOCK )         \
	FMT( RI_FORMAT_ETC2_EAC_R11G11_SNORM, VK_FORMAT_EAC_R11G11_SNORM_BLOCK )

const VkFormat RIFormatToVK(uint32_t format) {
	switch (format) {
#define FMT( ri, vk ) case ri: return vk;
#define FWD( ri, vk ) case ri: return vk;
		RI_VK_FORMAT_MAP( FMT, FWD )
#undef FMT
#undef FWD
		default:
			Com_Printf("Unhandled RI_Format to VK: %d\n", (int)format);
			break;
	}
	return VK_FORMAT_UNDEFINED;
}

const enum RI_Format_e VKToRIFormat(VkFormat format) {
	switch (format) {
#define FMT( ri, vk ) case vk: return ri;
#define FWD( ri, vk )
		RI_VK_FORMAT_MAP( FMT, FWD )
#undef FMT
#undef FWD
		default:
			Com_Printf("Unhandled VK format to RI: %d\n", (int)format);
			break;
	}
	return RI_FORMAT_UNKNOWN;
}
#endif

