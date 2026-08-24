#include "ri_mtl.h"

#if DEVICE_IMPL_MTL

#include "../qcommon/qcommon.h"

// Single source of truth for the RI <-> MTL format mapping, mirroring RI_VK_FORMAT_MAP in ri_vk.c. Only
// the formats metal-c currently binds are listed; every other RI format resolves to INVALID until the
// metal-c enum (and this table) grow. Keep rows sorted to match the RI_Format_e declaration order.
#define RI_MTL_FORMAT_MAP( FMT )                                             \
	FMT( RI_FORMAT_UNKNOWN,     MTLC_PIXEL_FORMAT_INVALID )                   \
	FMT( RI_FORMAT_BGRA8_UNORM, MTLC_PIXEL_FORMAT_BGRA8UNORM )               \
	FMT( RI_FORMAT_BGRA8_SRGB,  MTLC_PIXEL_FORMAT_BGRA8UNORM_SRGB )          \
	FMT( RI_FORMAT_RGBA8_UNORM, MTLC_PIXEL_FORMAT_RGBA8UNORM )               \
	FMT( RI_FORMAT_D32_SFLOAT,  MTLC_PIXEL_FORMAT_DEPTH32FLOAT )

enum mtlc_pixel_format RIFormatToMTL( uint32_t format )
{
	switch( format ) {
#define FMT( ri, mtl ) case ri: return mtl;
		RI_MTL_FORMAT_MAP( FMT )
#undef FMT
		default:
			Com_Printf( "Unhandled RI_Format to MTL: %d\n", (int)format );
			break;
	}
	return MTLC_PIXEL_FORMAT_INVALID;
}

enum RI_Format_e MTLToRIFormat( enum mtlc_pixel_format format )
{
	switch( format ) {
#define FMT( ri, mtl ) case mtl: return ri;
		// RI_FORMAT_UNKNOWN <-> INVALID is covered by the default; skip it to avoid a duplicate case.
		FMT( RI_FORMAT_BGRA8_UNORM, MTLC_PIXEL_FORMAT_BGRA8UNORM )
		FMT( RI_FORMAT_BGRA8_SRGB,  MTLC_PIXEL_FORMAT_BGRA8UNORM_SRGB )
		FMT( RI_FORMAT_RGBA8_UNORM, MTLC_PIXEL_FORMAT_RGBA8UNORM )
		FMT( RI_FORMAT_D32_SFLOAT,  MTLC_PIXEL_FORMAT_DEPTH32FLOAT )
#undef FMT
		default:
			break;
	}
	return RI_FORMAT_UNKNOWN;
}

#endif
