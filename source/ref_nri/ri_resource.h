#ifndef RI_RESOURCE_H
#define RI_RESOURCE_H

#include "ri_types.h"

#if DEVICE_IMPL_VULKAN
//VkResult RI_VK_InitImageView( struct RIDevice_s *dev, VkImageViewCreateInfo *info, struct RIDescriptor_s *desc, VkDescriptorType type );

#define RI_VK_DESCRIPTOR_IS_IMAGE( desc ) ( desc.vk.type == VK_DESCRIPTOR_TYPE_SAMPLER || desc.vk.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || desc.vk.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE )

static inline void RI_VK_FillColorAttachment(VkRenderingAttachmentInfo* info, struct RIDescriptor_s* desc, bool attachAndClear) {
	info->imageView = desc->vk.image.imageView;
	info->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
}

static inline void RI_VK_FillDepthAttachment(VkRenderingAttachmentInfo* info, struct RIDescriptor_s* desc, bool attachAndClear) {
	info->imageView = desc->vk.image.imageView;
	info->imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	info->clearValue.depthStencil.depth = 1.0f;
}

#endif


#endif
