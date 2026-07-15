#ifndef RI_COMMAND_H
#define RI_COMMAND_H

// Command submission + scheduling types: queues, command pools/buffers, the command ring, the
// deferred-free list, the timeline semaphore, the viewport/scissor geometry used at record time, and
// the barrier/copy descriptors recorded into a command buffer.

#include "ri_prelude.h"
#include "ri_resource.h" // RIResourceState_e / RIBarrierStages_e, RIBuffer_s / RITexture_s

enum RIQueueType_e {
	RI_QUEUE_GRAPHICS,
	RI_QUEUE_COMPUTE,
	RI_QUEUE_COPY,
	RI_QUEUE_LEN
};

enum RIFreeType_e {
	RI_FREE_UNKNOWN = 0,
	RI_FREE_VK_START = 0,
	RI_FREE_VK_IMAGE,
	RI_FREE_VK_IMAGEVIEW,
	RI_FREE_VK_SAMPLER,
	RI_FREE_VK_VMA_AllOC,
	RI_FREE_VK_BUFFER,
	RI_FREE_VK_BUFFER_VIEW,
	RI_FREE_VK_END,
};

struct RIFree_s {
	uint8_t type; // enum r_frame_free_list_e
	union {
#if ( DEVICE_IMPL_VULKAN )
		VkCommandBuffer vkCmdBuffer;
		VkImage vkImage;
		VkImageView vkImageView;
		VkBuffer vkBuffer;
		VkSampler vkSampler;
		VkBufferView vkBufferView;
		struct VmaAllocation_T *vmaAlloc;
#endif
	};
};

struct RIRect_s {
	int16_t x;
	int16_t y;
	int16_t width;
	int16_t height;
};

struct RIViewport_s {
	float x;
	float y;
	float width;
	float height;
	float depthMin;
	float depthMax;
	bool originBottomLeft; // expects "isViewportOriginBottomLeftSupported"
};

// Barrier descriptors. `before`/`after` are RIResourceState_e bit sets; the layout, access mask and
// pipeline stages are derived from them, so a transition is spelled as intent rather than masks.
// `beforeStages`/`afterStages` are RIBarrierStages_e bit sets that narrow the derived stage mask;
// RI_BARRIER_STAGE_NONE (0) means "derive conservatively from the state".

struct RIImageBarrier_s {
	const struct RITexture_s *texture;
	uint32_t before, after;             // RIResourceState_e bits
	uint32_t beforeStages, afterStages; // RIBarrierStages_e bits; 0 => derive from state
	uint8_t aspect;                     // RIBarrierAspect_e
	uint16_t baseMip, mipCount;         // mipCount 0 => remaining mips
	uint16_t baseLayer, layerCount;     // layerCount 0 => remaining layers
};

struct RIBufferBarrier_s {
	const struct RIBuffer_s *buffer;
	uint32_t before, after;             // RIResourceState_e bits
	uint32_t beforeStages, afterStages; // RIBarrierStages_e bits; 0 => derive from state
	uint64_t offset;
	uint64_t size; // 0 => whole buffer
};

// Global execution + memory barrier, carrying no resource handle.
struct RIMemoryBarrier_s {
	uint32_t before, after;             // RIResourceState_e bits
	uint32_t beforeStages, afterStages; // RIBarrierStages_e bits; 0 => derive from state
};

struct RIBarrierGroupDesc_s {
	const struct RIMemoryBarrier_s *memoryBarriers;
	size_t numMemoryBarriers;
	const struct RIBufferBarrier_s *bufferBarriers;
	size_t numBufferBarriers;
	const struct RIImageBarrier_s *imageBarriers;
	size_t numImageBarriers;
};

// Copy one texture subresource into a buffer region. `src` must already be in RI_RESOURCE_STATE_COPY_SRC;
// the caller owns the surrounding barriers. bufferRowLength/bufferImageHeight are in texels, 0 = tightly
// packed.
struct RICopyTextureToBufferDesc_s {
	const struct RITexture_s *src;
	const struct RIBuffer_s *dst;
	uint64_t bufferOffset;
	uint32_t bufferRowLength;
	uint32_t bufferImageHeight;
	uint32_t mipLevel;
	uint32_t baseArrayLayer;
	uint32_t layerCount; // 0 => 1
	uint8_t aspect;      // RIBarrierAspect_e
	int32_t x, y, z;
	uint32_t width, height;
	uint32_t depth; // 0 => 1
};

struct RIPool_s {
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkQueue queue;
			VkCommandPool pool;
		} vk;
#endif
	};
};

struct RICmd_s {
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkCommandBuffer cmd;
		} vk;
#endif
	};
};

#define RI_COMMAND_RING_POOL_COUNT 8
#define RI_COMMAND_RING_CMD_PER_POOL 32

struct RICommandRingElement_s {
	struct RICmd_s *cmds;
	uint32_t numCmds;
	struct RIPool_s *pool;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkSemaphore semaphore;
			VkFence fence;
		} vk;
#endif
	};
};

struct RICommandRingBuffer_s {
	uint32_t poolIndex;
	uint32_t cmdIndex;
	uint32_t fenceIndex;

	uint32_t poolCount;
	uint32_t cmdPerPool;
	bool syncPrimitive;

	struct RIPool_s pools[RI_COMMAND_RING_POOL_COUNT];
	struct RICmd_s cmds[RI_COMMAND_RING_POOL_COUNT][RI_COMMAND_RING_CMD_PER_POOL];

	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkFence fences[RI_COMMAND_RING_POOL_COUNT][RI_COMMAND_RING_CMD_PER_POOL];
			VkSemaphore semaphores[RI_COMMAND_RING_POOL_COUNT][RI_COMMAND_RING_CMD_PER_POOL];
		} vk;
#endif
	};
};

struct RIQueue_s {
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkQueueFlags queueFlags;
			uint16_t queueFamilyIdx;
			uint16_t slotIdx;
			VkQueue queue;
		} vk;
#endif
	};
};

struct RITimeline_s {
	uint64_t signalValue; // CPU mirror of the last value handed to a GPU submit
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkSemaphore semaphore;
		} vk;
#endif
	};
};

#endif
