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

	// Metal deferred-free kinds. A Metal free is just a release of the wrapped object; the type selects
	// which union arm below carries the handle.
	RI_FREE_MTL_START,
	RI_FREE_MTL_TEXTURE,
	RI_FREE_MTL_BUFFER,
	RI_FREE_MTL_END,
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
#if ( DEVICE_IMPL_MTL )
		struct mtlc_buffer mtlBuffer;
		struct mtlc_texture mtlTexture;
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

// Dynamic-rendering attachment: a view plus whether to clear it on load. Color clears to clearColor
// (zero-initialized => transparent black), depth always clears to 1.0.
struct RIAttachmentDesc_s {
	struct RITextureView_s view;
	bool clear; // true => loadAction CLEAR, false => LOAD
	float clearColor[4]; // color attachments only
};

// A dynamic render pass (VK dynamic rendering / Metal MTLRenderPassDescriptor + encoder). Recorded with
// RICmdBeginRendering .. draws .. RICmdEndRendering.
struct RIRenderingDesc_s {
	uint16_t width, height;
	uint8_t colorNum;
	struct RIAttachmentDesc_s colors[8];
	bool hasDepth;
	struct RIAttachmentDesc_s depth;
};

struct RIPool_s {
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkQueue queue;
			VkCommandPool pool;
		} vk;
#endif
#if ( DEVICE_IMPL_MTL )
		struct {
			// Metal has no command-pool object; command buffers come straight from the queue. Keep the
			// owning queue so InitRICmd can vend a command buffer, mirroring the VK pool's queue field.
			struct mtlc_command_queue queue;
		} mtl;
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
#if ( DEVICE_IMPL_MTL )
		struct {
			struct mtlc_command_buffer cmd;
			struct mtlc_render_command_encoder encoder; // active encoder between Begin/End rendering; nil otherwise
			// Bumped every time a new encoder starts. Residency declared with useResource: is scoped to one
			// encoder, so anything caching "already made resident" has to key on this rather than on the
			// frame or the command buffer.
			uint64_t encoderEpoch;
		} mtl;
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
#if ( DEVICE_IMPL_MTL )
		struct {
			// Pacing is done with the command buffer's completion status / a shared-event value rather
			// than a fence+semaphore pair. `cmd` is the submitted buffer whose completion this element
			// waits on; `submitValue` is its shared-event signal value.
			struct mtlc_command_buffer cmd;
			uint64_t submitValue;
		} mtl;
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
#if ( DEVICE_IMPL_MTL )
		struct {
			// Metal paces on command-buffer completion + a single shared event instead of per-element
			// fence/semaphore grids. sharedEvent is an MTLSharedEvent (metal-c binding added later).
			void *sharedEvent;
			uint64_t submitValues[RI_COMMAND_RING_POOL_COUNT][RI_COMMAND_RING_CMD_PER_POOL];
		} mtl;
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
#if ( DEVICE_IMPL_MTL )
		struct {
			struct mtlc_command_queue queue;
			uint8_t queueType; // RIQueueType_e this slot fronts (Metal queues are not typed)
		} mtl;
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
#if ( DEVICE_IMPL_MTL )
		struct {
			void *sharedEvent; // MTLSharedEvent; signaledValue mirrors the VK timeline (binding added later)
		} mtl;
#endif
	};
};

#endif
