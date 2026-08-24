#include "ri_scratch_alloc.h"
#include "stb_ds.h"
#include "ri_renderer.h"

struct RIBlockMem_s RIUniformScratchAllocHandler( struct RIDevice_s *device, struct RIScratchAlloc_s *scratch )
{
	struct RIBlockMem_s mem = { 0 };
#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
		VkBufferCreateInfo stageBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		stageBufferCreateInfo.pNext = NULL;
		stageBufferCreateInfo.flags = 0;
		stageBufferCreateInfo.size = scratch->blockSize;
		stageBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		VK_ConfigureBufferQueueFamilies( &stageBufferCreateInfo, device->queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );

		VmaAllocationInfo allocationInfo = { 0 };
		VmaAllocationCreateInfo allocInfo = { 0 };
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
			VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | 
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_WrapResult(vmaCreateBufferWithAlignment(device->vk.vmaAllocator, &stageBufferCreateInfo, &allocInfo, scratch->alignmentReq, &mem.vk.buffer, &mem.vk.allocator, &allocationInfo));

		VkMemoryPropertyFlags memPropFlags;
		vmaGetAllocationMemoryProperties( device->vk.vmaAllocator, mem.vk.allocator, &memPropFlags );

		mem.pMappedAddress = allocationInfo.pMappedData;
	}
#endif
#if ( DEVICE_IMPL_MTL )
	{
		// A Shared, mapped uniform scratch block. On Apple Silicon this is directly GPU-visible, so the
		// UBO data the frontend writes here is usable by draws once the draw/bind path lands (Milestone C).
		struct RIBufferDesc_s desc = { .size = scratch->blockSize, .usage = RI_BUFFER_USAGE_CONSTANT_BUFFER, .memoryLocation = RI_MEMORY_HOST_UPLOAD };
		struct RIBuffer_s buf = { 0 };
		InitRIBuffer( device, &desc, &buf );
		mem.mtl.buffer = buf.mtl.buffer;
		mem.pMappedAddress = RIBufferMappedData( device, &buf );
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
#if ( DEVICE_IMPL_MTL )
	return mtlc_buffer_is_nil( block->mtl.buffer );
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
#if ( DEVICE_IMPL_MTL )
	if( !mtlc_buffer_is_nil( block->mtl.buffer ) ) {
		mtlc_buffer_release( block->mtl.buffer );
		block->mtl.buffer = mtlc_buffer_from_id( NULL );
	}
#endif
}

void FreeRIScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool ) {
	// Backend-neutral: __FreeRIBlockMem has an arm per backend. This used to sit behind the Vulkan
	// guard, which leaked every scratch block on Metal across vid_restart.
	if( !__isPoolSlotEmpty( device, &pool->current ) ) {
		__FreeRIBlockMem( device, &pool->current );
	}

	for( size_t i = 0; i < arrlen( pool->recycle ); i++ ) {
		__FreeRIBlockMem( device, &pool->recycle[i] );
	}

	for( size_t i = 0; i < arrlen( pool->pool ); i++ ) {
		__FreeRIBlockMem( device, &pool->pool[i] );
	}
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

size_t RINumberOfUsedBlock(struct RIDevice_s *device,struct RIScratchAlloc_s* pool) {
	size_t numBlock = 0;
	if( !__isPoolSlotEmpty( device, &pool->current ) ) {
		numBlock++;
	}
	numBlock += arrlen(pool->recycle);
	return numBlock;
}
struct RIBlockMem_s *RIGetUsedBlock( struct RIDevice_s *device, struct RIScratchAlloc_s *pool, size_t index )
{
	if( !__isPoolSlotEmpty( device, &pool->current ) ) {
		if( index == 0 )
			return &pool->current;
		return &pool->recycle[index - 1];
	}
	return &pool->recycle[index];
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
#if ( DEVICE_IMPL_MTL )
	// The Metal counterpart of the flush above. InitRIBuffer gives a host-upload block Shared storage on
	// unified memory (nothing to publish) but Managed storage on a discrete GPU, where a CPU write is
	// invisible to the GPU until didModifyRange: -- without this every UBO on an Intel/AMD Mac is stale.
	if( !device->physicalAdapter.mtl.hasUnifiedMemory && !mtlc_buffer_is_nil( req->block.mtl.buffer ) )
		mtlc_buffer_did_modify_range( req->block.mtl.buffer, (struct ns_range){ req->bufferOffset, req->bufferSize } );
#endif
}
