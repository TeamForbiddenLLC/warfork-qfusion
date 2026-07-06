#ifndef RI_RESOURCE_H
#define RI_RESOURCE_H

// GPU resources: buffers, textures, texture views, and the barrier handles that describe their
// pipeline sync state.

#include "ri_prelude.h"

enum RITextureViewType_s {
	RI_VIEWTYPE_SHADER_RESOURCE_1D,
	RI_VIEWTYPE_SHADER_RESOURCE_1D_ARRAY,
	RI_VIEWTYPE_SHADER_RESOURCE_STORAGE_1D,
	RI_VIEWTYPE_SHADER_RESOURCE_STORAGE_1D_ARRAY,
	RI_VIEWTYPE_SHADER_RESOURCE_2D,
	RI_VIEWTYPE_SHADER_RESOURCE_2D_ARRAY,
	RI_VIEWTYPE_SHADER_RESOURCE_CUBE,
	RI_VIEWTYPE_SHADER_RESOURCE_CUBE_ARRAY,
	RI_VIEWTYPE_SHADER_RESOURCE_STORAGE_2D,
	RI_VIEWTYPE_SHADER_RESOURCE_STORAGE_2D_ARRAY,

	RI_VIEWTYPE_COLOR_ATTACHMENT,
	RI_VIEWTYPE_DEPTH_STENCIL_ATTACHMENT,
	RI_VIEWTYPE_DEPTH_READONLY_STENCIL_ATTACHMENT,
	RI_VIEWTYPE_DEPTH_ATTACHMENT_STENCIL_READONLY,
	RI_VIEWTYPE_DEPTH_STENCIL_READONLY,
	RI_VIEWTYPE_SHADING_RATE_ATTACHMENT
};

enum RITextureUsageBits_e {
	RI_USAGE_NONE = 0,
	RI_USAGE_SHADER_RESOURCE = 0x1,
	RI_USAGE_SHADER_RESOURCE_STORAGE = 0x2,
	RI_USAGE_COLOR_ATTACHMENT = 0x4,
	RI_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x8,
	RI_USAGE_SHADING_RATE = 0x10,
};

enum RISampleCount_e
{
	RI_SAMPLE_COUNT_1 = 1,
	RI_SAMPLE_COUNT_2 = 2,
	RI_SAMPLE_COUNT_4 = 4,
	RI_SAMPLE_COUNT_8 = 8,
	RI_SAMPLE_COUNT_16 = 16,
	RI_SAMPLE_COUNT_COUNT = 5,
};

enum RIBufferUsage_e {
	RI_BUFFER_USAGE_NONE = 0,
	RI_BUFFER_USAGE_SHADER_RESOURCE = 0x1,
	RI_BUFFER_USAGE_SHADER_RESOURCE_STORAGE = 0x2,
	RI_BUFFER_USAGE_VERTEX_BUFFER = 0x4,
	RI_BUFFER_USAGE_INDEX_BUFFER = 0x8,
	RI_BUFFER_USAGE_CONSTANT_BUFFER = 0x10,
	RI_BUFFER_USAGE_ARGUMENT_BUFFER = 0x20,

	RI_BUFFER_USAGE_SCRATCH = 0x40,
	RI_BUFFER_USAGE_BINDING_TABLE = 0x80,
	RI_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPT = 0x100,
	RI_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE = 0x200,
};

enum RITextureType_e {
	RI_TEXTURE_1D,
	RI_TEXTURE_2D,
	RI_TEXTURE_3D
};

struct RIBuffer_s {
	// Stable identity (hash_random) decoupled from the reusable Vulkan handle; folded into any
	// descriptor built from this buffer so a freed+recreated handle never aliases in the set cache.
	hash_t cookie;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkBuffer buffer;
			struct VmaAllocation_T *allocation;
		} vk;
#endif
	};
};

struct RIBarrierImageHandle_s {
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkPipelineStageFlags2 stage;
			VkAccessFlags2 access;
			VkImageLayout layout;
		} vk;
#endif
	};
};

struct RIBarrierBufferHandle_s {
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkPipelineStageFlags2 stage;
			VkAccessFlags2 access;
		} vk;
#endif
	};
};

struct RITexture_s {
	// Stable identity (hash_random) decoupled from the reusable Vulkan handle. See RIBuffer_s.cookie.
	hash_t cookie;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkImage image;
			struct VmaAllocation_T *allocation;
		} vk;
#endif
	};
};

struct RITextureView_s {
	// Stable identity (hash_random); the input folded into a sampled/storage-image descriptor cookie.
	hash_t cookie;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkImageView image;
		} vk;
#endif
	};
};

#endif
