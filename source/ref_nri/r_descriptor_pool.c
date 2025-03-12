#include "r_descriptor_pool.h"
#include "r_nri.h"
#include "stb_ds.h"

struct descriptor_set_slot_s *AllocDescriptorsetSlot( struct descriptor_set_allloc_s *alloc )
{
	if( alloc->blocks == NULL || alloc->blockIndex == RESERVE_BLOCK_SIZE ) {
		struct descriptor_set_slot_s *block = calloc( RESERVE_BLOCK_SIZE, sizeof( struct descriptor_set_slot_s ) );
		alloc->blockIndex = 0;
		arrpush( alloc->blocks, block );
		return block + ( alloc->blockIndex++ );
	}
	return alloc->blocks[arrlen( alloc->blocks ) - 1] + ( alloc->blockIndex++ );
}

void AttachDescriptorSlot( struct descriptor_set_allloc_s *alloc, struct descriptor_set_slot_s *slot )
{
	assert( slot );
	{
		slot->quNext = NULL;
		slot->quPrev = alloc->queueEnd;
		if( alloc->queueEnd ) {
			alloc->queueEnd->quNext = slot;
		}
		alloc->queueEnd = slot;
		if( !alloc->queueBegin ) {
			alloc->queueBegin = slot;
		}
	}
	{
		const size_t hashIndex = slot->hash % ALLOC_HASH_RESERVE;
		slot->hPrev = NULL;
		slot->hNext = NULL;
		if( alloc->hashSlots[hashIndex] ) {
			alloc->hashSlots[hashIndex]->hPrev = slot;
			slot->hNext = alloc->hashSlots[hashIndex];
		}
		alloc->hashSlots[hashIndex] = slot;
	}
}

void DetachDescriptorSlot( struct descriptor_set_allloc_s *alloc, struct descriptor_set_slot_s *slot )
{
	assert( slot );
	// remove from queue
	{
		if( alloc->queueBegin == slot ) {
			alloc->queueBegin = slot->quNext;
			if( slot->quNext ) {
				slot->quNext->quPrev = NULL;
			}
		} else if( alloc->queueEnd == slot ) {
			alloc->queueEnd = slot->quPrev;
			if( slot->quPrev ) {
				slot->quPrev->quNext = NULL;
			}
		} else {
			if( slot->quPrev ) {
				slot->quPrev->quNext = slot->quNext;
			}
			if( slot->quNext ) {
				slot->quNext->quPrev = slot->quPrev;
			}
		}
	}
	// free from hashTable
	{
		const size_t hashIndex = slot->hash % ALLOC_HASH_RESERVE;
		if( alloc->hashSlots[hashIndex] == slot ) {
			alloc->hashSlots[hashIndex] = slot->hNext;
			if( slot->hNext ) {
				slot->hPrev = NULL;
			}
		} else {
			if( slot->hPrev ) {
				slot->hPrev->hNext = slot->hNext;
			}
			if( slot->hNext ) {
				slot->hNext->hPrev = slot->hPrev;
			}
		}
	}
}

struct descriptor_set_result_s ResolveDescriptorSet( struct RIDevice_s *device, struct descriptor_set_allloc_s *alloc, uint32_t frameCount, uint32_t hash )
{
	struct descriptor_set_result_s result = { 0 };
	const size_t hashIndex = hash % ALLOC_HASH_RESERVE;
	for( struct descriptor_set_slot_s *c = alloc->hashSlots[hashIndex]; c; c = c->hNext ) {
		if( c->hash == hash ) {
			if( alloc->queueEnd == c ) {
				// already at the end of the queue
			} else if (alloc->queueBegin == c) {
				alloc->queueBegin = c->quNext;
				if( c->quNext ) {
					c->quNext->quPrev = NULL;
				}
			} else {
				if( c->quPrev ) {
					c->quPrev->quNext = c->quNext;
				}
				if( c->quNext ) {
					c->quNext->quPrev = c->quPrev;
				}
			}
			c->quNext = NULL;
			c->quPrev = alloc->queueEnd;
			if( alloc->queueEnd ) {
				alloc->queueEnd->quNext = c;
			}
			alloc->queueEnd = c;

			c->frameCount = frameCount;
			result.set = c;
			result.found = true;
			assert(result.set);
			return result;
		}
	}

	if( alloc->queueBegin && frameCount > alloc->queueBegin->frameCount + alloc->framesInFlight) {
		struct descriptor_set_slot_s *slot = alloc->queueBegin;
		DetachDescriptorSlot( alloc, slot );
		slot->frameCount = frameCount;
		slot->hash = hash;
		AttachDescriptorSlot( alloc, slot );
		result.set = slot;
		result.found = false;
		assert(result.set);
		return result;
	}

	if( arrlen( alloc->reservedSlots ) == 0 ) {
		alloc->descriptorAllocator(device, alloc);
		assert(arrlen(alloc->reservedSlots) > 0); // we didn't reserve any slots ...
	}
	struct descriptor_set_slot_s *slot = arrpop( alloc->reservedSlots );
	slot->hash = hash;
	slot->frameCount = frameCount;
	
	//device->coreI.SetDescriptorSetDebugName(slot->descriptorSet, alloc->debugName);

	AttachDescriptorSlot( alloc, slot );
	result.set = slot;
	result.found = false;
	assert(result.set);
	return result;
}

void FreeDescriptorSetAlloc( struct RIDevice_s *device, struct descriptor_set_allloc_s *alloc )
{
#if ( DEVICE_IMPL_VULKAN )
	for( size_t i = 0; i < arrlen( alloc->blocks ); i++ ) {
		// TODO: do i need to free indivudal descriptor sets or can i just free the entire pool
		// for(size_t blockIdx = 0; blockIdx < RESERVE_BLOCK_SIZE; blockIdx++) {
		//	vkFreeDescriptorSets(device->vk.device, alloc->blocks[i]->vk.pool, )
		//}
		free( alloc->blocks[i] );
	}
	arrfree( alloc->blocks );
	for( size_t i = 0; i < arrlen( alloc->pools ); i++ ) {
		vkDestroyDescriptorPool( device->vk.device, alloc->pools[i].vk.handle, NULL );
	}
	arrfree( alloc->pools );
#endif
}
