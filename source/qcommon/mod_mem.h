#ifndef R_MEM_MODULE_H
#define R_MEM_MODULE_H

#include "../gameshared/q_arch.h"

struct mempool_s;
typedef struct mempool_s mempool_t;

struct mempool_stats_s {
	size_t size;
	size_t realSize;
};

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
	ret name( __VA_ARGS__ );

DECLARE_TYPEDEF_METHOD( void *, __Q_Malloc, size_t size, const char *sourceFilename, const char *functionName, int sourceLine );
DECLARE_TYPEDEF_METHOD( void *, __Q_Realloc, void *ptr, size_t size, const char *sourceFilename, const char *functionName, int sourceLine );
DECLARE_TYPEDEF_METHOD( void *, __Q_Calloc, size_t count, size_t size, const char *sourceFilename, const char *functionName, int sourceLine );
DECLARE_TYPEDEF_METHOD( void *, __Q_MallocAligned, size_t alignment, size_t size, const char *sourceFilename, const char *functionName, int sourceLine );
DECLARE_TYPEDEF_METHOD( void *, __Q_CallocAligned, size_t count, size_t alignment,  size_t size, const char *sourceFilename, const char *functionName, int sourceLine );
DECLARE_TYPEDEF_METHOD( void, Q_Free, void *ptr );

#define Q_Malloc( size ) __Q_Malloc( size, __FILE__, __FUNCTION__, __LINE__ )
#define Q_Calloc(count, size) __Q_Calloc(count, size, __FILE__, __FUNCTION__, __LINE__ )
#define Q_Realloc( ptr, size ) __Q_Realloc( ptr, size, __FILE__, __FUNCTION__, __LINE__ )
#define Q_MallocAligned( alignment, size ) __Q_MallocAligned( alignment, size, __FILE__, __FUNCTION__, __LINE__ )
#define Q_CallocAligned( count, alignment, size ) __Q_CallocAligned( count, alignment, size, __FILE__, __FUNCTION__, __LINE__ )

DECLARE_TYPEDEF_METHOD( mempool_t *, Q_CreatePool, mempool_t *parent, const char *name );
DECLARE_TYPEDEF_METHOD( void, Q_LinkToPool, void *ptr, mempool_t *pool );
DECLARE_TYPEDEF_METHOD( void, Q_FreePool, mempool_t *pool );
DECLARE_TYPEDEF_METHOD( void, Q_EmptyPool, mempool_t *pool );
DECLARE_TYPEDEF_METHOD( struct mempool_stats_s, Q_PoolStats, mempool_t *pool );
DECLARE_TYPEDEF_METHOD( void, Mem_ValidationAllAllocations );
#undef DECLARE_TYPEDEF_METHOD

mempool_t* Q_ParentPool(); 

struct mem_import_s {
  mempool_t* parent; // the parent pool to link to by default if NULL is passed into a pool
	Q_PoolStatsFn Q_PoolStats;
	__Q_MallocFn __Q_Malloc;
	__Q_CallocFn __Q_Calloc;
	__Q_ReallocFn __Q_Realloc;
	__Q_MallocAlignedFn __Q_MallocAligned;
	__Q_CallocAlignedFn __Q_CallocAligned;
	Mem_ValidationAllAllocationsFn Mem_ValidationAllAllocations; 
	Q_FreeFn Q_Free;
	Q_CreatePoolFn Q_CreatePool;
	Q_LinkToPoolFn Q_LinkToPool;
	Q_FreePoolFn Q_FreePool;
	Q_EmptyPoolFn Q_EmptyPool;
};

#define DECLARE_MEM_STRUCT(PARENT) { \
	PARENT, \
	Q_PoolStats, \
	__Q_Malloc, \
	__Q_Calloc, \
	__Q_Realloc, \
	__Q_MallocAligned, \
	__Q_CallocAligned, \
	Mem_ValidationAllAllocations, \
	Q_Free, \
	Q_CreatePool, \
	Q_LinkToPool, \
	Q_FreePool, \
	Q_EmptyPool \
};

#if MEM_DEFINE_INTERFACE_IMPL
static struct mem_import_s mem_import;
mempool_t* Q_ParentPool() { return mem_import.parent; }
void *__Q_Malloc( size_t size, const char *sourceFilename, const char *functionName, int sourceLine ) { return mem_import.__Q_Malloc(size, sourceFilename, functionName, sourceLine); }
void* __Q_Calloc(size_t count, size_t size, const char *sourceFilename, const char *functionName, int sourceLine ) { return mem_import.__Q_Calloc(count, size, sourceFilename, functionName, sourceLine); }
void *__Q_Realloc( void *ptr, size_t size, const char *sourceFilename, const char *functionName, int sourceLine ) { return mem_import.__Q_Realloc(ptr, size, sourceFilename, functionName, sourceLine); }
void *__Q_MallocAligned( size_t alignment, size_t size, const char *sourceFilename, const char *functionName, int sourceLine ) { return mem_import.__Q_MallocAligned(alignment, size, sourceFilename, functionName, sourceLine); }
void * __Q_CallocAligned(size_t count, size_t alignment,  size_t size, const char *sourceFilename, const char *functionName, int sourceLine) { return mem_import.__Q_CallocAligned(count, alignment, size, sourceFilename, functionName, sourceLine); }
struct mempool_stats_s Q_PoolStats(mempool_t* pool) { return mem_import.Q_PoolStats(pool); }
void Mem_ValidationAllAllocations( ) { return mem_import.Mem_ValidationAllAllocations(); }
void Q_Free( void *ptr ) { return mem_import.Q_Free(ptr); }
mempool_t *Q_CreatePool( mempool_t *parent, const char *name ) { return mem_import.Q_CreatePool(parent ? parent : mem_import.parent , name);}
void Q_LinkToPool( void *ptr, mempool_t *pool ) { return mem_import.Q_LinkToPool(ptr, pool);}
void Q_FreePool( mempool_t *pool ) {mem_import.Q_FreePool(pool);}
void Q_EmptyPool( mempool_t *pool ) {mem_import.Q_EmptyPool(pool);}
static inline void Q_ImportMemModule(const struct mem_import_s* mem) {
	mem_import = *mem;
}
#endif

#endif
