#ifndef _MEM_MODULE_H
#define _MEM_MODULE_H

#include "../gameshared/q_arch.h"
#include "../gameshared/q_shared.h"

struct mempool_s;
typedef struct mempool_s mempool_t;

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
	ret name( __VA_ARGS__ );


// these functions are module specific
DECLARE_TYPEDEF_METHOD( void *, _Mod_Mem_AllocExt, mempool_t *pool, size_t size, size_t aligment, int value, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( void *, _Mod_Mem_Alloc, mempool_t *pool, size_t size, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( mempool_t *, _Mod_AllocPool, mempool_t *parent, const char *name, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( void, _Mod_Free, void *data, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( void, _Mod_Mem_FreePool, mempool_t **pool, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( void, _Mod_Mem_EmptyPool, mempool_t *pool, const char *filename, int fileline );

DECLARE_TYPEDEF_METHOD( void *, _Mem_Realloc, void *data, size_t size, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( char *, _Mem_CopyString, mempool_t *pool, const char *in, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( void, _Mem_CheckSentinels, void *data, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( size_t, Mem_PoolTotalSize, mempool_t *pool );

#undef  DECLARE_TYPEDEF_METHOD

struct mem_import_s {
	_Mod_Mem_AllocExtFn _Mod_Mem_AllocExt;
	_Mod_Mem_AllocFn _Mod_Mem_Alloc;
	_Mod_Mem_FreePoolFn _Mod_Mem_FreePool;
	_Mod_Mem_EmptyPoolFn _Mod_Mem_EmptyPool;
	_Mod_AllocPoolFn _Mod_AllocPool;
	_Mod_FreeFn _Mod_Free;

	_Mem_ReallocFn _Mem_Realloc;
	_Mem_CopyStringFn _Mem_CopyString;
	_Mem_CheckSentinelsFn _Mem_CheckSentinels;
	Mem_PoolTotalSizeFn Mem_PoolTotalSize;
};

#define Mod_Mem_AllocExt( pool, size, align, value ) _Mod_Mem_AllocExt( pool, size, align, value, __FILE__, __LINE__ )
#define Mod_Mem_Alloc( pool, size ) _Mod_Mem_Alloc( pool, size, __FILE__, __LINE__ )
#define Mod_Mem_Realloc( data, size ) _Mem_Realloc( data, size, __FILE__, __LINE__ )
#define Mod_Mem_Free( mem ) _Mod_Free( mem, __FILE__, __LINE__ )
#define Mod_Mem_AllocPool( parent, name ) _Mod_AllocPool( parent, name, __FILE__, __LINE__ )
#define Mod_Mem_FreePool( pool ) _Mod_Mem_FreePool( pool, __FILE__, __LINE__ )
#define Mod_Mem_EmptyPool( pool ) _Mem_EmptyPool( pool, __FILE__, __LINE__ )
#define Mod_Mem_CopyString( pool, str ) _Mem_CopyString( pool, str, __FILE__, __LINE__ )
#define Mod_Mem_CheckSentinels( data ) _Mem_CheckSentinels( data, __FILE__, __LINE__ )

#if MEM_DEFINE_INTERFACE_IMPL
extern struct mem_import_s mem_import;

void * _Mod_Mem_AllocExt(mempool_t *pool, size_t size, size_t aligment, int z, const char *filename, int fileline ) {return mem_import._Mod_Mem_AllocExt( pool, size, aligment, z, filename, fileline );}
void * _Mod_Mem_Alloc(mempool_t *pool, size_t size, const char *filename, int fileline ) {return mem_import._Mod_Mem_Alloc( pool, size, filename, fileline );}
mempool_t * _Mod_AllocPool(mempool_t *parent, const char *name, const char *filename, int fileline ) {return mem_import._Mod_AllocPool( parent, name, filename, fileline );}
void _Mod_Free( void *data, const char *filename, int fileline ) {mem_import._Mod_Free( data, filename, fileline );}
void _Mod_Mem_FreePool( mempool_t **pool, const char *filename, int fileline ) {mem_import._Mod_Mem_FreePool( pool, filename, fileline );}
void _Mod_Mem_EmptyPool( mempool_t *pool, const char *filename, int fileline ) {mem_import._Mod_Mem_EmptyPool( pool, filename, fileline );}

void * _Mem_Realloc( void *data, size_t size, const char *filename, int fileline ) {return mem_import._Mem_Realloc( data, size, filename, fileline );}
char * _Mem_CopyString( mempool_t *pool, const char *in, const char *filename, int fileline ) {return mem_import._Mem_CopyString( pool, in, filename, fileline );}
void _Mem_CheckSentinels( void *data, const char *filename, int fileline ) {mem_import._Mem_CheckSentinels( data, filename, fileline );}
size_t Mem_PoolTotalSize( mempool_t *pool ) {return mem_import.Mem_PoolTotalSize( pool );}

#endif


#endif
