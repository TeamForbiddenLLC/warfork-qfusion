#include "ri_scratch_alloc.h"
#include "stb_ds.h"
#include "ri_renderer.h"
#include "../qcommon/qcommon.h"

static inline void __FreeRIBlockMem( struct RIDevice_s *device, struct RIBlockMem_s *block );

struct RIBlockMem_s RIUniformScratchAllocHandler( struct RIDevice_s *device, struct RIScratchAlloc_s *scratch, size_t size )
{
	struct RIBlockMem_s mem = { 0 };
#if ( DEVICE_IMPL_VULKAN )
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
		{
			uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };
			VkBufferCreateInfo stageBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			stageBufferCreateInfo.pNext = NULL;
			stageBufferCreateInfo.flags = 0;
			stageBufferCreateInfo.size = size;
			stageBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			VK_ConfigureBufferQueueFamilies( &stageBufferCreateInfo, device->queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );

			VmaAllocationInfo allocationInfo = { 0 };
			VmaAllocationCreateInfo allocInfo = { 0 };
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			// No ALLOW_TRANSFER_INSTEAD: the frontend memcpy's straight into the block, so it must always be
			// host visible. With that flag VMA may pick device-local memory and leave pMappedData NULL.
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

			const VkResult result =
				vmaCreateBufferWithAlignment( device->vk.vmaAllocator, &stageBufferCreateInfo, &allocInfo, scratch->alignmentReq, &mem.vk.buffer, &mem.vk.allocator, &allocationInfo );
			if( !VK_WrapResult( result ) || !allocationInfo.pMappedData ) {
				__FreeRIBlockMem( device, &mem );
				memset( &mem, 0, sizeof( mem ) );
				return mem;
			}
			mem.pMappedAddress = allocationInfo.pMappedData;
		}
	}
#endif
#if ( DEVICE_IMPL_MTL )
	if( RIIsTargetSelected( RI_DEVICE_API_MTL ) ) {
		{
			// A Shared, mapped uniform scratch block. On Apple Silicon this is directly GPU-visible, so the
			// UBO data the frontend writes here is usable by draws once the draw/bind path lands (Milestone C).
			struct RIBufferDesc_s desc = { .size = size, .usage = RI_BUFFER_USAGE_CONSTANT_BUFFER, .memoryLocation = RI_MEMORY_HOST_UPLOAD };
			struct RIBuffer_s buf = { 0 };
			InitRIBuffer( device, &desc, &buf );
			mem.mtl.buffer = buf.mtl.buffer;
			mem.pMappedAddress = RIBufferMappedData( device, &buf );
		}
	}
#endif
	return mem;
}

void InitRIScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool, const struct RIScratchAllocDesc_s *desc ) {
	assert( desc->alloc );
	assert( desc->blockSize > 0 );
	assert( desc->alignmentReq > 0 && ( desc->alignmentReq & ( desc->alignmentReq - 1 ) ) == 0 ); // Q_ALIGN_TO needs a power of two
	memset( pool, 0, sizeof( struct RIScratchAlloc_s ) );
  pool->alignmentReq = desc->alignmentReq;
  pool->blockSize = desc->blockSize;
  pool->alloc = desc->alloc;
}

static inline bool __isPoolSlotEmpty( struct RIDevice_s *device, struct RIBlockMem_s *block )
{
#if ( DEVICE_IMPL_VULKAN )
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
		{
			return block->vk.buffer == NULL;
		}
	}
#endif
#if ( DEVICE_IMPL_MTL )
	if( RIIsTargetSelected( RI_DEVICE_API_MTL ) ) {
		return mtlc_buffer_is_nil( block->mtl.buffer );
	}
#endif
	return false;
}

static inline void __FreeRIBlockMem(struct RIDevice_s *device,struct RIBlockMem_s *block ) {
#if ( DEVICE_IMPL_VULKAN )
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
		if( block->vk.buffer ) {
			vkDestroyBuffer( device->vk.device, block->vk.buffer, NULL );
			block->vk.buffer = VK_NULL_HANDLE;
		}
		if( block->vk.allocator ) {
			vmaFreeMemory( device->vk.vmaAllocator, block->vk.allocator );
			block->vk.allocator = NULL;
		}
	}
#endif
#if ( DEVICE_IMPL_MTL )
	if( RIIsTargetSelected( RI_DEVICE_API_MTL ) ) {
		if( !mtlc_buffer_is_nil( block->mtl.buffer ) ) {
			mtlc_buffer_release( block->mtl.buffer );
			block->mtl.buffer = mtlc_buffer_from_id( NULL );
		}
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
	for( size_t i = 0; i < arrlen( pool->oversized ); i++ ) {
		__FreeRIBlockMem( device, &pool->oversized[i] );
	}
	arrfree( pool->recycle );
	arrfree( pool->pool );
	arrfree( pool->oversized );
}

void RIResetScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool )
{
	for( size_t i = 0; i < arrlen( pool->recycle ); i++ ) {
		arrpush( pool->pool, pool->recycle[i] );
	}
	// Called once the frame that used this allocator has retired, so its one-off blocks can go.
	for( size_t i = 0; i < arrlen( pool->oversized ); i++ ) {
		__FreeRIBlockMem( device, &pool->oversized[i] );
	}
	arrsetlen( pool->oversized, 0 );
	pool->blockOffset = 0;
	arrsetlen( pool->recycle, 0 );
}

size_t RINumberOfUsedBlock(struct RIDevice_s *device,struct RIScratchAlloc_s* pool) {
	size_t numBlock = 0;
	if( !__isPoolSlotEmpty( device, &pool->current ) ) {
		numBlock++;
	}
	numBlock += arrlen(pool->recycle);
	numBlock += arrlen(pool->oversized);
	return numBlock;
}
struct RIBlockMem_s *RIGetUsedBlock( struct RIDevice_s *device, struct RIScratchAlloc_s *pool, size_t index )
{
	if( !__isPoolSlotEmpty( device, &pool->current ) ) {
		if( index == 0 )
			return &pool->current;
		index--;
	}
	if( index < (size_t)arrlen( pool->recycle ) )
		return &pool->recycle[index];
	return &pool->oversized[index - arrlen( pool->recycle )];
}

struct RIBufferScratchAllocReq_s RIAllocBufferFromScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool, size_t reqSize )
{
	const size_t alignReqSize = Q_ALIGN_TO( reqSize, pool->alignmentReq );
	assert(pool->alloc);
	struct RIBufferScratchAllocReq_s req = { 0 };

	// A request that can never fit in a regular block gets a dedicated one: handing it a normal block
	// would have the caller write past its end into whatever VMA placed next.
	if( alignReqSize > pool->blockSize ) {
		struct RIBlockMem_s block = pool->alloc( device, pool, alignReqSize );
		if( !block.pMappedAddress ) {
			Com_Printf( S_COLOR_RED "Scratch alloc: failed to allocate a %zu byte oversized block\n", alignReqSize );
			return req;
		}
		arrpush( pool->oversized, block );
		req.block = block;
		req.pMappedAddress = block.pMappedAddress;
		req.bufferOffset = 0;
		req.bufferSize = reqSize;
		return req;
	}

	if( __isPoolSlotEmpty( device, &pool->current ) || pool->blockOffset + alignReqSize > pool->blockSize ) {
		// Get the replacement before retiring the current block so a failed allocation leaves the
		// allocator as it was instead of with an empty current block.
		struct RIBlockMem_s next = { 0 };
		const size_t poolSize = arrlen( pool->pool );
		if( poolSize > 0 ) {
			next = pool->pool[poolSize - 1];
			arrsetlen( pool->pool, poolSize - 1 );
		} else {
			next = pool->alloc( device, pool, pool->blockSize );
			if( !next.pMappedAddress ) {
				Com_Printf( S_COLOR_RED "Scratch alloc: failed to allocate a %zu byte block\n", pool->blockSize );
				return req;
			}
		}
		if( !__isPoolSlotEmpty( device, &pool->current ) )
			arrpush( pool->recycle, pool->current );
		pool->current = next;
		pool->blockOffset = 0;
	}

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
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
		if( req->block.vk.allocator )
			VK_WrapResult( vmaFlushAllocation( device->vk.vmaAllocator, req->block.vk.allocator, req->bufferOffset, req->bufferSize ) );
	}
#endif
#if ( DEVICE_IMPL_MTL )
	if( RIIsTargetSelected( RI_DEVICE_API_MTL ) ) {
		// The Metal counterpart of the flush above. InitRIBuffer gives a host-upload block Shared storage on
		// unified memory (nothing to publish) but Managed storage on a discrete GPU, where a CPU write is
		// invisible to the GPU until didModifyRange: -- without this every UBO on an Intel/AMD Mac is stale.
		if( !device->physicalAdapter.mtl.hasUnifiedMemory && !mtlc_buffer_is_nil( req->block.mtl.buffer ) )
			mtlc_buffer_did_modify_range( req->block.mtl.buffer, (struct ns_range){ req->bufferOffset, req->bufferSize } );
	}
#endif
}
