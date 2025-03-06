
#ifndef RI_RESOURCE_UPLOAD_H
#define RI_RESOURCE_UPLOAD_H

#define RI_RESOURCE_NUM_COMMAND_SETS 4
#define RI_RESOURCE_STAGE_SIZE (8 * MB_TO_BYTE)

#include "ri_types.h"
#include "qtypes.h"

struct RIResourceReq {
	uint64_t byteOffset;
	void *cpuMapping;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkBuffer buffer;
    	struct VmaAllocation_T* alloc;
		} vk;
#endif
	};
};

struct RIResourcePostImageBarrier_s {
	VkImage image;
	struct RIBarrierImageHandle_s postBarrier; 
};

struct RIResourcePostBufferBarrier_s {
	VkBuffer buffer;
	struct RIBarrierBufferHandle_s postBarrier;
};

struct RIResourceUploader_s {
	struct RIQueue_s *copyQueue;
	struct RIResourcePostImageBarrier_s* postImageBarriers;
	struct RIResourcePostBufferBarrier_s* postBufferBarriers;
	size_t tailOffset;
	size_t remaningSpace;
	size_t reservedSpacePerSet[RI_RESOURCE_NUM_COMMAND_SETS];
	uint32_t activeSet;
	uint32_t syncIndex;

	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			struct VmaAllocation_T *stageAlloc;
			VkBuffer stageBuffer;
			void *pMappedData;
			
			VkSemaphore uploadSem;	
			struct {
				VkCommandPool cmdPool;
				VkCommandBuffer cmdBuffer;
				struct RI_VK_TempBuffers {
					VkBuffer buffer;
					struct VmaAllocation_T *alloc;
				} *temporary;
			} cmdSets[RI_RESOURCE_NUM_COMMAND_SETS];
		} vk;
#endif
	};
};

void RI_InitResourceUploader( struct RIDevice_s *device, struct RIResourceUploader_s *resource );

struct RIResourceBufferTransaction_s {
	struct RIBufferHandle_s target;

	struct RIBarrierBufferHandle_s srcBarrier;
	struct RIBarrierBufferHandle_s postBarrier;

	size_t size;
	size_t offset;

	// begin mapping
	void *data;
	struct RIResourceReq req;
};

void RI_ResourceBeginCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans );
void RI_ResourceEndCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans );

struct RIResourceTextureTransaction_s {
	struct RITextureHandle_s target;

	struct RIBarrierImageHandle_s srcBarrier;
	struct RIBarrierImageHandle_s postBarrier;

	// https://github.com/microsoft/DirectXTex/wiki/Image
	uint32_t format; // RI_Format_e 
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

	// begin mapping
	void *data;
	uint32_t alignRowPitch;
	uint32_t alignSlicePitch;
	struct RIResourceReq req;
};

void RI_ResourceBeginCopyTexture(struct RIDevice_s* device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans );
void RI_ResourceEndCopyTexture(struct RIDevice_s* device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans );

void RI_InsertTransitionBarriers(struct RIDevice_s* device, struct RIResourceUploader_s *res, struct RICmd_s* cmd);
void RI_ResourceSubmit(struct RIDevice_s* device, struct RIResourceUploader_s *res);

//struct RIBarrierBufferHandle_s  RI_VertexBufferBarrier( struct RIDevice_s *device );
//struct RIBarrierBufferHandle_s  RI_IndexBufferBarrier( struct RIDevice_s *device );
//struct RIBarrierImageHandle_s RI_SampledImageImageBarrier( struct RIDevice_s *device );

#endif
