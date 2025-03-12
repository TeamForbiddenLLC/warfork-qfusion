#ifndef RI_RENDERER_H
#define RI_RENDERER_H

#include "ri_types.h"

static inline uint32_t RIGetQueueFlags(struct RIRenderer_s* renderer,const struct RIQueue_s* queue) {
#if ( DEVICE_IMPL_VULKAN )
    return (queue->vk.queueFlags & VK_QUEUE_GRAPHICS_BIT ? RI_QUEUE_GRAPHICS_BIT : 0) |
          (queue->vk.queueFlags  & VK_QUEUE_COMPUTE_BIT ? RI_QUEUE_COMPUTE_BIT  : 0) |
          (queue->vk.queueFlags & VK_QUEUE_TRANSFER_BIT ? RI_QUEUE_TRANSFER_BIT  : 0) | 
          (queue->vk.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? RI_QUEUE_SPARSE_BINDING_BIT  : 0) |  
          (queue->vk.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ? RI_QUEUE_VIDEO_DECODE_BIT : 0) |
          (queue->vk.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ? RI_QUEUE_VIDEO_ENCODE_BIT  : 0) |
          (queue->vk.queueFlags & VK_QUEUE_PROTECTED_BIT ? RI_QUEUE_PROTECTED_BIT  : 0) |
          (queue->vk.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV ? RI_QUEUE_OPTICAL_FLOW_BIT_NV : 0);
#endif
	return 0;
}

struct RIDeviceDesc_s {
          struct RIPhysicalAdapter_s *physicalAdapter;
};
int InitRIRenderer( const struct RIBackendInit_s *init, struct RIRenderer_s *renderer );
void ShutdownRIRenderer( struct RIRenderer_s *renderer );

int EnumerateRIAdapters( struct RIRenderer_s *renderer, struct RIPhysicalAdapter_s *adapters, uint32_t *numAdapters );

int InitRIDevice( struct RIRenderer_s *renderer, struct RIDeviceDesc_s *init, struct RIDevice_s *device );

void WaitRIQueueIdle( struct RIDevice_s *device, struct RIQueue_s *queue );

int FreeRIDevice( struct RIDevice_s *dev );
void FreeRIFree(struct RIDevice_s* dev,struct RIFree_s* mem);

// RIDescriptor
void RI_UpdateDescriptor(struct RIDevice_s* dev,struct RIDescriptor_s* desc);
void FreeRIDescriptor(struct RIDevice_s* dev, struct RIDescriptor_s* desc);
static inline bool RI_IsEmptyDescriptor( struct RIDescriptor_s *desc ) { return desc->cookie == 0;}

#if DEVICE_IMPL_VULKAN
void VK_ConfigureBufferQueueFamilies( VkBufferCreateInfo *info, struct RIQueue_s *queues, size_t numQueues, uint32_t *queueFamiliesIdx, size_t reservedLen );
void VK_ConfigureImageQueueFamilies( VkImageCreateInfo *info, struct RIQueue_s *queues, size_t numQueues, uint32_t *queueFamiliesIdx, size_t reservedLen );
void vk_fillQueueFamilies( struct RIDevice_s *dev, uint32_t *queueFamilies, uint32_t *queueFamiliesIdx, size_t reservedLen );
#endif



#endif

