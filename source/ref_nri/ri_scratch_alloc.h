#ifndef RI_SCRATCH_ALLOC_H
#define RI_SCRATCH_ALLOC_H
#include "../../gameshared/q_arch.h"
#include "ri_types.h"

#define RI_UNIFORM_SCRATCH_ALLLOC_SIZE (256 * 128)
#define RI_UNIFORM_SCRATCH_REQ_ALIGNMENT (256) 

struct RIBlockMem_s {
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			//VkBufferView blockView;	
		  VkBuffer buffer;
		  struct VmaAllocation_T * allocator;
		} vk;
#endif
	};
	uint8_t* pMappedAddress;
};

struct RIScratchAlloc_s;
typedef struct RIBlockMem_s ( *RIAllocBlock_Func)(struct RIDevice_s* device, struct RIScratchAlloc_s* scratch);

struct RIScratchAlloc_s {
	struct RIBlockMem_s *recycle;
	struct RIBlockMem_s *pool;

	size_t alignmentReq;
	size_t blockSize;
	RIAllocBlock_Func alloc;

	// the current buffer
	struct RIBlockMem_s current;
	size_t blockOffset;
};

struct RIScratchAllocDesc_s{
	size_t blockSize;
	size_t alignmentReq;
	RIAllocBlock_Func alloc;
};

struct RIBufferScratchAllocReq_s {
	struct RIBlockMem_s block;
	uint8_t *pMappedAddress;
	size_t bufferOffset;
	size_t bufferSize;
};

struct RIBlockMem_s RIUniformScratchAllocHandler(struct RIDevice_s* device, struct RIScratchAlloc_s* scratch);

void InitRIScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool, const struct RIScratchAllocDesc_s *desc );
void FreeRIScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s *pool ); 
void RIResetScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s  *pool );

struct RIBufferScratchAllocReq_s RIAllocBufferFromScratchAlloc( struct RIDevice_s *device, struct RIScratchAlloc_s  *pool, size_t reqSize );
void RIFinishScrachReq( struct RIDevice_s *device, struct RIBufferScratchAllocReq_s *req );

#endif

