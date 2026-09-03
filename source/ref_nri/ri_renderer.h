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

// RIDescriptor value builders, RI_IsEmptyDescriptor and FreeRISampler are declared in ri_descriptor.h
// (reached via ri_types.h) so every consumer sees the sret-returning prototype.

// RITexture
// Monotonic counter bumped whenever a texture, view, or buffer is destroyed. Anything that caches a
// resolved backend handle past the call that produced it -- a Metal argument buffer holds raw MTLTexture
// references, and declaring a released one resident is a use-after-free -- folds this into its cache key
// so every entry built before a destruction misses.
uint64_t RIResourceEpoch( void );

void FreeRITexture( struct RIDevice_s *dev, struct RITexture_s *tex );
void FreeRITextureView( struct RIDevice_s *dev, struct RITextureView_s *view );

// Backend-neutral render-target creation (see RITextureDesc_s), the texture counterpart of InitRIBuffer.
// The view spans the whole texture in the texture's own format: a VkImageView carrying the texture's
// usage on Vulkan, and on Metal the texture handle itself (non-owning; FreeRITextureView only clears it).
int InitRITexture( struct RIDevice_s *dev, const struct RITextureDesc_s *desc, struct RITexture_s *tex );
int InitRITextureView( struct RIDevice_s *dev, const struct RITextureDesc_s *desc, const struct RITexture_s *tex, struct RITextureView_s *view );

// Queue a texture and its view for destruction once the frames that may still reference them have
// retired. `freeList` is an stb_ds array of RIFree_s (r_frame_set_s.freeList); both handles are cleared.
// No-op for an uncreated texture, so a fresh slot can be passed unconditionally.
void RIDeferFreeTexture( struct RIFree_s **freeList, struct RITexture_s *tex, struct RITextureView_s *view );

// RIBuffer. Backend-neutral buffer creation used by the frontend (VBOs, uniform/scratch buffers) so it
// no longer creates VkBuffers directly. RIBufferMappedData returns the persistently-mapped CPU pointer
// for host-visible buffers (NULL for RI_MEMORY_DEVICE).
int InitRIBuffer( struct RIDevice_s *dev, const struct RIBufferDesc_s *desc, struct RIBuffer_s *buffer );
void FreeRIBuffer( struct RIDevice_s *dev, struct RIBuffer_s *buffer );
void *RIBufferMappedData( struct RIDevice_s *dev, struct RIBuffer_s *buffer );

// RICmd
void FreeRICmd( struct RIDevice_s *dev, struct RICmd_s *cmd, struct RIPool_s *pool );
void BeginRICmd( struct RIDevice_s *dev, struct RICmd_s *cmd );
void EndRICmd( struct RIDevice_s *dev, struct RICmd_s *cmd );
void InitRICmd( struct RIDevice_s *dev, struct RIPool_s * pool, struct RICmd_s *cmd  );

// Record resource-state transitions. Every group is batched into a single backend barrier command; any
// group may be empty, and an entirely empty desc records nothing.
void RICmdBarrier( struct RIDevice_s *dev, struct RICmd_s *cmd, const struct RIBarrierGroupDesc_s *desc );
// Single-barrier conveniences over RICmdBarrier.
void RICmdImageBarrier( struct RIDevice_s *dev, struct RICmd_s *cmd, const struct RIImageBarrier_s *barrier );
void RICmdBufferBarrier( struct RIDevice_s *dev, struct RICmd_s *cmd, const struct RIBufferBarrier_s *barrier );

void RICmdCopyTextureToBuffer( struct RIDevice_s *dev, struct RICmd_s *cmd, const struct RICopyTextureToBufferDesc_s *desc );

// Dynamic render pass. Backend-neutral replacement for the frontend's inline vkCmdBeginRendering/
// vkCmdEndRendering: VK uses dynamic rendering, Metal opens/closes a render command encoder on cmd.
void RICmdBeginRendering( struct RIDevice_s *dev, struct RICmd_s *cmd, const struct RIRenderingDesc_s *desc );
void RICmdEndRendering( struct RIDevice_s *dev, struct RICmd_s *cmd );

void InitRIPool( struct RIDevice_s *dev, struct RIPool_s *pool, struct RIQueue_s *queue );
void ResetRIPool( struct RIDevice_s *dev, struct RIPool_s *pool );
void FreeRIPool( struct RIDevice_s *dev, struct RIPool_s *pool );

// RICommandRingBuffer
void InitRICommandRingBuffer( struct RIDevice_s *dev, struct RIQueue_s *queue, struct RICommandRingBuffer_s *ring, uint32_t poolCount, uint32_t cmdPerPool, bool syncPrimitives );
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
