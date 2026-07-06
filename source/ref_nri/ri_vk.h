#ifndef RI_VK_H
#define RI_VK_H

#include "ri_types.h"
#include "ri_format.h"

#if DEVICE_IMPL_VULKAN
#define RI_VK_DESCRIPTOR_IS_IMAGE( desc ) ( ( desc ).type == RI_DESCRIPTOR_TYPE_SAMPLER || ( desc ).type == RI_DESCRIPTOR_TYPE_STORAGE_IMAGE || ( desc ).type == RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE )

// Backend-neutral RIDescriptorType_e -> VkDescriptorType, applied only at descriptor-set write time.
static inline VkDescriptorType RI_VK_BindlessDescriptorType( uint8_t type )
{
	switch( (enum RIDescriptorType_e)type ) {
		case RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case RI_DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case RI_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case RI_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case RI_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case RI_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

static inline VkImageLayout RI_VK_ResourceStateToImageLayout( enum RIResourceState_e state )
{
	switch( state ) {
		case RI_RESOURCE_STATE_SHADER_RESOURCE: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case RI_RESOURCE_STATE_UNORDERED_ACCESS: return VK_IMAGE_LAYOUT_GENERAL;
	}
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

static inline void RI_VK_FillColorAttachment( VkRenderingAttachmentInfo *info, struct RITextureView_s view, bool attachAndClear )
{
	info->imageView = view.vk.image;
	info->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
}

static inline void RI_VK_FillDepthAttachment( VkRenderingAttachmentInfo *info, struct RITextureView_s view, bool attachAndClear )
{
	info->imageView = view.vk.image;
	info->imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	info->clearValue.depthStencil.depth = 1.0f;
}

const VkFormat RIFormatToVK(uint32_t format);
const enum RI_Format_e VKToRIFormat(VkFormat);  

#endif

#endif
