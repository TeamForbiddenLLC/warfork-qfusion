#ifndef RI_CONVERSION_H
#define RI_CONVERSION_H

#include "ri_types.h"

static inline enum RIVendor_e VendorFromID(uint32_t vendorID) {
    switch (vendorID) {
		case 0x10DE:
			return RI_NVIDIA;
		case 0x1002:
			return RI_AMD;
		case 0x8086:
			return RI_INTEL;
	}
	return RI_UNKNOWN;
}

#if DEVICE_IMPL_VULKAN

static inline VkRect2D RIToVKRect2D(struct RIRect_s* in) {
	VkRect2D out;
	out.extent.width = in->width;
	out.extent.height = in->height;
	out.offset.x = in->x;
	out.offset.y = in->y;
	return out;
}

static inline VkRect2D RIViewportToRect2D( struct RIViewport_s *in )
{
	VkRect2D out;
	out.extent.width = in->width;
	out.extent.height = in->height;
	out.offset.x = in->x;
	out.offset.y = in->y;
	return out;
}

static inline VkViewport RIToVKViewport(struct RIViewport_s* in) {
	VkViewport out;
	out.x = in->x;
	out.y = in->y;
	out.width = in->width;
	out.height = in->height;
	out.minDepth = in->depthMin;
	out.maxDepth = in->depthMax;

	// Origin top-left requires flipping
	if( !in->originBottomLeft ) {
		out.y += in->height;
		out.height = -in->height;
	}
	return out;
}

static inline VkCompareOp ri_vk_RICompareOpToVK(enum RICompareFunc_e func) {
	switch (func) {
		case RI_COMPARE_NONE:
			return VK_COMPARE_OP_NEVER;
		case RI_COMPARE_ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		case RI_COMPARE_NEVER:
			return VK_COMPARE_OP_NEVER;
		case RI_COMPARE_EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case RI_COMPARE_NOT_EQUAL:
			return VK_COMPARE_OP_NOT_EQUAL;
		case RI_COMPARE_LESS:
			return VK_COMPARE_OP_LESS;
		case RI_COMPARE_LESS_EQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case RI_COMPARE_GREATER:
			return VK_COMPARE_OP_GREATER;
		case RI_COMPARE_GREATER_EQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		default:
			break;
	}
	assert(false);
	return VK_COMPARE_OP_NEVER;
}

static inline VkIndexType ri_vk_RIIndexTypeToVK(enum RIIndexType_e type) {
	switch(type) {
		case RI_INDEX_TYPE_16:
			return VK_INDEX_TYPE_UINT16;
		case RI_INDEX_TYPE_32:
			return VK_INDEX_TYPE_UINT32;
	}
	assert( false );
	return VK_INDEX_TYPE_UINT32;
}

static inline VkCullModeFlagBits ri_vk_RICullModeToVK( enum RICullMode_e mask )
{
	VkCullModeFlagBits flags = VK_CULL_MODE_NONE;
	if( mask & RI_CULL_MODE_FRONT )
		flags |= VK_CULL_MODE_FRONT_BIT;
	if( mask & RI_CULL_MODE_BACK )
		flags |= VK_CULL_MODE_BACK_BIT;

	return flags;
}

static inline VkBlendFactor ri_vk_RIColorWriteMaskToVK(enum RIColorWriteMask_e mask) {
	VkBlendFactor ret = 0;
	if (mask & RI_COLOR_WRITE_R) {
		ret |= VK_COLOR_COMPONENT_R_BIT;
	}
	if (mask & RI_COLOR_WRITE_G) {
		ret |= VK_COLOR_COMPONENT_G_BIT;
	}
	if (mask & RI_COLOR_WRITE_B) {
		ret |= VK_COLOR_COMPONENT_B_BIT;
	}
	if (mask & RI_COLOR_WRITE_A) {
		ret |= VK_COLOR_COMPONENT_A_BIT;
	}
	return ret;
}

static inline VkPrimitiveTopology ri_vk_RITopologyToVK(enum RITopology_e topology) {
	switch (topology) {
		case RI_TOPOLOGY_POINT_LIST:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case RI_TOPOLOGY_LINE_LIST:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case RI_TOPOLOGY_LINE_STRIP:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case RI_TOPOLOGY_TRIANGLE_LIST:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case RI_TOPOLOGY_TRIANGLE_STRIP:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case RI_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
		case RI_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		case RI_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		case RI_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
		case RI_TOPOLOGY_PATCH_LIST:
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	}
	// this shouldn't happen
	assert(false);
	return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

static inline VkBlendFactor ri_vk_RIBlendFactorToVK(enum RIBlendFactor_e factor) {
	switch (factor) {
		case RI_BLEND_ZERO:
			return VK_BLEND_FACTOR_ZERO;
		case RI_BLEND_ONE:
			return VK_BLEND_FACTOR_ONE;
		case RI_BLEND_SRC_COLOR:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case RI_BLEND_ONE_MINUS_SRC_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case RI_BLEND_DST_COLOR:
			return VK_BLEND_FACTOR_DST_COLOR;
		case RI_BLEND_ONE_MINUS_DST_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case RI_BLEND_SRC_ALPHA:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case RI_BLEND_ONE_MINUS_SRC_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case RI_BLEND_DST_ALPHA:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case RI_BLEND_ONE_MINUS_DST_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case RI_BLEND_CONSTANT_COLOR:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case RI_BLEND_ONE_MINUS_CONSTANT_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case RI_BLEND_CONSTANT_ALPHA:
			return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case RI_BLEND_ONE_MINUS_CONSTANT_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case RI_BLEND_SRC_ALPHA_SATURATE:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case RI_BLEND_SRC1_COLOR:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case RI_BLEND_ONE_MINUS_SRC1_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case RI_BLEND_SRC1_ALPHA:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case RI_BLEND_ONE_MINUS_SRC1_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	}
	return 0;
} 

static inline VkImageType ri_vk_RITextureTypeToVKImageType( enum RITextureType_e e )
{
	switch( e ) {
		case RI_TEXTURE_1D:
			return VK_IMAGE_TYPE_1D;
		case RI_TEXTURE_2D:
			return VK_IMAGE_TYPE_2D;
		case RI_TEXTURE_3D:
			return VK_IMAGE_TYPE_3D;
	}
	// this shouldn't happen
	assert( false );
	return VK_IMAGE_TYPE_MAX_ENUM;
}
#endif

#endif
