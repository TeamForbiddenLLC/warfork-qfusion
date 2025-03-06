#include "ri_resource_upload.h"
#include "ri_types.h"
#include "qtypes.h"

#include <stb_ds.h>
#include "ri_format.h"
#include "ri_renderer.h"

static void __EndActiveCommandSet( struct RIDevice_s *device, struct RIResourceUploader_s *res ) {
	GPU_VULKAN_BLOCK( device->renderer, ( {
						  vkEndCommandBuffer( res->vk.cmdSets[res->activeSet].cmdBuffer );

						  VkSemaphoreSubmitInfo signalSem = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
						  signalSem.stageMask = VK_PIPELINE_STAGE_2_NONE;
						  signalSem.value = 1 + res->syncIndex;
						  signalSem.semaphore = res->vk.uploadSem;

						  VkCommandBufferSubmitInfo cmdSubmitInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
						  cmdSubmitInfo.commandBuffer = res->vk.cmdSets[res->activeSet].cmdBuffer;

						  VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
						  submitInfo.pSignalSemaphoreInfos = &signalSem;
						  submitInfo.signalSemaphoreInfoCount = 1;
						  submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
						  submitInfo.commandBufferInfoCount = 1;

						  vkQueueSubmit2( res->copyQueue->vk.queue, 1, &submitInfo, VK_NULL_HANDLE );
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
#if ( DEVICE_IMPL_VULKAN )
	if( GPU_VULKAN_SELECTED( device->renderer ) ) {
		resource->copyQueue = &device->queues[RI_QUEUE_COPY];
		{
			VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
			semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;
			VK_WrapResult( vkCreateSemaphore( device->vk.device, &semaphoreCreateInfo, NULL, &resource->vk.uploadSem ) );
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
			VK_WrapResult( vkAllocateCommandBuffers( device->vk.device, &cmdAllocInfo, &resource->vk.cmdSets[i].cmdBuffer ) );
		}
		{
			VmaAllocationInfo allocationInfo = {};
			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
			VkBufferCreateInfo stageBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			VK_ConfigureBufferQueueFamilies( &stageBufferCreateInfo, device->queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );
			stageBufferCreateInfo.pNext = NULL;
			stageBufferCreateInfo.flags = 0;
			stageBufferCreateInfo.size = RI_RESOURCE_STAGE_SIZE;
			stageBufferCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			VK_WrapResult(vmaCreateBuffer(device->vk.vmaAllocator, &stageBufferCreateInfo, &allocInfo, &resource->vk.stageBuffer, &resource->vk.stageAlloc, &allocationInfo));
			resource->vk.pMappedData = allocationInfo.pMappedData;
		}
	}
#endif
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

#if ( DEVICE_IMPL_VULKAN )
	if( GPU_VULKAN_SELECTED( dev->renderer ) ) {
		req->vk.alloc = res->vk.stageAlloc;
		req->cpuMapping = res->vk.pMappedData;
		req->byteOffset = res->tailOffset;
		req->vk.buffer = res->vk.stageBuffer;
	}
#endif

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
#if ( DEVICE_IMPL_VULKAN )
	if( GPU_VULKAN_SELECTED( dev->renderer ) ) {
		struct RI_VK_TempBuffers tempBuffer;
		VkBufferCreateInfo stageBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		stageBufferCreateInfo.pNext = NULL;
		stageBufferCreateInfo.flags = 0;
		stageBufferCreateInfo.size = reqSize;
		stageBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stageBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stageBufferCreateInfo.queueFamilyIndexCount = 0;
		stageBufferCreateInfo.pQueueFamilyIndices = NULL;

		VmaAllocationInfo allocationInfo = {};
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT ;
	
		VK_WrapResult(vmaCreateBuffer(dev->vk.vmaAllocator, &stageBufferCreateInfo, &allocInfo, &tempBuffer.buffer, &tempBuffer.alloc, &allocationInfo));
		//VK_WrapResult( vkCreateBuffer( dev->vk.device, &stageBufferCreateInfo, NULL, &tempBuffer.buffer ) );
		//VK_WrapResult( vmaAllocateMemoryForBuffer( dev->vk.vmaAllocator, tempBuffer.buffer, &allocInfo, &tempBuffer.alloc, &allocationInfo ) );
		//vmaBindBufferMemory2( dev->vk.vmaAllocator, tempBuffer.alloc, 0, tempBuffer.buffer, NULL );

		req->vk.alloc = tempBuffer.alloc;
		req->cpuMapping = allocationInfo.pMappedData;
		req->byteOffset = 0;
		req->vk.buffer = tempBuffer.buffer;

		arrpush( res->vk.cmdSets[res->activeSet].temporary, tempBuffer );
	}
#endif
	return true;
}


void RI_ResourceBeginCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans ) {
	__ResolveStageBuffer(device, res, trans->size, &trans->req);
	trans->data = (uint8_t *)trans->req.cpuMapping + trans->req.byteOffset;
}

void RI_ResourceEndCopyBuffer( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceBufferTransaction_s *trans ) {
#if ( DEVICE_IMPL_VULKAN )
	if( GPU_VULKAN_SELECTED( device->renderer ) ) {
		bool foundInTransition = false;
		for( size_t i = 0; i < arrlen( res->postBufferBarriers ); i++ ) {
			if( res->postBufferBarriers[i].buffer == trans->target.vk.buffer ) {
				foundInTransition = true;
				break;
			}
		}
		if( !foundInTransition ) {
			VkBufferMemoryBarrier2 bufferBarriers[1] = {};
			bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			bufferBarriers[0].srcStageMask = trans->srcBarrier.vk.stage;
			bufferBarriers[0].srcAccessMask = trans->srcBarrier.vk.access; // VK_ACCESS_2_NONE;
			bufferBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			bufferBarriers[0].dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			bufferBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // TODO: VK_SHARING_MODE_EXCLUSIVE could be used instead of VK_SHARING_MODE_CONCURRENT with queue ownership transfers
			bufferBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarriers[0].buffer = trans->target.vk.buffer;
			bufferBarriers[0].offset = 0;
			bufferBarriers[0].size = VK_WHOLE_SIZE;

			VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			dependencyInfo.bufferMemoryBarrierCount = Q_ARRAY_COUNT( bufferBarriers );
			dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
			vkCmdPipelineBarrier2( res->vk.cmdSets[res->activeSet].cmdBuffer, &dependencyInfo );

			struct RIResourcePostBufferBarrier_s postTransition = {};
			postTransition.buffer = trans->target.vk.buffer;
			postTransition.postBarrier = trans->postBarrier;
			arrpush( res->postBufferBarriers, postTransition );
		}

		VkBufferCopy copyBuffer = {};
		copyBuffer.size = trans->size;
		copyBuffer.dstOffset = trans->offset;
		copyBuffer.srcOffset = trans->req.byteOffset;
		assert(res->activeSet < Q_ARRAY_COUNT(res->vk.cmdSets));
		vkCmdCopyBuffer( res->vk.cmdSets[res->activeSet].cmdBuffer, trans->req.vk.buffer, trans->target.vk.buffer, 1, &copyBuffer );
	}
#endif
}

void RI_InsertTransitionBarriers( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RICmd_s *cmd )
{
#if ( DEVICE_IMPL_VULKAN )
	if( GPU_VULKAN_SELECTED( device->renderer ) ) {
		size_t postBufferIdx = 0;
		size_t numBufferBarriers = 0;
		VkBufferMemoryBarrier2 bufferBarriers[32] = {};
		
		size_t postImageIdx = 0;
		size_t numImageBarriers = 0;
		VkImageMemoryBarrier2 imageBarriers[32] = {};

		while( true ) {
			while( postBufferIdx < arrlen( res->postBufferBarriers ) && numBufferBarriers < Q_ARRAY_COUNT( bufferBarriers ) ) {
				VkBufferMemoryBarrier2 *barrier = &bufferBarriers[numBufferBarriers++];
				struct RIResourcePostBufferBarrier_s *post = &res->postBufferBarriers[postBufferIdx++];
				memset( barrier, 0, sizeof( VkBufferMemoryBarrier2 ) );
				barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
				barrier->srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier->srcAccessMask = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				barrier->dstStageMask = post->postBarrier.vk.stage;
				barrier->dstAccessMask = post->postBarrier.vk.access;
				barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // TODO: VK_SHARING_MODE_EXCLUSIVE could be used instead of VK_SHARING_MODE_CONCURRENT with queue ownership transfers
				barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier->buffer = post->buffer;
				barrier->offset = 0;
				barrier->size = VK_WHOLE_SIZE;
			}

			while( postImageIdx < arrlen( res->postImageBarriers ) && numImageBarriers < Q_ARRAY_COUNT( imageBarriers ) ) {
				VkImageMemoryBarrier2 *barrier = &imageBarriers[numImageBarriers++];
				struct RIResourcePostImageBarrier_s *post = &res->postImageBarriers[postImageIdx++];
				memset( barrier, 0, sizeof( VkImageMemoryBarrier2 ) );
				barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier->srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier->oldLayout  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier->srcAccessMask  = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				barrier->dstStageMask = post->postBarrier.vk.stage;
				barrier->dstAccessMask = post->postBarrier.vk.access;
				barrier->newLayout = post->postBarrier.vk.layout;
				barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier->image = post->image;
				barrier->subresourceRange = (VkImageSubresourceRange){
					VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS,
				};
			}
			if(numBufferBarriers == 0 && numImageBarriers == 0) {
				break;
			}
			VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			dependencyInfo.bufferMemoryBarrierCount = numBufferBarriers;
			dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
			dependencyInfo.imageMemoryBarrierCount = numImageBarriers;
			dependencyInfo.pImageMemoryBarriers = imageBarriers;
			vkCmdPipelineBarrier2( cmd->vk.cmd, &dependencyInfo );
		}
	}
	arrsetlen(res->postImageBarriers, 0);
	arrsetlen(res->postBufferBarriers, 0);
#endif
}

void RI_ResourceSubmit( struct RIDevice_s *device, struct RIResourceUploader_s *res )
{
	__EndActiveCommandSet( device, res );
	res->activeSet = ( res->activeSet + 1 ) % RI_RESOURCE_NUM_COMMAND_SETS;
	if( res->syncIndex >= RI_RESOURCE_NUM_COMMAND_SETS ) {
#if ( DEVICE_IMPL_VULKAN )
		if( GPU_VULKAN_SELECTED( device->renderer ) ) {
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
			arrsetlen( res->vk.cmdSets[res->activeSet].temporary, 0 );
		}
#endif
	}
	__BeginNewCommandSet( device, res );
}

void RI_ResourceBeginCopyTexture( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans )
{
	const uint64_t alignedRowPitch = Q_ALIGN_TO( trans->rowPitch, device->physicalAdapter.uploadBufferTextureRowAlignment );
	const uint64_t alignedSlicePitch = Q_ALIGN_TO( trans->sliceNum * alignedRowPitch, device->physicalAdapter.uploadBufferTextureSliceAlignment );
	trans->alignRowPitch = alignedRowPitch;
	trans->alignSlicePitch = alignedSlicePitch;
	__ResolveStageBuffer(device, res, alignedSlicePitch, &trans->req);
	trans->data = (uint8_t *)trans->req.cpuMapping + trans->req.byteOffset;
}

void RI_ResourceEndCopyTexture( struct RIDevice_s *device, struct RIResourceUploader_s *res, struct RIResourceTextureTransaction_s *trans )
{

#if ( DEVICE_IMPL_VULKAN )
	if( GPU_VULKAN_SELECTED( device->renderer ) ) {
	
		bool foundInTransition = false;
		for(size_t i = 0; i < arrlen(res->postImageBarriers); i++) {
			if(res->postImageBarriers[i].image == trans->target.vk.image) {
				foundInTransition = true;
				break;
			}
		}

		if( !foundInTransition ) {
			VkImageMemoryBarrier2 imageBarriers[1] = {};
			imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			imageBarriers[0].srcAccessMask = trans->srcBarrier.vk.access; // VK_ACCESS_2_NONE;
			imageBarriers[0].srcStageMask = trans->srcBarrier.vk.stage;
			imageBarriers[0].oldLayout = trans->srcBarrier.vk.layout;	  // VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			imageBarriers[0].dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarriers[0].image = trans->target.vk.image;
			imageBarriers[0].subresourceRange = (VkImageSubresourceRange){
				VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS,
			};
			VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			dependencyInfo.imageMemoryBarrierCount = Q_ARRAY_COUNT( imageBarriers );
			dependencyInfo.pImageMemoryBarriers = imageBarriers;
			vkCmdPipelineBarrier2( res->vk.cmdSets[res->activeSet].cmdBuffer, &dependencyInfo );

			struct RIResourcePostImageBarrier_s postTransition = {};
			postTransition.image = trans->target.vk.image;
			postTransition.postBarrier = trans->postBarrier;
			arrpush(res->postImageBarriers, postTransition);
		}
		{
			const struct RIFormatProps_s *formatProps = GetRIFormatProps( trans->format );

			const uint32_t rowBlockNum = trans->rowPitch / formatProps->stride;
			const uint32_t bufferRowLength = rowBlockNum * formatProps->blockWidth;

			const uint32_t sliceRowNum = trans->alignSlicePitch / trans->rowPitch;
			const uint32_t bufferImageHeight = sliceRowNum * formatProps->blockWidth;

			VkBufferImageCopy copyReq = {};
			copyReq.bufferOffset = trans->req.byteOffset;
			copyReq.bufferRowLength = bufferRowLength;
			copyReq.bufferImageHeight = bufferImageHeight;
			copyReq.imageOffset.x = trans->x;
			copyReq.imageOffset.y = trans->y;
			copyReq.imageOffset.z = trans->z;
			copyReq.imageExtent.width = trans->width;
			copyReq.imageExtent.height = trans->height;
			copyReq.imageExtent.depth = Q_MAX(trans->depth, 1);
			copyReq.imageSubresource.mipLevel = trans->mipOffset;
			copyReq.imageSubresource.layerCount = 1;
			copyReq.imageSubresource.baseArrayLayer = trans->arrayOffset;
			copyReq.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			assert( res->activeSet < Q_ARRAY_COUNT( res->vk.cmdSets ) );
			vkCmdCopyBufferToImage( res->vk.cmdSets[res->activeSet].cmdBuffer, trans->req.vk.buffer, trans->target.vk.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyReq );
		}
	}
#endif
}
