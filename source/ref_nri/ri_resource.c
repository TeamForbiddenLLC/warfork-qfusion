#include "ri_resource.h"
#include "ri_types.h"

#include "ri_renderer.h"
#if DEVICE_IMPL_VULKAN
//VkResult RI_VK_InitImageView( struct RIDevice_s *dev, VkImageViewCreateInfo *info, struct RIDescriptor_s *desc, VkDescriptorType type )
//{
//	assert( info->image != VK_NULL_HANDLE );
//	assert( dev->renderer->api == RI_DEVICE_API_VK );
//	desc->vk.type = type; //VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
//	desc->vk.image.handle = info->image;
//	desc->flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
//	desc->vk.image.info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//	VkResult result = vkCreateImageView( dev->vk.device, info, NULL, &desc->vk.image.info.imageView );
//	VK_WrapResult( result );
//	RI_UpdateDescriptor( dev, desc );
//	return result;
//}
#endif

