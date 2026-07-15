#ifndef RI_RESOURCE_H
#define RI_RESOURCE_H

// GPU resources: buffers, textures, texture views, and the resource-state vocabulary that describes
// their pipeline sync state to barriers and descriptor builders.

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

// Backend-neutral resource state: an OR-able set describing how a resource is being accessed. The
// image layout, access mask, and pipeline stages of a barrier are all derived from it
// (RI_VK_ResourceStateTo* in ri_vk.h), so callers describe intent rather than hand-assembling masks.
// The all-clear value (RI_RESOURCE_STATE_UNDEFINED) is the UNDEFINED state.
// Ported from HPL2 RIBarrier.h RIResourceState_e, via rhi-zig cmd.zig ResourceState.
enum RIResourceState_e {
	RI_RESOURCE_STATE_UNDEFINED = 0,
	RI_RESOURCE_STATE_GENERAL = 0x1,             // GENERAL layout, broad read/write
	RI_RESOURCE_STATE_RENDER_TARGET = 0x2,       // COLOR_ATTACHMENT_OPTIMAL, write
	RI_RESOURCE_STATE_RENDER_TARGET_READ = 0x4,  // COLOR_ATTACHMENT_OPTIMAL, read|write (blend)
	RI_RESOURCE_STATE_DEPTH_WRITE = 0x8,         // DEPTH_ATTACHMENT_OPTIMAL
	RI_RESOURCE_STATE_DEPTH_READ = 0x10,         // DEPTH_READ_ONLY_OPTIMAL
	RI_RESOURCE_STATE_SHADER_RESOURCE = 0x20,    // SHADER_READ_ONLY_OPTIMAL, sampled
	RI_RESOURCE_STATE_STORAGE_READ = 0x40,       // GENERAL, storage read
	RI_RESOURCE_STATE_STORAGE_WRITE = 0x80,      // GENERAL, storage write
	RI_RESOURCE_STATE_COPY_SRC = 0x100,          // TRANSFER_SRC_OPTIMAL
	RI_RESOURCE_STATE_COPY_DST = 0x200,          // TRANSFER_DST_OPTIMAL
	RI_RESOURCE_STATE_PRESENT = 0x400,           // PRESENT_SRC_KHR
	RI_RESOURCE_STATE_INDIRECT_ARGUMENT = 0x800, // buffer only
	RI_RESOURCE_STATE_VERTEX_BUFFER = 0x1000,    // buffer only
	RI_RESOURCE_STATE_INDEX_BUFFER = 0x2000,     // buffer only
	RI_RESOURCE_STATE_CONSTANT_BUFFER = 0x4000,  // buffer only
	RI_RESOURCE_STATE_ACCEL_READ = 0x8000,       // acceleration structure read
	RI_RESOURCE_STATE_ACCEL_WRITE = 0x10000,     // acceleration structure build write
	RI_RESOURCE_STATE_CLEAR_STORAGE = 0x20000,   // GENERAL, vkCmdClear* transfer write
	RI_RESOURCE_STATE_HOST_READ = 0x40000,       // buffer only; carries no image layout

	// Named composite mirroring the OR'd value callers used before the set was widened.
	RI_RESOURCE_STATE_UNORDERED_ACCESS = RI_RESOURCE_STATE_STORAGE_READ | RI_RESOURCE_STATE_STORAGE_WRITE,
};

// Optional per-side stage narrowing on a barrier; RI_BARRIER_STAGE_NONE derives a conservative mask
// from the resource state instead (e.g. SHADER_RESOURCE -> all shader stages).
enum RIBarrierStages_e {
	RI_BARRIER_STAGE_NONE = 0,
	RI_BARRIER_STAGE_VERTEX = 0x1,
	RI_BARRIER_STAGE_FRAGMENT = 0x2,
	RI_BARRIER_STAGE_COMPUTE = 0x4,
	RI_BARRIER_STAGE_RAY_TRACING = 0x8,
	RI_BARRIER_STAGE_DRAW_INDIRECT = 0x10,
	RI_BARRIER_STAGE_COPY = 0x20,
	RI_BARRIER_STAGE_BLIT = 0x40,
	RI_BARRIER_STAGE_CLEAR = 0x80,
	RI_BARRIER_STAGE_ACCEL_BUILD = 0x100,
	// COLOR_ATTACHMENT_OUTPUT. Needed as a before-stage on the first barrier after swapchain acquire:
	// the acquire semaphore is waited at that stage, and an UNDEFINED before-state alone derives no
	// src stage to chain to it.
	RI_BARRIER_STAGE_COLOR_ATTACHMENT = 0x200,
	RI_BARRIER_STAGE_HOST = 0x400,

	RI_BARRIER_STAGE_ALL_GRAPHICS = RI_BARRIER_STAGE_VERTEX | RI_BARRIER_STAGE_FRAGMENT,
	RI_BARRIER_STAGE_ALL_SHADER = RI_BARRIER_STAGE_VERTEX | RI_BARRIER_STAGE_FRAGMENT |
	                              RI_BARRIER_STAGE_COMPUTE | RI_BARRIER_STAGE_RAY_TRACING,
};

enum RIBarrierAspect_e {
	RI_BARRIER_ASPECT_COLOR,
	RI_BARRIER_ASPECT_DEPTH,
	RI_BARRIER_ASPECT_STENCIL,
	RI_BARRIER_ASPECT_DEPTH_STENCIL,
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
