// SPDX-License-Identifier: GPL-2.0-or-later
//
// Plain-old-data structs and enums shared across the Metal / Foundation /
// QuartzCore bindings. These mirror the C structs and NS_ENUM values defined by
// the system frameworks, so they are declared directly (no framework header is
// pulled in) with the exact layout and integer values the Objective-C runtime
// expects.
//
// Port of rhi-zig/deps/metal/src/types.zig.

#ifndef MTLC_TYPES_H
#define MTLC_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
#define MTLC_EXTERN_C extern "C"
#define MTLC_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define MTLC_EXTERN_C
#define MTLC_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

// NSUInteger -- pointer-width unsigned integer used pervasively by the
// Foundation / Metal APIs for counts, lengths and indices.
typedef unsigned long mtlc_uinteger;
// NSInteger -- pointer-width signed integer.
typedef long mtlc_integer;

MTLC_STATIC_ASSERT(sizeof(mtlc_uinteger) == sizeof(void *),
               "mtlc_uinteger must be pointer-width (NSUInteger)");
MTLC_STATIC_ASSERT(sizeof(mtlc_integer) == sizeof(void *),
               "mtlc_integer must be pointer-width (NSInteger)");

// -- Geometry ---------------------------------------------------------------

// CGSize (CoreGraphics). Used by ca_metal_layer_set_drawable_size.
struct cg_size {
  double width;
  double height;
};

// MTLSize.
struct mtlc_size {
  mtlc_uinteger width;
  mtlc_uinteger height;
  mtlc_uinteger depth;
};

// MTLOrigin.
struct mtlc_origin {
  mtlc_uinteger x;
  mtlc_uinteger y;
  mtlc_uinteger z;
};

// MTLRegion.
struct mtlc_region {
  struct mtlc_origin origin;
  struct mtlc_size size;
};

// MTLClearColor -- RGBA in the [0, 1] range.
struct mtlc_clear_color {
  double red;
  double green;
  double blue;
  double alpha;
};

// MTLViewport.
struct mtlc_viewport {
  double origin_x;
  double origin_y;
  double width;
  double height;
  double znear;
  double zfar;
};

// MTLScissorRect.
struct mtlc_scissor_rect {
  mtlc_uinteger x;
  mtlc_uinteger y;
  mtlc_uinteger width;
  mtlc_uinteger height;
};

// NSRange -- a location/length pair. Two NSUIntegers, so it is passed in two
// integer registers on both arm64 and x86_64: plain __MTLC_MSG, never _STRET
// (same reasoning as the CGSize note in internal.h).
struct ns_range {
  mtlc_uinteger location;
  mtlc_uinteger length;
};

// -- Enums (values taken from the metal-cpp headers) ------------------------
//
// Every enum below fixes its underlying type to mtlc_uinteger. That is not
// cosmetic: these values are handed straight to objc_msgSend through a cast
// function pointer, and a plain C enum would be int-sized, so the callee would
// read a half-width argument slot.

// MTLPixelFormat (MTLPixelFormat.hpp). Curated subset -- extend as needed.
enum mtlc_pixel_format : mtlc_uinteger {
  MTLC_PIXEL_FORMAT_INVALID = 0,
  MTLC_PIXEL_FORMAT_RGBA8UNORM = 70,
  MTLC_PIXEL_FORMAT_BGRA8UNORM = 80,
  MTLC_PIXEL_FORMAT_BGRA8UNORM_SRGB = 81,
  MTLC_PIXEL_FORMAT_DEPTH32FLOAT = 252,
};

// MTLLoadAction (MTLRenderPass.hpp).
enum mtlc_load_action : mtlc_uinteger {
  MTLC_LOAD_ACTION_DONT_CARE = 0,
  MTLC_LOAD_ACTION_LOAD = 1,
  MTLC_LOAD_ACTION_CLEAR = 2,
};

// MTLStoreAction (MTLRenderPass.hpp).
enum mtlc_store_action : mtlc_uinteger {
  MTLC_STORE_ACTION_DONT_CARE = 0,
  MTLC_STORE_ACTION_STORE = 1,
  MTLC_STORE_ACTION_MULTISAMPLE_RESOLVE = 2,
  MTLC_STORE_ACTION_STORE_AND_MULTISAMPLE_RESOLVE = 3,
};

// MTLPrimitiveType (MTLRenderCommandEncoder.hpp).
enum mtlc_primitive_type : mtlc_uinteger {
  MTLC_PRIMITIVE_TYPE_POINT = 0,
  MTLC_PRIMITIVE_TYPE_LINE = 1,
  MTLC_PRIMITIVE_TYPE_LINE_STRIP = 2,
  MTLC_PRIMITIVE_TYPE_TRIANGLE = 3,
  MTLC_PRIMITIVE_TYPE_TRIANGLE_STRIP = 4,
};

// MTLIndexType (MTLStageInputOutputDescriptor.hpp).
enum mtlc_index_type : mtlc_uinteger {
  MTLC_INDEX_TYPE_UINT16 = 0,
  MTLC_INDEX_TYPE_UINT32 = 1,
};

// MTLCompareFunction (MTLDepthStencil.hpp).
enum mtlc_compare_function : mtlc_uinteger {
  MTLC_COMPARE_FUNCTION_NEVER = 0,
  MTLC_COMPARE_FUNCTION_LESS = 1,
  MTLC_COMPARE_FUNCTION_EQUAL = 2,
  MTLC_COMPARE_FUNCTION_LESS_EQUAL = 3,
  MTLC_COMPARE_FUNCTION_GREATER = 4,
  MTLC_COMPARE_FUNCTION_NOT_EQUAL = 5,
  MTLC_COMPARE_FUNCTION_GREATER_EQUAL = 6,
  MTLC_COMPARE_FUNCTION_ALWAYS = 7,
};

// MTLSamplerMinMagFilter (MTLSampler.hpp).
enum mtlc_sampler_min_mag_filter : mtlc_uinteger {
  MTLC_SAMPLER_MIN_MAG_FILTER_NEAREST = 0,
  MTLC_SAMPLER_MIN_MAG_FILTER_LINEAR = 1,
};

// MTLSamplerMipFilter (MTLSampler.hpp).
enum mtlc_sampler_mip_filter : mtlc_uinteger {
  MTLC_SAMPLER_MIP_FILTER_NOT_MIPMAPPED = 0,
  MTLC_SAMPLER_MIP_FILTER_NEAREST = 1,
  MTLC_SAMPLER_MIP_FILTER_LINEAR = 2,
};

// MTLSamplerAddressMode (MTLSampler.hpp).
enum mtlc_sampler_address_mode : mtlc_uinteger {
  MTLC_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 0,
  MTLC_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 1,
  MTLC_SAMPLER_ADDRESS_MODE_REPEAT = 2,
  MTLC_SAMPLER_ADDRESS_MODE_MIRROR_REPEAT = 3,
  MTLC_SAMPLER_ADDRESS_MODE_CLAMP_TO_ZERO = 4,
  MTLC_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER_COLOR = 5,
};

// MTLVertexFormat (MTLVertexDescriptor.hpp). Curated subset.
enum mtlc_vertex_format : mtlc_uinteger {
  MTLC_VERTEX_FORMAT_INVALID = 0,
  MTLC_VERTEX_FORMAT_UCHAR2 = 1,
  MTLC_VERTEX_FORMAT_UCHAR4 = 3,
  MTLC_VERTEX_FORMAT_UCHAR4_NORMALIZED = 9,
  MTLC_VERTEX_FORMAT_HALF2 = 25,
  MTLC_VERTEX_FORMAT_HALF3 = 26,
  MTLC_VERTEX_FORMAT_HALF4 = 27,
  MTLC_VERTEX_FORMAT_FLOAT = 28,
  MTLC_VERTEX_FORMAT_FLOAT2 = 29,
  MTLC_VERTEX_FORMAT_FLOAT3 = 30,
  MTLC_VERTEX_FORMAT_FLOAT4 = 31,
  MTLC_VERTEX_FORMAT_HALF = 53,
};

// MTLCullMode (MTLRenderCommandEncoder.hpp).
enum mtlc_cull_mode : mtlc_uinteger {
  MTLC_CULL_MODE_NONE = 0,
  MTLC_CULL_MODE_FRONT = 1,
  MTLC_CULL_MODE_BACK = 2,
};

// MTLWinding (MTLRenderCommandEncoder.hpp).
enum mtlc_winding : mtlc_uinteger {
  MTLC_WINDING_CLOCKWISE = 0,
  MTLC_WINDING_COUNTER_CLOCKWISE = 1,
};

// MTLTriangleFillMode (MTLRenderCommandEncoder.hpp).
enum mtlc_triangle_fill_mode : mtlc_uinteger {
  MTLC_TRIANGLE_FILL_MODE_FILL = 0,
  MTLC_TRIANGLE_FILL_MODE_LINES = 1,
};

// MTLBlendFactor (MTLRenderPipeline.hpp).
enum mtlc_blend_factor : mtlc_uinteger {
  MTLC_BLEND_FACTOR_ZERO = 0,
  MTLC_BLEND_FACTOR_ONE = 1,
  MTLC_BLEND_FACTOR_SOURCE_COLOR = 2,
  MTLC_BLEND_FACTOR_ONE_MINUS_SOURCE_COLOR = 3,
  MTLC_BLEND_FACTOR_SOURCE_ALPHA = 4,
  MTLC_BLEND_FACTOR_ONE_MINUS_SOURCE_ALPHA = 5,
  MTLC_BLEND_FACTOR_DESTINATION_COLOR = 6,
  MTLC_BLEND_FACTOR_ONE_MINUS_DESTINATION_COLOR = 7,
  MTLC_BLEND_FACTOR_DESTINATION_ALPHA = 8,
  MTLC_BLEND_FACTOR_ONE_MINUS_DESTINATION_ALPHA = 9,
  MTLC_BLEND_FACTOR_SOURCE_ALPHA_SATURATED = 10,
};

// MTLBlendOperation (MTLRenderPipeline.hpp).
enum mtlc_blend_operation : mtlc_uinteger {
  MTLC_BLEND_OPERATION_ADD = 0,
  MTLC_BLEND_OPERATION_SUBTRACT = 1,
  MTLC_BLEND_OPERATION_REVERSE_SUBTRACT = 2,
  MTLC_BLEND_OPERATION_MIN = 3,
  MTLC_BLEND_OPERATION_MAX = 4,
};

// MTLColorWriteMask (MTLRenderPipeline.hpp). An option set, not an enumeration.
enum mtlc_color_write_mask : mtlc_uinteger {
  MTLC_COLOR_WRITE_MASK_NONE = 0,
  MTLC_COLOR_WRITE_MASK_RED = 1 << 3,
  MTLC_COLOR_WRITE_MASK_GREEN = 1 << 2,
  MTLC_COLOR_WRITE_MASK_BLUE = 1 << 1,
  MTLC_COLOR_WRITE_MASK_ALPHA = 1 << 0,
  MTLC_COLOR_WRITE_MASK_ALL = 0xf,
};

// MTLVertexStepFunction (MTLVertexDescriptor.hpp).
enum mtlc_vertex_step_function : mtlc_uinteger {
  MTLC_VERTEX_STEP_FUNCTION_CONSTANT = 0,
  MTLC_VERTEX_STEP_FUNCTION_PER_VERTEX = 1,
  MTLC_VERTEX_STEP_FUNCTION_PER_INSTANCE = 2,
};

// MTLTextureType (MTLTexture.hpp). Curated subset.
enum mtlc_texture_type : mtlc_uinteger {
  MTLC_TEXTURE_TYPE_1D = 0,
  MTLC_TEXTURE_TYPE_2D = 2,
  MTLC_TEXTURE_TYPE_CUBE = 5,
  MTLC_TEXTURE_TYPE_3D = 7,
};

// MTLCPUCacheMode -- the standalone value, also the low 4 bits of
// mtlc_resource_options.
enum mtlc_cpu_cache_mode : mtlc_uinteger {
  MTLC_CPU_CACHE_MODE_DEFAULT_CACHE = 0,
  MTLC_CPU_CACHE_MODE_WRITE_COMBINED = 1,
};

// MTLResourceUsage -- how a shader reads a resource reached through an argument
// buffer. Declared with useResource:usage:stages: so Metal knows to make the
// allocation resident; resources bound directly on the encoder need none of
// this. MTLC_RESOURCE_USAGE_SAMPLE is deprecated since macOS 13 -- READ covers
// sampling.
enum mtlc_resource_usage : mtlc_uinteger {
  MTLC_RESOURCE_USAGE_READ = 1ul << 0,
  MTLC_RESOURCE_USAGE_WRITE = 1ul << 1,
  MTLC_RESOURCE_USAGE_SAMPLE = 1ul << 2,
};

// MTLRenderStages -- which stages a residency declaration applies to. Naming
// only the stage that actually reads the resource is cheaper than declaring it
// for all of them.
enum mtlc_render_stages : mtlc_uinteger {
  MTLC_RENDER_STAGE_VERTEX = 1ul << 0,
  MTLC_RENDER_STAGE_FRAGMENT = 1ul << 1,
  MTLC_RENDER_STAGE_TILE = 1ul << 2,
  MTLC_RENDER_STAGE_OBJECT = 1ul << 3,
  MTLC_RENDER_STAGE_MESH = 1ul << 4,
};

// MTLStorageMode -- the standalone value taken by
// mtlc_texture_descriptor_set_storage_mode. NOT pre-shifted; the
// MTLC_RESOURCE_STORAGE_MODE_* constants below are the shifted form.
enum mtlc_storage_mode : mtlc_uinteger {
  MTLC_STORAGE_MODE_SHARED = 0,
  MTLC_STORAGE_MODE_MANAGED = 1,
  MTLC_STORAGE_MODE_PRIVATE = 2,
  MTLC_STORAGE_MODE_MEMORYLESS = 3,
};

// MTLHazardTrackingMode -- the standalone value, bits 8..9 of
// mtlc_resource_options.
enum mtlc_hazard_tracking_mode : mtlc_uinteger {
  MTLC_HAZARD_TRACKING_MODE_DEFAULT = 0,
  MTLC_HAZARD_TRACKING_MODE_UNTRACKED = 1,
  MTLC_HAZARD_TRACKING_MODE_TRACKED = 2,
};

// MTLArgumentBuffersTier (MTLDevice.hpp) -- what -[MTLDevice argumentBuffersSupport]
// reports. Tier1 is every Metal-capable GPU (Intel integrated included); Tier2
// adds far larger per-argument-buffer resource limits. The SDK declares this
// NS_ENUM(NSInteger), but it is only ever returned, and the return slot is
// register-width either way, so mtlc_uinteger matches the rest of this header.
enum mtlc_argument_buffers_tier : mtlc_uinteger {
  MTLC_ARGUMENT_BUFFERS_TIER1 = 0,
  MTLC_ARGUMENT_BUFFERS_TIER2 = 1,
};

// -- Option sets (bitfields backed by NSUInteger) ---------------------------
//
// Unlike the single-value enums above, these are ORed together at call sites.
// Functions therefore take a plain mtlc_uinteger rather than the enum type: in
// C++ the result of ORing two enumerators is the underlying type and would not
// convert back to the enum, so an enum-typed parameter would break every C++
// caller that combines flags.

// MTLResourceOptions -- an OR of the pre-shifted constants below. Zero is
// Metal's default (StorageModeShared | CPUCacheModeDefaultCache | hazard
// tracking default), so passing 0 is always valid.
enum mtlc_resource_options : mtlc_uinteger {
  MTLC_RESOURCE_CPU_CACHE_MODE_DEFAULT_CACHE = 0ul << 0,
  MTLC_RESOURCE_CPU_CACHE_MODE_WRITE_COMBINED = 1ul << 0,

  MTLC_RESOURCE_STORAGE_MODE_SHARED = 0ul << 4,
  MTLC_RESOURCE_STORAGE_MODE_MANAGED = 1ul << 4,
  MTLC_RESOURCE_STORAGE_MODE_PRIVATE = 2ul << 4,
  MTLC_RESOURCE_STORAGE_MODE_MEMORYLESS = 3ul << 4,

  MTLC_RESOURCE_HAZARD_TRACKING_MODE_DEFAULT = 0ul << 8,
  MTLC_RESOURCE_HAZARD_TRACKING_MODE_UNTRACKED = 1ul << 8,
  MTLC_RESOURCE_HAZARD_TRACKING_MODE_TRACKED = 2ul << 8,
};

// MTLTextureUsage -- an OR of the flags below.
enum mtlc_texture_usage : mtlc_uinteger {
  MTLC_TEXTURE_USAGE_UNKNOWN = 0ul,
  MTLC_TEXTURE_USAGE_SHADER_READ = 1ul << 0,
  MTLC_TEXTURE_USAGE_SHADER_WRITE = 1ul << 1,
  MTLC_TEXTURE_USAGE_RENDER_TARGET = 1ul << 2,
  MTLC_TEXTURE_USAGE_PIXEL_FORMAT_VIEW = 1ul << 4,
};

// Guard against accidental drift in the packed option-set values. These are the
// same three assertions the Zig bindings carry.
MTLC_STATIC_ASSERT(MTLC_RESOURCE_STORAGE_MODE_PRIVATE == 0x20,
               "MTLResourceStorageModePrivate must be 0x20");
MTLC_STATIC_ASSERT(MTLC_RESOURCE_STORAGE_MODE_MANAGED == 0x10,
               "MTLResourceStorageModeManaged must be 0x10");
MTLC_STATIC_ASSERT(MTLC_TEXTURE_USAGE_RENDER_TARGET == 0x4,
               "MTLTextureUsageRenderTarget must be 0x4");

// -- Handle boilerplate -----------------------------------------------------

// Every Metal / Foundation / QuartzCore class is modelled the way metal-cpp
// models it: a thin typed view over an objc_object*. In C that is a struct with
// a single `obj` field, passed by value, where obj == NULL means nil.
//
// MTLC_HANDLE(T) declares `struct T` plus the four operations every handle
// shares. _from_id and _is_nil are inline (no message send); _retain and
// _release are defined in the library, so that no public header has to pull in
// <objc/message.h>.
//
// Ownership follows Cocoa, exactly as the Zig bindings do. Anything returned by
// a *_new_* function or a descriptor's _init is owned by the caller at retain
// count 1 -- release it. Property getters (_name, _next_drawable,
// _color_attachments, ...) return autoreleased objects you do NOT own; retain
// only if you keep them past the current autorelease pool.
#define MTLC_HANDLE(T)                                                         \
  struct T {                                                                   \
    void *obj;                                                                 \
  };                                                                           \
  static inline struct T T##_from_id(void *raw) {                              \
    struct T h;                                                                \
    h.obj = raw;                                                               \
    return h;                                                                  \
  }                                                                            \
  static inline bool T##_is_nil(struct T h) { return h.obj == NULL; }          \
  MTLC_EXTERN_C void T##_retain(struct T h);                                   \
  MTLC_EXTERN_C void T##_release(struct T h)

#endif // MTLC_TYPES_H
