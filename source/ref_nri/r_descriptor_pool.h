
#ifndef R_DESCRIPTOR_POOL_H
#define R_DESCRIPTOR_POOL_H

#include "../../gameshared/q_arch.h"
#include "r_frame_cmd_buffer.h"
#include "r_hasher.h"
#include "r_image.h"
#include "r_nri.h"

#define RESERVE_BLOCK_SIZE 1024
#define ALLOC_HASH_RESERVE 256
#define DESCRIPTOR_MAX_SIZE 64

struct descriptor_set_slot_s {
	uint32_t hash;
	uint32_t frameCount;
	// queue
	struct descriptor_set_slot_s *quNext;
	struct descriptor_set_slot_s *quPrev;

	// hash
	struct descriptor_set_slot_s *hNext;
	struct descriptor_set_slot_s *hPrev;

	NriDescriptorSet *descriptorSet;
};

struct descriptor_set_allloc_s {
	// configuration for the allocator
	struct {
		NriPipelineLayout *layout;
		uint32_t setIndex;
		uint32_t samplerMaxNum;
		uint32_t constantBufferMaxNum;
		uint32_t dynamicConstantBufferMaxNum;
		uint32_t textureMaxNum;
		uint32_t storageTextureMaxNum;
		uint32_t bufferMaxNum;
		uint32_t storageBufferMaxNum;
		uint32_t structuredBufferMaxNum;
		uint32_t storageStructuredBufferMaxNum;
		uint32_t accelerationStructureMaxNum;
	} config;

	struct descriptor_set_slot_s *hashSlots[ALLOC_HASH_RESERVE];
	struct descriptor_set_slot_s *queueBegin;
	struct descriptor_set_slot_s *queueEnd;

	struct descriptor_set_slot_s **reservedSlots;
	NriDescriptorPool **pools;
	struct descriptor_set_slot_s **blocks;
	size_t blockIndex;
};

struct descriptor_set_result_s {
	bool found;
	struct NriDescriptorSet *set;
};

struct descriptor_set_result_s ResolveDescriptorSet( struct nri_backend_s *backend, struct frame_cmd_buffer_s *cmd, struct descriptor_set_allloc_s *alloc, uint32_t hash );
void FreeDescriptorSetAlloc( struct nri_backend_s *backend, struct descriptor_set_allloc_s *alloc );


#endif
