#ifndef RI_CONVERSION_MTL_H
#define RI_CONVERSION_MTL_H

// Metal-backend counterpart to ri_conversion.h: backend-neutral RI enums -> metal-c enums. Guarded by
// DEVICE_IMPL_MTL so it is a no-op on non-Metal builds.

#include <assert.h>

#include "ri_types.h"
#include "ri_pipeline.h"

#if DEVICE_IMPL_MTL

static inline struct mtlc_viewport RIToMTLViewport( struct RIViewport_s *in )
{
	// Metal's viewport is top-left origin with a y-down framebuffer, the opposite of the GL/Vulkan
	// bottom-left convention RI defaults to. When the caller does not request bottom-left, the values
	// pass straight through; the bottom-left case is handled at pipeline/NDC level in a later phase.
	struct mtlc_viewport out;
	out.origin_x = in->x;
	out.origin_y = in->y;
	out.width = in->width;
	out.height = in->height;
	out.znear = in->depthMin;
	out.zfar = in->depthMax;
	return out;
}

static inline struct mtlc_scissor_rect RIToMTLScissorRect( struct RIRect_s *in )
{
	struct mtlc_scissor_rect out;
	out.x = in->x;
	out.y = in->y;
	out.width = in->width;
	out.height = in->height;
	return out;
}

static inline enum mtlc_compare_function RICompareOpToMTL( enum RICompareFunc_e func )
{
	switch( func ) {
		// "None" means the depth test is disabled. Metal has no separate enable flag -- the equivalent is
		// Always (plus depthWriteEnabled:NO, set by the caller). Mapping it to Never would reject every
		// fragment instead of accepting every one.
		case RI_COMPARE_NONE: return MTLC_COMPARE_FUNCTION_ALWAYS;
		case RI_COMPARE_ALWAYS: return MTLC_COMPARE_FUNCTION_ALWAYS;
		case RI_COMPARE_NEVER: return MTLC_COMPARE_FUNCTION_NEVER;
		case RI_COMPARE_EQUAL: return MTLC_COMPARE_FUNCTION_EQUAL;
		case RI_COMPARE_NOT_EQUAL: return MTLC_COMPARE_FUNCTION_NOT_EQUAL;
		case RI_COMPARE_LESS: return MTLC_COMPARE_FUNCTION_LESS;
		case RI_COMPARE_LESS_EQUAL: return MTLC_COMPARE_FUNCTION_LESS_EQUAL;
		case RI_COMPARE_GREATER: return MTLC_COMPARE_FUNCTION_GREATER;
		case RI_COMPARE_GREATER_EQUAL: return MTLC_COMPARE_FUNCTION_GREATER_EQUAL;
		default: break;
	}
	assert( false );
	return MTLC_COMPARE_FUNCTION_NEVER;
}

static inline enum mtlc_index_type RIIndexTypeToMTL( enum RIIndexType_e type )
{
	switch( type ) {
		case RI_INDEX_TYPE_16: return MTLC_INDEX_TYPE_UINT16;
		case RI_INDEX_TYPE_32: return MTLC_INDEX_TYPE_UINT32;
	}
	assert( false );
	return MTLC_INDEX_TYPE_UINT32;
}

// Metal has no primitive-topology-with-adjacency or patch primitives; those RI topologies have no Metal
// equivalent and are asserted out. Strips/lists map directly.
static inline enum mtlc_primitive_type RITopologyToMTL( enum RITopology_e topology )
{
	switch( topology ) {
		case RI_TOPOLOGY_POINT_LIST: return MTLC_PRIMITIVE_TYPE_POINT;
		case RI_TOPOLOGY_LINE_LIST: return MTLC_PRIMITIVE_TYPE_LINE;
		case RI_TOPOLOGY_LINE_STRIP: return MTLC_PRIMITIVE_TYPE_LINE_STRIP;
		case RI_TOPOLOGY_TRIANGLE_LIST: return MTLC_PRIMITIVE_TYPE_TRIANGLE;
		case RI_TOPOLOGY_TRIANGLE_STRIP: return MTLC_PRIMITIVE_TYPE_TRIANGLE_STRIP;
		default: break;
	}
	assert( false );
	return MTLC_PRIMITIVE_TYPE_TRIANGLE;
}

static inline enum mtlc_cull_mode RICullModeToMTL( enum RICullMode_e mode )
{
	switch( mode ) {
		case RI_CULL_MODE_NONE: return MTLC_CULL_MODE_NONE;
		case RI_CULL_MODE_FRONT: return MTLC_CULL_MODE_FRONT;
		case RI_CULL_MODE_BACK: return MTLC_CULL_MODE_BACK;
		default: break;
	}
	// RI_CULL_MODE_BOTH has no Metal equivalent (Metal culls at most one face). Callers that need it
	// must skip the draw entirely rather than rely on raster state.
	return MTLC_CULL_MODE_NONE;
}

static inline enum mtlc_blend_factor RIBlendFactorToMTL( enum RIBlendFactor_e factor )
{
	switch( factor ) {
		case RI_BLEND_ZERO: return MTLC_BLEND_FACTOR_ZERO;
		case RI_BLEND_ONE: return MTLC_BLEND_FACTOR_ONE;
		case RI_BLEND_SRC_COLOR: return MTLC_BLEND_FACTOR_SOURCE_COLOR;
		case RI_BLEND_ONE_MINUS_SRC_COLOR: return MTLC_BLEND_FACTOR_ONE_MINUS_SOURCE_COLOR;
		case RI_BLEND_DST_COLOR: return MTLC_BLEND_FACTOR_DESTINATION_COLOR;
		case RI_BLEND_ONE_MINUS_DST_COLOR: return MTLC_BLEND_FACTOR_ONE_MINUS_DESTINATION_COLOR;
		case RI_BLEND_SRC_ALPHA: return MTLC_BLEND_FACTOR_SOURCE_ALPHA;
		case RI_BLEND_ONE_MINUS_SRC_ALPHA: return MTLC_BLEND_FACTOR_ONE_MINUS_SOURCE_ALPHA;
		case RI_BLEND_DST_ALPHA: return MTLC_BLEND_FACTOR_DESTINATION_ALPHA;
		case RI_BLEND_ONE_MINUS_DST_ALPHA: return MTLC_BLEND_FACTOR_ONE_MINUS_DESTINATION_ALPHA;
		case RI_BLEND_SRC_ALPHA_SATURATE: return MTLC_BLEND_FACTOR_SOURCE_ALPHA_SATURATED;
		default: break;
	}
	// Blend constants and dual-source (SRC1) factors need bindings metal-c does not have yet; the
	// engine's GLSTATE blend modes never select them, so reaching here is a programming error.
	assert( false );
	return MTLC_BLEND_FACTOR_ONE;
}

// RI's mask bits are R=1,G=2,B=4,A=8; Metal's are reversed (A=1,B=2,G=4,R=8), so this is a remap
// rather than a cast.
static inline mtlc_uinteger RIColorWriteMaskToMTL( enum RIColorWriteMask_e mask )
{
	mtlc_uinteger out = MTLC_COLOR_WRITE_MASK_NONE;
	if( mask & RI_COLOR_WRITE_R ) out |= MTLC_COLOR_WRITE_MASK_RED;
	if( mask & RI_COLOR_WRITE_G ) out |= MTLC_COLOR_WRITE_MASK_GREEN;
	if( mask & RI_COLOR_WRITE_B ) out |= MTLC_COLOR_WRITE_MASK_BLUE;
	if( mask & RI_COLOR_WRITE_A ) out |= MTLC_COLOR_WRITE_MASK_ALPHA;
	return out;
}

// Vertex-attribute formats. Only the formats R_FillNriVertexAttrib* actually emit are mapped; anything
// else is a programming error rather than a silently invalid pipeline.
static inline enum mtlc_vertex_format RIFormatToMTLVertex( enum RI_Format_e format )
{
	switch( format ) {
		case RI_FORMAT_R16_SFLOAT: return MTLC_VERTEX_FORMAT_HALF;
		case RI_FORMAT_RG16_SFLOAT: return MTLC_VERTEX_FORMAT_HALF2;
		case RI_FORMAT_RGBA16_SFLOAT: return MTLC_VERTEX_FORMAT_HALF4;
		case RI_FORMAT_R32_SFLOAT: return MTLC_VERTEX_FORMAT_FLOAT;
		case RI_FORMAT_RG32_SFLOAT: return MTLC_VERTEX_FORMAT_FLOAT2;
		case RI_FORMAT_RGB32_SFLOAT: return MTLC_VERTEX_FORMAT_FLOAT3;
		case RI_FORMAT_RGBA32_SFLOAT: return MTLC_VERTEX_FORMAT_FLOAT4;
		case RI_FORMAT_RGBA8_UINT: return MTLC_VERTEX_FORMAT_UCHAR4;
		case RI_FORMAT_RGBA8_UNORM: return MTLC_VERTEX_FORMAT_UCHAR4_NORMALIZED;
		default: break;
	}
	assert( false );
	return MTLC_VERTEX_FORMAT_INVALID;
}

#endif

#endif
