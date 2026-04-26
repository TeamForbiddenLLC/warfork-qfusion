#ifndef RI_RENDERER_H
#define RI_RENDERER_H

#include "ri_types.h"

struct RIDeviceDesc_s {
	struct RIPhysicalAdapter_s *physicalAdapter;
};
int InitRIRenderer( const struct RIBackendInit_s *init, struct RIRenderer_s *renderer );
void ShutdownRIRenderer( struct RIRenderer_s *renderer );

int EnumerateRIAdapters( struct RIRenderer_s *renderer, struct RIPhysicalAdapter_s *adapters, uint32_t *numAdapters );
int InitRIDevice( struct RIRenderer_s *renderer, struct RIDeviceDesc_s *init, struct RIDevice_s *device );

void WaitRIQueueIdle( struct RIDevice_s *device, struct RIQueue_s *queue );

int FreeRIDevice( struct RIDevice_s *dev );
void FreeRIFree( struct RIDevice_s *dev, struct RIFree_s *mem );

// RIDescriptor
void UpdateRIDescriptor( struct RIDevice_s *dev, struct RIDescriptor_s *desc ); // after configure an RIDescriptor call update to configure it
void FreeRIDescriptor( struct RIDevice_s *dev, struct RIDescriptor_s *desc );
struct RITextureView_s TextureviewRIDescriptor(struct RIDescriptor_s* desc); 
static inline bool RI_IsEmptyDescriptor( struct RIDescriptor_s *desc )
{
	return desc->cookie == 0;
}


// RITexture
void FreeRITexture( struct RIDevice_s *dev, struct RITexture_s *tex );
void FreeRITextureView( struct RIDevice_s *dev, struct RITextureView_s *view );

// RICmd
void FreeRICmd( struct RIDevice_s *dev, struct RICmd_s *cmd, struct RIPool_s *pool );
void BeginRICmd( struct RIDevice_s *dev, struct RICmd_s *cmd );
void EndRICmd( struct RIDevice_s *dev, struct RICmd_s *cmd );
void InitRICmd( struct RIDevice_s *dev, struct RIPool_s * pool, struct RICmd_s *cmd  );

void InitRIPool( struct RIDevice_s *dev, struct RIPool_s *pool, struct RIQueue_s *queue );
void ResetRIPool( struct RIDevice_s *dev, struct RIPool_s *pool );
void FreeRIPool( struct RIDevice_s *dev, struct RIPool_s *pool );

// RICommandRingBuffer
void InitRICommandRingBuffer( struct RIDevice_s *dev, struct RIQueue_s *queue, struct RICommandRingBuffer_s *ring, bool syncPrimitives );
void FreeRICommandRingBuffer( struct RIDevice_s *dev, struct RICommandRingBuffer_s *ring );
void AdvanceRICommandRingBuffer( struct RICommandRingBuffer_s *ring );
struct RICommandRingElement_s GetRICommandRingElement( struct RIDevice_s *dev, struct RICommandRingBuffer_s *ring, uint32_t numCmds );
void WaitRICommandRingElement( struct RIDevice_s *dev, struct RICommandRingElement_s *element );
#if DEVICE_IMPL_VULKAN
void VK_ConfigureBufferQueueFamilies( VkBufferCreateInfo *info, struct RIQueue_s *queues, size_t numQueues, uint32_t *queueFamiliesIdx, size_t reservedLen );
void VK_ConfigureImageQueueFamilies( VkImageCreateInfo *info, struct RIQueue_s *queues, size_t numQueues, uint32_t *queueFamiliesIdx, size_t reservedLen );
void vk_fillQueueFamilies( struct RIDevice_s *dev, uint32_t *queueFamilies, uint32_t *queueFamiliesIdx, size_t reservedLen );
#endif

#endif
