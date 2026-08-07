/*
Copyright (C) 2025 Warfork contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// cm_q3bsp_fuzz.c -- fuzz harness for the collision model's Q3 BSP loader
//
// Companion to ref_base/test/fuzz/r_q3bsp_fuzz.c. This side matters more: it
// runs on dedicated servers, and the same .bsp is parsed a second time here
// with an entirely separate set of lump readers.
//
// A rejected map exits through Com_Error, which longjmps back to the harness.
// That is a pass. Only crashes, sanitizer reports and hangs are findings.

#include "qcommon.h"
#include "cm_local.h"

#include <setjmp.h>
#ifndef FUZZ_LIBFUZZER
#include <dirent.h>
#include <sys/stat.h>
#endif

#ifndef FUZZ_CORPUS_DIR
#define FUZZ_CORPUS_DIR "."
#endif

void CM_LoadQ3BrushModel( cmodel_state_t *cms, void *parent, void *buf, size_t bufferLen, bspFormatDesc_t *format );

static jmp_buf fuzz_abortJmp;
static bool fuzz_inLoader;
static bool fuzz_verbose;

// ============================================================================
// engine stubs
// ============================================================================

void Com_Error( com_error_code_t code, const char *format, ... )
{
	if( fuzz_verbose )
	{
		va_list ap;
		va_start( ap, format );
		fprintf( stderr, "  rejected: " );
		vfprintf( stderr, format, ap );
		fprintf( stderr, "\n" );
		va_end( ap );
	}

	if( fuzz_inLoader )
		longjmp( fuzz_abortJmp, 1 );

	abort();
}

void Sys_Error( const char *format, ... )
{
	va_list ap;
	va_start( ap, format );
	fprintf( stderr, "Sys_Error: " );
	vfprintf( stderr, format, ap );
	fprintf( stderr, "\n" );
	va_end( ap );
	abort();
}

void Com_Printf( const char *format, ... )
{
	if( fuzz_verbose )
	{
		va_list ap;
		va_start( ap, format );
		vfprintf( stderr, format, ap );
		va_end( ap );
	}
}

void Com_DPrintf( const char *format, ... ) { }

// ---------------------------------------------------------------------------
// tracked allocator
//
// The loader allocates from cms->mempool and never frees individually, so the
// harness keeps its own list and reclaims everything between iterations.
// ---------------------------------------------------------------------------

typedef struct fuzzAlloc_s
{
	struct fuzzAlloc_s *next;
	void *ptr;
} fuzzAlloc_t;

static fuzzAlloc_t *fuzz_allocs;

static void *Fuzz_Track( void *ptr )
{
	fuzzAlloc_t *node;

	if( !ptr )
		return NULL;

	node = malloc( sizeof( *node ) );
	if( !node )
		abort();
	node->ptr = ptr;
	node->next = fuzz_allocs;
	fuzz_allocs = node;
	return ptr;
}

static void Fuzz_Untrack( void *ptr )
{
	fuzzAlloc_t **link, *node;

	if( !ptr )
		return;

	for( link = &fuzz_allocs; *link; link = &( *link )->next )
	{
		if( ( *link )->ptr == ptr )
		{
			node = *link;
			*link = node->next;
			free( node );
			return;
		}
	}
}

static void Fuzz_FreeAll( void )
{
	while( fuzz_allocs )
	{
		fuzzAlloc_t *node = fuzz_allocs;
		fuzz_allocs = node->next;
		free( node->ptr );
		free( node );
	}
}

void *_Mem_Alloc( mempool_t *pool, size_t size, int musthave, int canthave, const char *filename, int fileline )
{
	void *ptr = calloc( 1, size ? size : 1 );
	if( !ptr )
		abort();
	return Fuzz_Track( ptr );
}

void *_Mem_Realloc( void *data, size_t size, const char *filename, int fileline )
{
	void *ptr;
	Fuzz_Untrack( data );
	ptr = realloc( data, size ? size : 1 );
	if( !ptr )
		abort();
	return Fuzz_Track( ptr );
}

void _Mem_Free( void *data, int musthave, int canthave, const char *filename, int fileline )
{
	Fuzz_Untrack( data );
	free( data );
}

mempool_t *_Mem_AllocPool( mempool_t *parent, const char *name, int flags, const char *filename, int fileline )
{
	return NULL;
}

void _Mem_FreePool( mempool_t **pool, int musthave, int canthave, const char *filename, int fileline ) { }
void _Mem_EmptyPool( mempool_t *pool, int musthave, int canthave, const char *filename, int fileline ) { }
void _Mem_CheckSentinelsGlobal( const char *filename, int fileline ) { }

// the loader frees the file buffer itself; the harness owns it instead
void FS_FreeFile( void *buffer ) { }

mempool_t *tempMemPool;

// ============================================================================
// harness
// ============================================================================

static const modelFormatDescr_t fuzz_formats[] =
{
	{ "*", 4, q3BSPFormats, 0, ( const modelLoader_t )CM_LoadQ3BrushModel },
	{ NULL, 0, NULL, 0, NULL }
};

int Fuzz_LoadCollisionBSP( const uint8_t *data, size_t size )
{
	static cmodel_state_t cms;
	uint8_t *buf;
	const modelFormatDescr_t *descr;
	bspFormatDesc_t *bspFormat = NULL;
	int rejected = 0;

	descr = Q_FindFormatDescriptor( fuzz_formats, data, size, (const bspFormatDesc_t **)&bspFormat );
	if( !descr || !bspFormat )
		return -1;

	buf = malloc( size + 1 );
	if( !buf )
		abort();
	memcpy( buf, data, size );
	buf[size] = 0;

	memset( &cms, 0, sizeof( cms ) );

	fuzz_inLoader = true;
	if( setjmp( fuzz_abortJmp ) == 0 )
		CM_LoadQ3BrushModel( &cms, NULL, buf, size, bspFormat );
	else
		rejected = 1;
	fuzz_inLoader = false;

	free( buf );
	Fuzz_FreeAll();

	return rejected;
}

static void Fuzz_Init( void )
{
	static bool initialized;

	if( initialized )
		return;
	initialized = true;

	fuzz_verbose = getenv( "FUZZ_VERBOSE" ) != NULL;
}

#ifdef FUZZ_LIBFUZZER

int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	Fuzz_Init();
	Fuzz_LoadCollisionBSP( data, size );
	return 0;
}

#else

// 64 MB: comfortably above the largest map in the repo, and small enough that a
// bogus size (a directory handed to fopen reports LONG_MAX) is caught here
#define FUZZ_MAX_FILE_SIZE	( 64 * 1024 * 1024 )

static int Fuzz_RunFile( const char *path )
{
	FILE *f;
	long size;
	uint8_t *data;
	int result;
	struct stat st;

	if( stat( path, &st ) != 0 || !S_ISREG( st.st_mode ) )
		return 0;   // directories and specials are not inputs

	f = fopen( path, "rb" );
	if( !f )
	{
		fprintf( stderr, "cannot open %s\n", path );
		return -1;
	}

	fseek( f, 0, SEEK_END );
	size = ftell( f );
	fseek( f, 0, SEEK_SET );
	if( size < 0 || size > FUZZ_MAX_FILE_SIZE )
	{
		fclose( f );
		fprintf( stderr, "bad size on %s\n", path );
		return -1;
	}

	data = malloc( (size_t)size + 1 );
	if( !data )
		abort();
	if( size && fread( data, 1, (size_t)size, f ) != (size_t)size )
	{
		fclose( f );
		free( data );
		fprintf( stderr, "short read on %s\n", path );
		return -1;
	}
	fclose( f );

	result = Fuzz_LoadCollisionBSP( data, (size_t)size );
	free( data );

	printf( "  %-48s %s\n", path,
		result < 0 ? "not a bsp" : ( result ? "rejected" : "loaded" ) );
	return 0;
}

int main( int argc, char **argv )
{
	int i;
	int failed = 0;

	Fuzz_Init();

	if( argc > 1 )
	{
		for( i = 1; i < argc; i++ )
			failed |= Fuzz_RunFile( argv[i] ) < 0;
	}
	else
	{
		DIR *dir = opendir( FUZZ_CORPUS_DIR );
		struct dirent *ent;
		char path[1024];

		if( !dir )
		{
			printf( "cm_q3bsp_fuzz: no corpus at %s, nothing to replay\n", FUZZ_CORPUS_DIR );
			return 0;
		}

		printf( "cm_q3bsp_fuzz: replaying corpus %s\n", FUZZ_CORPUS_DIR );
		while( ( ent = readdir( dir ) ) != NULL )
		{
			struct stat st;

			if( ent->d_name[0] == '.' )
				continue;
			snprintf( path, sizeof( path ), "%s/%s", FUZZ_CORPUS_DIR, ent->d_name );

			// one level down, so corpus/regress is replayed as well
			if( stat( path, &st ) == 0 && S_ISDIR( st.st_mode ) )
			{
				DIR *sub = opendir( path );
				struct dirent *subent;
				char subpath[1024];

				if( !sub )
					continue;
				while( ( subent = readdir( sub ) ) != NULL )
				{
					if( subent->d_name[0] == '.' )
						continue;
					snprintf( subpath, sizeof( subpath ), "%s/%s", path, subent->d_name );
					failed |= Fuzz_RunFile( subpath ) < 0;
				}
				closedir( sub );
				continue;
			}

			failed |= Fuzz_RunFile( path ) < 0;
		}
		closedir( dir );
	}

	if( failed )
	{
		printf( "cm_q3bsp_fuzz: FAILED\n" );
		return 1;
	}

	printf( "cm_q3bsp_fuzz: OK\n" );
	return 0;
}

#endif
