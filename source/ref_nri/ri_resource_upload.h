
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

struct RIResourceUploader_s {
	struct RIQueue_s *copyQueue;
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
	// allow the data to be garbage in multiple renderer instance
	//	union {
	// #if ( DEVICE_IMPL_VULKAN )
	//		struct RIResourceBufferTransaction_VK_s{
	//			VkBuffer buffer;
	//		} vk;
	// #endif
	//	};
	struct RIBufferHandle_s target;
	size_t size;
	size_t offset;
	
	// begin mapping
	void *data;
	struct RIResourceReq req;
};

void RI_ResourceBeginCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans );
void RI_ResourceEndCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans );

struct RIResourceTextureTransaction_s{

	struct RITextureHandle_s target;

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

void RI_ResourceSubmit(struct RIDevice_s* device, struct RIResourceUploader_s *res);

#endif
