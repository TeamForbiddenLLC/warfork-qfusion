#ifndef RI_MTL_H
#define RI_MTL_H

// Metal-backend counterpart to ri_vk.h: format mapping and small conversion helpers used only when the
// Metal backend is selected (DEVICE_IMPL_MTL). Kept behind the guard so the header is a harmless no-op
// on non-Metal builds, exactly like ri_vk.h is on non-Vulkan builds.

#include "ri_types.h"
#include "ri_format.h"

#if DEVICE_IMPL_MTL

// RI_Format_e <-> MTLPixelFormat. metal-c currently exposes only a curated subset of MTLPixelFormat
// (see enum mtlc_pixel_format in metal-c/types.h); formats without a binding map to
// MTLC_PIXEL_FORMAT_INVALID. Extend both the metal-c enum and this map as later phases need more.
enum mtlc_pixel_format RIFormatToMTL( uint32_t format );
enum RI_Format_e MTLToRIFormat( enum mtlc_pixel_format format );

#endif

#endif
