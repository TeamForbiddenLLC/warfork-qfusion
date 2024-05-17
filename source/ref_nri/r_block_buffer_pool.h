#ifndef R_BLOCK_BUFFER_H
#define R_BLOCK_BUFFER_H
#include "../../gameshared/q_arch.h"
#include "r_nri.h"

struct block_buffer_pool_req_s {
	NriBuffer *buffer;
	size_t bufferOffset;
	void *address;
};

struct block_buffer_s {
	NriMemory *memory;
	NriBuffer *buffer;
	void *cpuMapped;
};

struct block_buffer_pool_s {
	struct block_buffer_s *recycle;
	struct block_buffer_s *pool;

	size_t alignmentReq;
	size_t structureStride;
	size_t blockSize;
	NriBufferUsageBits usageBits;

	// the current buffer
	struct block_buffer_s current;
	size_t blockOffset;
};

struct block_buffer_pool_desc_s {
	size_t blockSize;
	size_t alignmentReq;
	size_t structureStride;
	NriBufferUsageBits usageBits;
};

void InitBlockBufferPool( struct nri_backend_s *nri, struct block_buffer_pool_s *pool, const struct block_buffer_pool_desc_s *desc );
struct block_buffer_pool_req_s BlockBufferPoolReq( struct nri_backend_s *nri, struct block_buffer_pool_s *pool, size_t reqSize );
void BlockBufferPoolReset( struct block_buffer_pool_s *pool );
void BlockBufferPoolFree( struct nri_backend_s *nri, struct block_buffer_pool_s *pool );

#endif
