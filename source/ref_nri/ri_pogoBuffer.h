#ifndef RI_POGO_BUFFER_H
#define RI_POGO_BUFFER_H

#include "ri_types.h"

struct FrameState_s;

struct RI_PogoBuffer {
	uint16_t attachmentIndex;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			struct RITexture_s textures[2];
			struct RITextureView_s views[2];
		} vk;
#endif
#if ( DEVICE_IMPL_MTL )
		struct {
			struct RITexture_s textures[2];
			struct RITextureView_s views[2];
		} mtl;
#endif
	};
};

void RI_PogoBufferInit( struct RIDevice_s *device, struct RI_PogoBuffer *pogo, uint32_t width, uint32_t height, uint32_t format );
void RI_PogoBufferDestroy( struct RIDevice_s *device, struct RI_PogoBuffer *pogo );
void RI_PogoBufferToggle( struct RIDevice_s *device, struct RI_PogoBuffer *pogo, struct RICmd_s *handle );

#if ( DEVICE_IMPL_VULKAN )
VkImageMemoryBarrier2 VK_RI_PogoShaderMemoryBarrier2( VkImage image, bool initial );
VkImageMemoryBarrier2 VK_RI_PogoAttachmentMemoryBarrier2( VkImage image, bool initial );
#endif

static inline struct RITextureView_s *RI_PogoBufferAttachment( struct RI_PogoBuffer *pogo )
{
#if ( DEVICE_IMPL_VULKAN )
	return &pogo->vk.views[pogo->attachmentIndex];
#elif ( DEVICE_IMPL_MTL )
	return &pogo->mtl.views[pogo->attachmentIndex];
#else
	return NULL;
#endif
}

static inline struct RITextureView_s *RI_PogoBufferShaderResource( struct RI_PogoBuffer *pogo )
{
#if ( DEVICE_IMPL_VULKAN )
	return &pogo->vk.views[( pogo->attachmentIndex + 1 ) % 2];
#elif ( DEVICE_IMPL_MTL )
	return &pogo->mtl.views[( pogo->attachmentIndex + 1 ) % 2];
#else
	return NULL;
#endif
}

#endif
