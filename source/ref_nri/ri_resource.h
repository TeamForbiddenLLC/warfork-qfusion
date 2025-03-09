#ifndef RI_RESOURCE_H
#define RI_RESOURCE_H

#include "ri_types.h"
#include <vulkan/vulkan_core.h>


#if DEVICE_IMPL_VULKAN
VkResult RI_VK_InitImageView( struct RIDevice_s *dev, VkImageViewCreateInfo *info, struct RIDescriptor_s *desc );
VkResult RI_VK_InitSampler( struct RIDevice_s *dev, VkSamplerCreateInfo *info, struct RIDescriptor_s *desc );

static inline void RI_VK_FillColorAttachment(VkRenderingAttachmentInfo* info, struct RIDescriptor_s* desc, bool attachAndClear) {
	info->imageView = desc->vk.image.info.imageView;
	info->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
}

static inline void RI_VK_FillDepthAttachment(VkRenderingAttachmentInfo* info, struct RIDescriptor_s* desc, bool attachAndClear) {
	info->imageView = desc->vk.image.info.imageView;
	info->imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	info->clearValue.depthStencil.depth = 1.0f;
}

#endif

bool RI_IsEmptyDescriptor(struct RIDevice_s* dev,struct RIDescriptor_s* desc);
void RI_FreeRIDescriptor(struct RIDevice_s* dev,struct RIDescriptor_s* desc);
void RI_UpdateDescriptorCookie(struct RIDevice_s* dev,struct RIDescriptor_s* desc);

#endif
