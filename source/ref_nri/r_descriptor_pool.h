
#ifndef R_DESCRIPTOR_POOL_H
#define R_DESCRIPTOR_POOL_H

#include "../../gameshared/q_arch.h"
#include "r_frame_cmd_buffer.h"
#include "r_image.h"

#include "qhash.h"
#include "ri_types.h"

#define RESERVE_BLOCK_SIZE 1024
#define ALLOC_HASH_RESERVE 256
#define DESCRIPTOR_MAX_SIZE 64
#define DESCRIPTOR_RESERVED_SIZE 64

struct descriptor_set_slot_s {
	uint32_t hash;
	uint32_t frameCount;
	// queue
	struct descriptor_set_slot_s *quNext;
	struct descriptor_set_slot_s *quPrev;

	// hash
	struct descriptor_set_slot_s *hNext;
	struct descriptor_set_slot_s *hPrev;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkDescriptorPool pool;
			VkDescriptorSet handle;
		} vk;
#endif
	};
};

struct descriptor_pool_alloc_slot_s {
		union {
#if ( DEVICE_IMPL_VULKAN )
			struct {
				VkDescriptorPool handle;
			} vk;
#endif
		};

};

struct descriptor_set_allloc_s;
typedef void ( *RIAllocDescriptor_Func )( struct RIDevice_s *device, struct descriptor_set_allloc_s *alloc );

struct descriptor_set_allloc_s {
	RIAllocDescriptor_Func descriptorAllocator;
	uint8_t framesInFlight; // the number of frames in flight 

	struct descriptor_set_slot_s *hashSlots[ALLOC_HASH_RESERVE];
	struct descriptor_set_slot_s *queueBegin;
	struct descriptor_set_slot_s *queueEnd;

	struct descriptor_set_slot_s **reservedSlots; // stb arrays
	struct descriptor_pool_alloc_slot_s* pools; // stb arrays
	struct descriptor_set_slot_s **blocks;
	size_t blockIndex;
};

struct descriptor_set_result_s {
	bool found;
	struct descriptor_set_slot_s *set; // the associated slot
};

struct descriptor_set_result_s ResolveDescriptorSet( struct RIDevice_s *device,
													 struct descriptor_set_allloc_s *alloc,
													 uint32_t frameCount,
													 uint32_t hash );
void FreeDescriptorSetAlloc( struct RIDevice_s *device, struct descriptor_set_allloc_s *alloc );

// utility
struct descriptor_set_slot_s *AllocDescriptorsetSlot( struct descriptor_set_allloc_s *alloc );
void AttachDescriptorSlot( struct descriptor_set_allloc_s *alloc, struct descriptor_set_slot_s *slot );
void DetachDescriptorSlot( struct descriptor_set_allloc_s *alloc, struct descriptor_set_slot_s *slot );

#endif
