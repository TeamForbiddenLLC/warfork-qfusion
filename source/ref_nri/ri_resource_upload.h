
#ifndef RI_RESOURCE_UPLOAD_H
#define RI_RESOURCE_UPLOAD_H

#define RI_RESOURCE_MAX_SETS 2
#define RI_RESOURCE_STAGE_BUFFER_SIZE ( 8 * MB_TO_BYTE )

#include "qtypes.h"
#include "ri_types.h"

/* -----------------------------------------------------------------------
 * Mapped memory range – the slice of staging memory a transaction gets
 * ----------------------------------------------------------------------- */
struct RIMappedMemoryRange {
	size_t offset; /* byte offset from the start of the backing buffer  */
	size_t size;
	void *data; /* CPU-accessible pointer to the mapped slice         */
#if ( DEVICE_IMPL_VULKAN )
	VkBuffer buffer;			   /* which VkBuffer backs this range                   */
	struct VmaAllocation_T *alloc; /* VMA allocation (NULL for stage buf)  */
#endif
};

/* -----------------------------------------------------------------------
 * Internal: temporary overflow buffer (freed on next acquire of same set)
 * ----------------------------------------------------------------------- */
struct RI_VK_TempBuffer {
#if ( DEVICE_IMPL_VULKAN )
	VkBuffer buffer;
	struct VmaAllocation_T *alloc;
#endif
};

/* -----------------------------------------------------------------------
 * Transfer command group  (mirrors TransferCommandGroup in resource_loader.zig)
 * One group = one queue + per-set cmd pool/buffer/staging/fence/semaphore
 * ----------------------------------------------------------------------- */
struct RITransferCommandGroup_s {
	struct RIQueue_s *queue;

	bool is_recording;
	size_t active_set;

	struct RIPool_s cmd_pool[RI_RESOURCE_MAX_SETS];
	struct RICmd_s cmd[RI_RESOURCE_MAX_SETS];

	/* per-set staging buffer (host-visible, persistently mapped) */
	size_t staging_buffer_offset; /* running tail within the active set's staging buf */
	struct RIBuffer_s staging_buffer[RI_RESOURCE_MAX_SETS];

	/* temporary overflow buffers – per-set, freed when the set is reused */
	struct RIBuffer_s *temporary_buffers[RI_RESOURCE_MAX_SETS]; /* stb_ds arrays */

	union {
		struct {
			/* per-set binary fence (starts signalled) + binary semaphore */
#if ( DEVICE_IMPL_VULKAN )
			VkFence fences[RI_RESOURCE_MAX_SETS];
			VkSemaphore semaphores[RI_RESOURCE_MAX_SETS];
#endif
		} vk;
	};
};

/* -----------------------------------------------------------------------
 * Resource uploader  (two groups: upload on graphics, copy on transfer)
 * ----------------------------------------------------------------------- */
struct RIResourceUploader_s {
	struct RITransferCommandGroup_s upload_resource; /* graphics queue            */
	struct RITransferCommandGroup_s copy_resource;	 /* transfer/copy queue       */

	bool is_running;
};

/* -----------------------------------------------------------------------
 * Buffer transaction
 * ----------------------------------------------------------------------- */
struct RIResourceBufferTransaction_s {
	struct RIBuffer_s target;
	size_t size;
	size_t offset; /* destination offset inside 'target'          */

	/* filled by RI_ResourceBeginCopyBuffer */
	struct RIMappedMemoryRange mapped;
};

/* -----------------------------------------------------------------------
 * Texture transaction
 * ----------------------------------------------------------------------- */
struct RIResourceTextureTransaction_s {
	struct RITexture_s target;

		union {
#if ( DEVICE_IMPL_VULKAN )
			struct {
					VkPipelineStageFlags2 current_stage;
					VkAccessFlags2 current_access;
					VkImageLayout current_layout;

					VkPipelineStageFlags2 post_stage;
					VkAccessFlags2 post_access;
					VkImageLayout post_layout;
			} vk;
#endif
		};

	/* https://github.com/microsoft/DirectXTex/wiki/Image */
	uint32_t format; /* RI_Format_e */
	uint32_t sliceNum;
	uint32_t rowPitch;

	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint32_t width;
	uint32_t height;
	uint32_t depth;

	uint32_t arrayOffset;
	uint32_t mipOffset;

	/* filled by RI_ResourceBeginCopyTexture */
	uint32_t alignRowPitch;
	uint32_t alignSlicePitch;
	struct RIMappedMemoryRange mapped;
};

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */
void RI_InitResourceUploader( struct RIDevice_s *device, struct RIResourceUploader_s *res );
void RI_FreeResourceUploader( struct RIDevice_s *device, struct RIResourceUploader_s *res );

void RI_ResourceBeginCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans );
void RI_ResourceEndCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans );

void RI_ResourceBeginCopyTexture( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans );
void RI_ResourceEndCopyTexture( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans );

/* Submit the upload group and advance to the next set. */
//void RI_ResourceSubmit( struct RIDevice_s *device, struct RIResourceUploader_s *res );

struct RIResourceUploaderVKResult_s {
	bool signaled;
	
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkFence fence;
			VkSemaphore semaphore;
		} vk;
#endif
	};
};

#if ( DEVICE_IMPL_VULKAN )
struct RIResourceUploaderVKResult_s RI_VKFlushResourceUpdate( struct RIDevice_s *device, struct RIResourceUploader_s *res, size_t num_semaphores, VkSemaphoreSubmitInfo *wait_semaphore_info );
#endif

#endif
