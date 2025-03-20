#include "ri_scratch_alloc.h"
#include "stb_ds.h"

struct RIBlockMem_s RIUniformScratchAllocHandler( struct RIDevice_s *device, struct RIScratchAlloc_s *scratch )
{
	struct RIBlockMem_s mem = { 0 };
#if ( DEVICE_IMPL_VULKAN )
	{
		VkBufferCreateInfo stageBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		stageBufferCreateInfo.pNext = NULL;
		stageBufferCreateInfo.flags = 0;
		stageBufferCreateInfo.size = scratch->blockSize;
		stageBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stageBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stageBufferCreateInfo.queueFamilyIndexCount = 0;
		stageBufferCreateInfo.pQueueFamilyIndices = NULL;
		//VK_WrapResult( vkCreateBuffer( device->vk.device, &stageBufferCreateInfo, NULL, &mem.vk.buffer ) );

		VmaAllocationInfo allocationInfo = { 0 };
		VmaAllocationCreateInfo allocInfo = { 0 };
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_WrapResult(vmaCreateBuffer(device->vk.vmaAllocator, &stageBufferCreateInfo, &allocInfo, &mem.vk.buffer, &mem.vk.allocator, &allocationInfo));

//		vmaAllocateMemoryForBuffer( device->vk.vmaAllocator, mem.vk.buffer, &allocInfo, &mem.vk.allocator, &allocationInfo );
//		vmaBindBufferMemory2( device->vk.vmaAllocator, mem.vk.allocator, 0, mem.vk.buffer, NULL );
		mem.pMappedAddress = allocationInfo.pMappedData;

		//VkBufferViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
		//createInfo.flags = (VkBufferViewCreateFlags)0;
		//createInfo.buffer = mem.vk.buffer;
		//createInfo.format = VK_FORMAT_UNDEFINED;
		//createInfo.offset = 0;
		//createInfo.range = scratch->blockSize;
		//VK_WrapResult( vkCreateBufferView( device->vk.device, &createInfo, NULL, &mem.vk.blockView ) );
	}
#endif
	return mem;
}

void InitRIScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool, const struct RIScratchAllocDesc_s *desc ) {
	memset( pool, 0, sizeof( struct RIScratchAlloc_s ) );
  pool->alignmentReq = desc->alignmentReq;
  pool->blockSize = desc->blockSize;
  pool->alloc = desc->alloc;
}

static inline bool __isPoolSlotEmpty( struct RIDevice_s *device, struct RIBlockMem_s *block )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		return block->vk.buffer == NULL;
	}
#endif
	return false;
}

static inline void __FreeRIBlockMem(struct RIDevice_s *device,struct RIBlockMem_s *block ) {
#if ( DEVICE_IMPL_VULKAN )
	if( block->vk.buffer ) {
		vkDestroyBuffer( device->vk.device, block->vk.buffer, NULL );
	}
	if( block->vk.allocator ) {
		vmaFreeMemory( device->vk.vmaAllocator, block->vk.allocator );
	}
#endif
}

void FreeRIScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool ) {
#if ( DEVICE_IMPL_VULKAN )
	if( pool->current.vk.buffer ) {
		__FreeRIBlockMem( device, &pool->current );
	}

	for( size_t i = 0; i < arrlen( pool->recycle ); i++ ) {
		__FreeRIBlockMem( device, &pool->recycle[i] );
	}

	for( size_t i = 0; i < arrlen( pool->pool ); i++ ) {
		__FreeRIBlockMem( device, &pool->pool[i] );
	}
#endif
	arrfree( pool->recycle );
	arrfree( pool->pool );
}

void RIResetScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool )
{
	for( size_t i = 0; i < arrlen( pool->recycle ); i++ ) {
		arrpush( pool->pool, pool->recycle[i] );
	}
	pool->blockOffset = 0;
	arrsetlen( pool->recycle, 0 );
}

struct RIBufferScratchAllocReq_s RIAllocBufferFromScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool, size_t reqSize )
{
	const size_t alignReqSize = Q_ALIGN_TO( reqSize, pool->alignmentReq );
	assert(pool->alloc);
	if( __isPoolSlotEmpty( device, &pool->current ) ) {
		pool->current = pool->alloc( device, pool );
		pool->blockOffset = 0;
	} else if( pool->blockOffset + alignReqSize > pool->blockSize ) {
		arrpush( pool->recycle, pool->current );
		const size_t poolSize = arrlen( pool->pool );
		if( poolSize > 0 ) {
			memcpy( &( pool->current ), &( pool->pool[poolSize - 1] ), sizeof( struct RIBlockMem_s ) );
			arrsetlen( pool->pool, poolSize - 1 );
		} else {
		  pool->current = pool->alloc( device, pool );
		}
		pool->blockOffset = 0;
	}

	struct RIBufferScratchAllocReq_s req = { 0 };
	req.block = pool->current;
	req.pMappedAddress = pool->current.pMappedAddress;
	req.bufferOffset = pool->blockOffset;
	req.bufferSize = reqSize;
	pool->blockOffset += alignReqSize;
	return req;
}

void RIFinishScrachReq( struct RIDevice_s *device, struct RIBufferScratchAllocReq_s *req )
{
#if ( DEVICE_IMPL_VULKAN )
	VK_WrapResult( vmaFlushAllocation( device->vk.vmaAllocator, req->block.vk.allocator, req->bufferOffset, req->bufferSize ) );
#endif
}
