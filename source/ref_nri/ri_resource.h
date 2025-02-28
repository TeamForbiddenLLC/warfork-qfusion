#ifndef RI_RESOURCE_H
#define RI_RESOURCE_H

#include "ri_types.h"


#if DEVICE_IMPL_VULKAN
VkResult RI_VK_InitImageView( struct RIDevice_s *dev, VkImageViewCreateInfo *info, struct RIDescriptor_s *desc );
VkResult RI_VK_InitSampler( struct RIDevice_s *dev, VkSamplerCreateInfo *info, struct RIDescriptor_s *desc );
#endif

bool RI_IsEmptyDescriptor(struct RIDevice_s* dev,struct RIDescriptor_s* desc);
void RI_FreeRIDescriptor(struct RIDevice_s* dev,struct RIDescriptor_s* desc);
void RI_UpdateDescriptorCookie(struct RIDevice_s* dev,struct RIDescriptor_s* desc);

#endif
