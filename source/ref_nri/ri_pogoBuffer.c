#include "ri_pogoBuffer.h"
#include "r_local.h"

VkImageMemoryBarrier2 VK_RI_PogoShaderMemoryBarrier2(VkImage image,bool initial) {
	VkImageMemoryBarrier2 imageBarriers = { 0 };
	imageBarriers.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imageBarriers.srcStageMask = initial ? VK_PIPELINE_STAGE_2_NONE : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	imageBarriers.srcAccessMask = initial ? VK_ACCESS_2_NONE : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	imageBarriers.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	imageBarriers.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	imageBarriers.oldLayout = initial ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageBarriers.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageBarriers.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarriers.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarriers.image = image;
	imageBarriers.subresourceRange = (VkImageSubresourceRange){
		VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS,
	};
	return imageBarriers;
}

VkImageMemoryBarrier2 VK_RI_PogoAttachmentMemoryBarrier2(VkImage image,bool initial) {
	VkImageMemoryBarrier2 imageBarriers = { 0 };
	imageBarriers.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imageBarriers.srcStageMask = initial ? VK_PIPELINE_STAGE_2_NONE : VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	imageBarriers.srcAccessMask = initial ? VK_ACCESS_2_NONE : VK_ACCESS_2_SHADER_READ_BIT;
	imageBarriers.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT ;
	imageBarriers.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT ;
	imageBarriers.oldLayout = initial ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageBarriers.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageBarriers.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarriers.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarriers.image = image;
	imageBarriers.subresourceRange = (VkImageSubresourceRange){
		VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS,
	};
	return imageBarriers;
}

void RI_PogoBufferToggle( struct RIDevice_s *device, struct RI_PogoBuffer *pogo, struct RICmd_s *handle )
{
	VkImageMemoryBarrier2 imageBarriers[2] = { 0 };
	imageBarriers[0] = VK_RI_PogoShaderMemoryBarrier2(pogo->pogoAttachment[pogo->attachmentIndex].texture->vk.image, false );
	pogo->attachmentIndex = ((pogo->attachmentIndex + 1) % 2);
	imageBarriers[1] = VK_RI_PogoAttachmentMemoryBarrier2( pogo->pogoAttachment[pogo->attachmentIndex].texture->vk.image, false );

	VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.imageMemoryBarrierCount = 2;
	dependencyInfo.pImageMemoryBarriers = imageBarriers;
	vkCmdPipelineBarrier2( handle->vk.cmd, &dependencyInfo );
}
