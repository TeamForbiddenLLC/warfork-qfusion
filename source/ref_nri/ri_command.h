#ifndef RI_COMMAND_H
#define RI_COMMAND_H

// Command submission + scheduling types: queues, command pools/buffers, the command ring, the
// deferred-free list, the timeline semaphore, and the viewport/scissor geometry used at record time.

#include "ri_prelude.h"

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
