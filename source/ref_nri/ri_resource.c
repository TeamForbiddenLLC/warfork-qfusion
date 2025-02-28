#include "ri_resource.h"
#include "qhash.h"
#include "ri_conversion.h"

#include "ri_format.h"
#include "ri_types.h"

#if DEVICE_IMPL_VULKAN
VkResult RI_VK_InitImageView( struct RIDevice_s *dev, VkImageViewCreateInfo *info, struct RIDescriptor_s *desc )
{
	assert( dev->renderer->api == RI_DEVICE_API_VK );
	desc->vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	desc->vk.image.handle = info->image;
	VkResult result = vkCreateImageView( dev->vk.device, info, NULL, &desc->vk.image.info.imageView );
	RI_UpdateDescriptorCookie(dev, desc);
	return result;
}

VkResult RI_VK_InitSampler( struct RIDevice_s *dev, VkSamplerCreateInfo *info, struct RIDescriptor_s *desc )
{
	assert( dev->renderer->api == RI_DEVICE_API_VK );
	desc->vk.type = VK_DESCRIPTOR_TYPE_SAMPLER;
	VkResult result = vkCreateSampler( dev->vk.device, info, NULL, &desc->vk.image.info.sampler );
	RI_UpdateDescriptorCookie(dev, desc);
	return result;
}
#endif

void RI_UpdateDescriptorCookie(struct RIDevice_s* dev,struct RIDescriptor_s* desc) {
	GPU_VULKAN_BLOCK( dev->renderer, ( {
						  switch( desc->vk.type ) {
							  case VK_DESCRIPTOR_TYPE_SAMPLER:
							  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
							  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
								  desc->cookie = hash_data( hash_u64( HASH_INITIAL_VALUE, desc->vk.type ), &desc->vk.image, sizeof( desc->vk.image ) );
								  break;
							  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
							  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
								  desc->cookie = hash_data( hash_u64( HASH_INITIAL_VALUE, desc->vk.type ), &desc->vk.buffer, sizeof( desc->vk.buffer ) );
								  break;
								default:
									assert(false);
									break;
						  }
					  } ) );
}

bool RI_IsEmptyDescriptor( struct RIDevice_s *dev, struct RIDescriptor_s *desc )
{
	//	GPU_VULKAN_BLOCK( dev->renderer, ( { return desc->cookie != 0; } ) );
	return desc->cookie != 0;
	//	return true;
}

void RI_FreeRIDescriptor( struct RIDevice_s *dev, struct RIDescriptor_s *desc )
{
	GPU_VULKAN_BLOCK( dev->renderer, ( {
					    switch( desc->vk.type ) {
							  case VK_DESCRIPTOR_TYPE_SAMPLER:
							  	if(desc->vk.image.info.sampler)
					  		  	vkDestroySampler( dev->vk.device, desc->vk.image.info.sampler, NULL );
									break;
								case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
							  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
							  	if(desc->vk.image.info.sampler)
					  		  	vkDestroySampler( dev->vk.device, desc->vk.image.info.sampler, NULL );
							  	if(desc->vk.image.info.imageView)
					  		  	vkDestroyImageView( dev->vk.device, desc->vk.image.info.imageView, NULL );
									break;
					  	  default:
					  		  break;
					    }
					  } ) );
	memset( desc, 0, sizeof( struct RIDescriptor_s ) );
}

//#if DEVICE_IMPL_VULKAN
//
//  static inline void __vk_fillQueueFamilies(struct RIDevice_s* dev, uint32_t* queueFamilies, uint32_t* queueFamiliesIdx, size_t reservedLen) {
//	  uint32_t uniqueQueue = 0;
//	  for( size_t i = 0; i < RI_QUEUE_LEN; i++ ) {
//		  if( dev->queues[i].vk.queue ) {
//			  const uint32_t queueBit = ( 1 << dev->queues[i].vk.queueFamilyIdx );
//			  if( ( uniqueQueue & queueBit ) > 0 ) {
//				  assert(*queueFamiliesIdx < reservedLen);
//				  queueFamilies[(*queueFamiliesIdx)++] = dev->queues[i].vk.queueFamilyIdx;
//			  
//			  }
//			  uniqueQueue |= queueBit;
//		  }
//	  }
//  }
//
//#endif
//
//int InitRITexture( struct RIDevice_s *dev, const struct RITextureDesc_s *desc, struct RITextureHandle_s *tex, struct RIMemory_s* mem)
//{
//	GPU_VULKAN_BLOCK( dev->renderer, ( {
//						  tex->height = desc->height;
//						  tex->width = desc->width;
//						  tex->type = desc->type;
//						  tex->sampleNum = desc->sampleNum;
//						  tex->mipNum = desc->mipNum;
//						  tex->depth = desc->depth;
//						  uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
//						  VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
//						  VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT; // typeless
//						  const struct RIFormatProps_s *formatProps = GetRIFormatProps( desc->format );
//						  if( formatProps->blockWidth > 1 )
//							  flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
//						  if( desc->layerNum >= 6 && desc->width == desc->height )
//							  flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
//						  if( desc->type == RI_TEXTURE_3D )
//							  flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT; // allow 3D demotion to a set of layers // TODO: hook up "VK_EXT_image_2d_view_of_3d"?
//						  // if (m_Desc.sampleLocationsTier && formatProps.isDepth)
//						  //     flags |= VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT;
//
//						  info.flags = flags;
//						  info.imageType = ri_vk_RITextureTypeToVKImageType( desc->type );
//						  info.format = RIFormatToVK( desc->format );
//						  info.extent.width = desc->width;
//						  info.extent.height = Q_MAX( desc->height, 1 );
//						  info.extent.depth = Q_MAX( desc->depth, 1 );
//						  info.mipLevels = Q_MAX( desc->mipNum, 1 );
//						  info.arrayLayers = Q_MAX( desc->layerNum, 1 );
//						  info.samples = Q_MAX( desc->sampleNum, 1 );
//						  info.tiling = VK_IMAGE_TILING_OPTIMAL;
//
//						  info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//						  if( desc->usageBits & RI_USAGE_SHADER_RESOURCE )
//							  info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
//						  if( desc->usageBits & RI_USAGE_SHADER_RESOURCE_STORAGE )
//							  info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
//						  if( desc->usageBits & RI_USAGE_COLOR_ATTACHMENT )
//							  info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//						  if( desc->usageBits & RI_USAGE_DEPTH_STENCIL_ATTACHMENT )
//							  info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
//						  if( desc->usageBits & RI_USAGE_SHADING_RATE )
//							  info.usage |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
//
//						  info.pQueueFamilyIndices = queueFamilies;
//						  __vk_fillQueueFamilies( dev, queueFamilies, &info.queueFamilyIndexCount, RI_QUEUE_LEN );
//
//						  info.sharingMode = ( info.queueFamilyIndexCount > 0 ) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE; // TODO: still no DCC on AMD with concurrent?
//						  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//						  VkResult result = vkCreateImage( dev->vk.device, &info, NULL, &tex->vk.image );
//							if( VK_WrapResult( result ) ) {
//							  return RI_FAIL;
//						  }
//
//							// allocate vma and bind dedicated VMA
//							VmaAllocationCreateInfo mem_reqs = { 0 };
//						  mem_reqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
//						  mem_reqs.usage = (VmaMemoryUsage)VMA_MEMORY_USAGE_GPU_ONLY;
//						  vmaAllocateMemoryForImage( dev->vk.vmaAllocator, tex->vk.image, &mem_reqs, &mem->vk.vmaAlloc, NULL );
//							vmaBindImageMemory2(dev->vk.vmaAllocator, mem->vk.vmaAlloc, 0, tex->vk.image, NULL);	
//						  
//						  return RI_SUCCESS;
//					  } ) );
//	return RI_INCOMPLETE_DEVICE;
//}
//void FreeRITexture( struct RIDevice_s *dev, struct RITextureHandle_s *tex ) {}
//
//void FreeRIMemory(struct RIDevice_s* dev, struct RIMemory_s* mem) {
//	GPU_VULKAN_BLOCK( dev->renderer, ( {
//		if(mem->vk.vmaAlloc) {
//			vmaFreeMemory(dev->vk.vmaAllocator, mem->vk.vmaAlloc);
//		}
//	} ) );
//}
//
//int InitRIBuffer( struct RIDevice_s *dev, const struct RIBufferDesc_s *desc, struct RIBuffer_s *buf, struct RIMemory_s *mem )
//{
//	GPU_VULKAN_BLOCK( dev->renderer, ( {
//						  uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
//						  VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
//						  info.size = desc->size;
//						  info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
//						  if( dev->physicalAdapter.vk.isBufferDeviceAddressSupported )
//							  info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
//						  if( desc->usageBits & RI_BUFFER_USAGE_VERTEX_BUFFER )
//							  info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_INDEX_BUFFER )
//							  info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_CONSTANT_BUFFER )
//							  info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_ARGUMENT_BUFFER )
//							  info.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_SCRATCH )
//							  info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_BINDING_TABLE )
//							  info.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE )
//							  info.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPT )
//							  info.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_SHADER_RESOURCE )
//							  info.usage |= desc->structureStride > 0 ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
//
//						  if( desc->usageBits & RI_BUFFER_USAGE_SHADER_RESOURCE_STORAGE )
//							  info.usage |= desc->structureStride > 0 ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
//
//						  info.pQueueFamilyIndices = queueFamilies;
//						  __vk_fillQueueFamilies( dev, queueFamilies, &info.queueFamilyIndexCount, RI_QUEUE_LEN );
//						  info.sharingMode = ( info.queueFamilyIndexCount > 0 ) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE; // TODO: still no DCC on AMD with concurrent?
//
//						  VkResult result = vkCreateBuffer( dev->vk.device, &info, NULL, &buf->vk.buffer );
//						  if( VK_WrapResult( result ) ) {
//							  return RI_FAIL;
//						  }
//
//						  VmaAllocationCreateInfo mem_reqs = { 0 };
//						  mem_reqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
//						  mem_reqs.usage = (VmaMemoryUsage)VMA_MEMORY_USAGE_GPU_ONLY;
//						  vmaAllocateMemoryForBuffer( dev->vk.vmaAllocator, buf->vk.buffer, &mem_reqs, &mem->vk.vmaAlloc, NULL );
//						  vmaBindBufferMemory2( dev->vk.vmaAllocator, mem->vk.vmaAlloc, 0, buf->vk.buffer, NULL );
//
//						  return RI_SUCCESS;
//					  } ) );
//	return RI_INCOMPLETE_DEVICE;
//}
//
//void FreeRIBuffer(struct RIDevice_s* dev, struct RIBuffer_s *buf) {
//
//}

