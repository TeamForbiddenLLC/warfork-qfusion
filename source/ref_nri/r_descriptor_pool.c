#include "r_descriptor_pool.h"
#include "r_nri.h"
#include "stb_ds.h"

static struct descriptor_set_slot_s* ReserveDescriptorSetSlot(struct descriptor_set_allloc_s *alloc) {
  if(alloc->blocks || alloc->blockIndex == RESERVE_BLOCK_SIZE) {
	  struct descriptor_set_slot_s *block = calloc( RESERVE_BLOCK_SIZE, sizeof( struct descriptor_set_slot_s ) );
	  alloc->blockIndex = 0;
	  arrpush( alloc->blocks, block );
	  return block + ( alloc->blockIndex++ );
  }
  return alloc->blocks[arrlen(alloc->blocks) - 1] + (alloc->blockIndex++);
}


static void AttachDescriptorSlot(struct descriptor_set_allloc_s *alloc,struct descriptor_set_slot_s* slot) {
	
	assert(slot);
	{
	  slot->quNext = NULL;
	  slot->quPrev = alloc->queueEnd;
	  if(alloc->queueEnd) {
	    alloc->queueEnd->quNext = slot;
	  }
	  alloc->queueEnd = slot;
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

static void DetachDescriptorSlot(struct descriptor_set_allloc_s *alloc, struct descriptor_set_slot_s* slot) {
  assert(slot);
  // remove from queue
  {
	  
	  if( alloc->queueBegin == slot ) {
		  alloc->queueBegin = slot->quNext;
	    if(slot->quNext) {
	      slot->quNext->quPrev = NULL;
	    }
	  } else if(alloc->queueEnd == slot) {
		  alloc->queueEnd = slot->quPrev;
		  if( slot->quPrev) {
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
		  if(slot->hNext) {
		    slot->hPrev = NULL;
		  }
	  } else {
		  if( slot->hPrev) {
			  slot->hPrev->hNext = slot->hNext;
		  }
		  if( slot->hNext) {
			  slot->hNext->hPrev = slot->hPrev;
		  }
	  }
  }
}

struct descriptor_set_result_s AllocResolveDescriptorSet( struct nri_backend_s *backend, struct frame_cmd_buffer_s* cmd, struct descriptor_set_allloc_s *alloc, uint32_t hash ) {
  NriDescriptorPool* descriptorPool;
  struct descriptor_set_result_s result = { 0 };
  const size_t hashIndex = hash % ALLOC_HASH_RESERVE;
  for(struct descriptor_set_slot_s* c = alloc->hashSlots[hashIndex]; c; c = c->hNext) {
    if(c->hash == hash) {
      c->frameCount = cmd->frameCount;
      result.set = c->descriptorSet;
      result.found = true;
      return result;
    }
  }

  if( alloc->queueBegin && alloc->queueBegin->frameCount > NUMBER_FRAMES_FLIGHT && alloc->queueBegin->frameCount + NUMBER_FRAMES_FLIGHT < cmd->frameCount ) {
	  struct descriptor_set_slot_s *slot = alloc->queueBegin;
	  DetachDescriptorSlot( alloc, slot );
	  slot->frameCount = cmd->frameCount;
	  slot->hash = hash;
	  AttachDescriptorSlot( alloc, slot );
	  result.set = slot->descriptorSet;
	  result.found = true;
	  return result;
  }

  if(arrlen(alloc->reservedSlots) == 0) {
    const NriDescriptorPoolDesc poolDesc = { 
                         .descriptorSetMaxNum = DESCRIPTOR_MAX_SIZE,
										     .samplerMaxNum = alloc->config.samplerMaxNum,
										     .constantBufferMaxNum = alloc->config.constantBufferMaxNum,
										     .dynamicConstantBufferMaxNum = alloc->config.dynamicConstantBufferMaxNum,
										     .textureMaxNum = alloc->config.textureMaxNum,
										     .storageTextureMaxNum = alloc->config.storageTextureMaxNum,
										     .bufferMaxNum = alloc->config.bufferMaxNum,
										     .storageBufferMaxNum = alloc->config.storageBufferMaxNum,
										     .structuredBufferMaxNum = alloc->config.structuredBufferMaxNum,
										     .storageStructuredBufferMaxNum = alloc->config.storageStructuredBufferMaxNum,
										     .accelerationStructureMaxNum = alloc->config.accelerationStructureMaxNum };
    NRI_ABORT_ON_FAILURE( backend->coreI.CreateDescriptorPool( backend->device, &poolDesc, &descriptorPool ) );
    NriDescriptorSet *sets[DESCRIPTOR_SET_MAX];
    backend->coreI.AllocateDescriptorSets( descriptorPool, alloc->layout, alloc->setIndex, sets, DESCRIPTOR_SET_MAX, 0 );
    for(size_t i = 0; i < DESCRIPTOR_SET_MAX; i++) {
      struct descriptor_set_slot_s* slot = ReserveDescriptorSetSlot(alloc);
      slot->descriptorSet = sets[i]; 
      arrpush(alloc->reservedSlots, slot);
    }
  }
  struct descriptor_set_slot_s *slot = arrpop( alloc->reservedSlots );
  slot->hash = hash;
  slot->frameCount = cmd->frameCount;

  AttachDescriptorSlot( alloc, slot );
  result.set = slot->descriptorSet;
  result.found = false;
  return result;
}

void FreeDescriptorSetAlloc(struct nri_backend_s* backend, struct descriptor_set_allloc_s* alloc) {
  for(size_t i = 0; i < arrlen(alloc->blocks); i++) {
    free(alloc->blocks[i]);
  }
  arrfree(alloc->blocks);
  for(size_t i = 0; i < arrlen(alloc->pools); i++) {
    backend->coreI.DestroyDescriptorPool(alloc->pools[i]);
  }
  arrfree(alloc->pools);
}
