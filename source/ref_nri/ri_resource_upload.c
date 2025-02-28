#include "ri_resource_upload.h"
#include "ri_types.h"
#include "qtypes.h"

#include <stb_ds.h>
#include "ri_format.h"


static void __EndActiveCommandSet( struct RIDevice_s *device, struct RIResourceUploader_s *res ) {
	GPU_VULKAN_BLOCK( device->renderer, ( {
						  vkEndCommandBuffer( res->vk.cmdSets[res->activeSet].cmdBuffer );
						  
							VkSemaphoreSubmitInfo signalSem = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
							signalSem.stageMask = VK_PIPELINE_STAGE_2_NONE;
							signalSem.value = 1 + res->syncIndex; 
							signalSem.semaphore = res->vk.uploadSem;

							VkCommandBufferSubmitInfo cmdSubmitInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
							cmdSubmitInfo.commandBuffer = res->vk.cmdSets[res->activeSet].cmdBuffer;

							VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
							submitInfo.pSignalSemaphoreInfos = &signalSem;
							submitInfo.signalSemaphoreInfoCount = 1;
							submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
							submitInfo.commandBufferInfoCount = 1;
						
							vkQueueSubmit2( res->copyQueue->vk.queue, 1, &submitInfo, VK_NULL_HANDLE);
							res->syncIndex++;
					  } ) );
}

static void __BeginNewCommandSet( struct RIDevice_s *device, struct RIResourceUploader_s *res )
{
	res->remaningSpace += res->reservedSpacePerSet[res->activeSet];
	res->reservedSpacePerSet[res->activeSet] = 0;
	GPU_VULKAN_BLOCK( device->renderer, ( {
						  VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
						  info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
						  vkBeginCommandBuffer( res->vk.cmdSets[res->activeSet].cmdBuffer, &info );
					  } ) );
}

void RI_InitResourceUploader( struct RIDevice_s *device, struct RIResourceUploader_s *resource )
{
	GPU_VULKAN_BLOCK( device->renderer, ( {
						  resource->copyQueue = &device->queues[RI_QUEUE_COPY];
						  {
							  VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
							  semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
							  VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
							  semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;
								VK_WrapResult(vkCreateSemaphore(device->vk.device, &semaphoreCreateInfo, NULL, &resource->vk.uploadSem));
							}
						  for( size_t i = 0; i < RI_RESOURCE_NUM_COMMAND_SETS; i++ ) {
							  VkCommandPoolCreateInfo cmdPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
							  cmdPoolCreateInfo.queueFamilyIndex = resource->copyQueue->vk.queueFamilyIdx;
							  cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
							  VK_WrapResult( vkCreateCommandPool( device->vk.device, &cmdPoolCreateInfo, NULL, &resource->vk.cmdSets[i].cmdPool ) );

							  VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
							  cmdAllocInfo.commandPool = resource->vk.cmdSets[i].cmdPool;
							  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
							  cmdAllocInfo.commandBufferCount = 1;
							  VK_WrapResult(vkAllocateCommandBuffers( device->vk.device, &cmdAllocInfo, &resource->vk.cmdSets[i].cmdBuffer ));
						  }
						  {
							  VkBufferCreateInfo stageBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
							  stageBufferCreateInfo.pNext = NULL;
							  stageBufferCreateInfo.flags = 0;
							  stageBufferCreateInfo.size = RI_RESOURCE_STAGE_SIZE;
							  stageBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
							  stageBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
							  stageBufferCreateInfo.queueFamilyIndexCount = 0;
							  stageBufferCreateInfo.pQueueFamilyIndices = NULL;
							  VK_WrapResult( vkCreateBuffer( device->vk.device, &stageBufferCreateInfo, NULL, &resource->vk.stageBuffer ) );

							  VmaAllocationInfo allocationInfo = {};
							  VmaAllocationCreateInfo allocInfo = {};
							  allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
							  vmaAllocateMemoryForBuffer( device->vk.vmaAllocator, resource->vk.stageBuffer, &allocInfo, &resource->vk.stageAlloc, &allocationInfo);
							  vmaBindBufferMemory2( device->vk.vmaAllocator, resource->vk.stageAlloc, 0, resource->vk.stageBuffer, NULL );
							  resource->vk.pMappedData = allocationInfo.pMappedData;
						  }
					  } ) );
	__BeginNewCommandSet( device, resource );
}

static bool __R_AllocFromStageBuffer( struct RIDevice_s *dev, struct RIResourceUploader_s *res, size_t reqSize, struct RIResourceReq *req )
{
	size_t allocSize = Q_ALIGN_TO( reqSize, 4 ); // we round up to multiples of uint32_t
	if( allocSize > res->remaningSpace ) {
		// we are out of avaliable space from staging
		return false;
	}

	// we are past the end of the buffer
	if( res->tailOffset + allocSize > RI_RESOURCE_STAGE_SIZE ) {
		const size_t remainingSpace = ( RI_RESOURCE_STAGE_SIZE - res->tailOffset ); // remaining space at the end of the buffer this unusable
		if( allocSize > res->remaningSpace - remainingSpace ) {
			return false;
		}

		res->remaningSpace -= remainingSpace;
		res->reservedSpacePerSet[res->activeSet] += remainingSpace; // give the remaning space to the requesting set
		res->tailOffset = 0;
	}

	GPU_VULKAN_BLOCK( dev->renderer, ( {
						  req->vk.alloc = res->vk.stageAlloc;
							req->cpuMapping = res->vk.pMappedData;				  
						  req->byteOffset = res->tailOffset;
						  req->vk.buffer = res->vk.stageBuffer;
					  } ) );

	res->reservedSpacePerSet[res->activeSet] += allocSize;

	res->tailOffset += allocSize;
	res->remaningSpace -= allocSize;
	return true;
}

static bool __ResolveStageBuffer( struct RIDevice_s *dev, struct RIResourceUploader_s *res, size_t reqSize, struct RIResourceReq *req )
{
	if( __R_AllocFromStageBuffer( dev, res, reqSize, req ) ) {
		return true;
	}
	GPU_VULKAN_BLOCK( dev->renderer, ( {
						  struct RI_VK_TempBuffers tempBuffer;
						  VkBufferCreateInfo stageBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
						  stageBufferCreateInfo.pNext = NULL;
						  stageBufferCreateInfo.flags = 0;
						  stageBufferCreateInfo.size = reqSize;
						  stageBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
						  stageBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
						  stageBufferCreateInfo.queueFamilyIndexCount = 0;
						  stageBufferCreateInfo.pQueueFamilyIndices = NULL;
						  VK_WrapResult( vkCreateBuffer( dev->vk.device, &stageBufferCreateInfo, NULL, &tempBuffer.buffer ) );

						  VmaAllocationInfo allocationInfo = {};
						  VmaAllocationCreateInfo allocInfo = {};
						  allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
						  vmaAllocateMemoryForBuffer( dev->vk.vmaAllocator, tempBuffer.buffer, &allocInfo, &tempBuffer.alloc, &allocationInfo );
						  vmaBindBufferMemory2( dev->vk.vmaAllocator, tempBuffer.alloc, 0, tempBuffer.buffer, NULL );

						  req->vk.alloc = tempBuffer.alloc;
							req->cpuMapping = allocationInfo.pMappedData;				  
						  req->byteOffset = 0;
							req->vk.buffer = tempBuffer.buffer;

						  arrpush( res->vk.cmdSets[res->activeSet].temporary, tempBuffer );
					  } ) );
	return true;
}


void RI_ResourceBeginCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans ) {
	__ResolveStageBuffer(device, res, trans->size, &trans->req);
	trans->data = (uint8_t *)trans->req.cpuMapping + trans->req.byteOffset;
}

void RI_ResourceEndCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans ) {
	GPU_VULKAN_BLOCK( device->renderer, ( {
						  VkBufferCopy copyBuffer = {};
						  copyBuffer.size = trans->size;
						  copyBuffer.dstOffset = trans->offset;
						  copyBuffer.srcOffset = trans->req.byteOffset;
						  vkCmdCopyBuffer( res->vk.cmdSets[res->activeSet].cmdBuffer, trans->req.vk.buffer, trans->target.vk.buffer, 1, &copyBuffer );
					  } ) );
}


void RI_ResourceBeginCopyTexture(struct RIDevice_s* device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans ) {
	const uint64_t alignedRowPitch = Q_ALIGN_TO( trans->rowPitch, device->physicalAdapter.uploadBufferTextureRowAlignment );
	const uint64_t alignedSlicePitch = Q_ALIGN_TO( trans->sliceNum * alignedRowPitch, device->physicalAdapter.uploadBufferTextureSliceAlignment );
	trans->alignRowPitch = alignedRowPitch;
	trans->alignSlicePitch = alignedSlicePitch;
	__ResolveStageBuffer(device, res, alignedSlicePitch, &trans->req);
	trans->data = (uint8_t *)trans->req.cpuMapping + trans->req.byteOffset;
}

void RI_ResourceSubmit(struct RIDevice_s* device, struct RIResourceUploader_s *res) {
		__EndActiveCommandSet(device, res);
	res->activeSet = ( res->activeSet + 1 ) % RI_RESOURCE_NUM_COMMAND_SETS;
	if( res->syncIndex >= RI_RESOURCE_NUM_COMMAND_SETS ) {
		GPU_VULKAN_BLOCK( device->renderer, ( {
							  uint64_t waitValue = 1 + res->syncIndex - RI_RESOURCE_NUM_COMMAND_SETS;
							  VkSemaphoreWaitInfo semaphoreWaitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
							  semaphoreWaitInfo.semaphoreCount = 1;
							  semaphoreWaitInfo.pSemaphores = &res->vk.uploadSem;
							  semaphoreWaitInfo.pValues = &waitValue;
							  VK_WrapResult( vkWaitSemaphores( device->vk.device, &semaphoreWaitInfo, 5000 * 1000 ) );
							  VK_WrapResult( vkResetCommandPool( device->vk.device, res->vk.cmdSets[res->activeSet].cmdPool, 0 ) );
							  for( size_t i = 0; i < arrlen( res->vk.cmdSets[res->activeSet].temporary ); i++ ) {
								  vkDestroyBuffer( device->vk.device, res->vk.cmdSets[res->activeSet].temporary[i].buffer, NULL );
								  vmaFreeMemory( device->vk.vmaAllocator, res->vk.cmdSets[res->activeSet].temporary[i].alloc );
							  }
								arrsetlen(res->vk.cmdSets[res->activeSet].temporary, 0);
						  } ) );
	}
	__BeginNewCommandSet(device, res);

}

void RI_ResourceEndCopyTexture( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans )
{
	GPU_VULKAN_BLOCK( device->renderer, ( {
						  const struct RIFormatProps_s *formatProps = GetRIFormatProps( trans->format );
						  VkBufferImageCopy copyReq = {};
						  copyReq.bufferOffset = trans->req.byteOffset;
						  copyReq.bufferRowLength = ( trans->alignRowPitch / formatProps->stride ) * formatProps->blockWidth;
						  copyReq.bufferImageHeight = ( trans->alignSlicePitch / trans->alignRowPitch ) * formatProps->blockWidth;
						  copyReq.imageOffset.x = trans->x;
						  copyReq.imageOffset.y = trans->y;
						  copyReq.imageOffset.z = trans->z;
						  copyReq.imageExtent.width = trans->width;
						  copyReq.imageExtent.height = trans->height;
						  copyReq.imageExtent.depth = trans->depth;
						  copyReq.imageSubresource.mipLevel = trans->mipOffset;
						  copyReq.imageSubresource.layerCount = 1;
						  copyReq.imageSubresource.baseArrayLayer = trans->arrayOffset;
						  copyReq.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						  vkCmdCopyBufferToImage( res->vk.cmdSets[res->activeSet].cmdBuffer, trans->req.vk.buffer, trans->target.vk.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyReq );
					  } ) );
}
