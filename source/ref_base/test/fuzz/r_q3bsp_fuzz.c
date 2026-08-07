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

// r_q3bsp_fuzz.c -- fuzz harness for the renderer's Q3 BSP loader
//
// .bsp files are attacker-controlled: a client downloads the map from whatever
// server it connects to. This drives Mod_LoadQ3BrushModel over arbitrary bytes
// with the renderer's dependencies stubbed out.
//
// Builds two ways:
//   BUILD_FUZZ_TEST  -> libFuzzer entry point (LLVMFuzzerTestOneInput)
//   BUILD_UNIT_TEST  -> main() that replays a corpus directory, so crashers
//                       found by the fuzzer stay regression-tested in CI
//
// A map that fails validation calls ri.Com_Error, which longjmps back here.
// That is a PASS: the loader correctly refused the file. Only a real crash,
// a sanitizer report, or a hang is a finding.

#include "r_local.h"

#include <setjmp.h>
#ifndef FUZZ_LIBFUZZER
#include <dirent.h>
#include <sys/stat.h>
#endif

#ifndef FUZZ_CORPUS_DIR
#define FUZZ_CORPUS_DIR "."
#endif

void Mod_LoadQ3BrushModel( model_t *mod, model_t *parent, void *buffer, size_t bufferLen, bspFormatDesc_t *format );

// ============================================================================
// tracked allocator
//
// The loader hangs everything off mod->mempool and never frees individually,
// so the harness keeps its own list and releases it after each input. Without
// this the fuzzer would OOM in seconds and every run would be a leak report.
// ============================================================================

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

static void *Fuzz_Alloc( size_t size, size_t alignment, bool zero )
{
	void *ptr = NULL;

	if( alignment < sizeof( void * ) )
		alignment = sizeof( void * );
	// posix_memalign wants a power-of-two multiple of sizeof(void*)
	if( alignment & ( alignment - 1 ) )
		alignment = 16;
	if( !size )
		size = 1;

	if( posix_memalign( &ptr, alignment, size ) != 0 )
		return NULL;
	if( zero )
		memset( ptr, 0, size );
	return Fuzz_Track( ptr );
}

// mod_mem.h interface, implemented directly rather than via
// MEM_DEFINE_INTERFACE_IMPL_SYSTEM so that everything can be reclaimed between
// iterations
void *__Q_Malloc( size_t size, const char *f, const char *fn, int line )
{
	return Fuzz_Alloc( size, 16, false );
}

void *__Q_Calloc( size_t count, size_t size, const char *f, const char *fn, int line )
{
	if( count && size > (size_t)-1 / count )
		return NULL;
	return Fuzz_Alloc( count * size, 16, true );
}

void *__Q_Realloc( void *ptr, size_t size, const char *f, const char *fn, int line )
{
	void *out;

	Fuzz_Untrack( ptr );
	out = realloc( ptr, size ? size : 1 );
	return Fuzz_Track( out );
}

void *__Q_MallocAligned( size_t alignment, size_t size, const char *f, const char *fn, int line )
{
	return Fuzz_Alloc( size, alignment, false );
}

void *__Q_CallocAligned( size_t count, size_t alignment, size_t size, const char *f, const char *fn, int line )
{
	if( count && size > (size_t)-1 / count )
		return NULL;
	return Fuzz_Alloc( count * size, alignment, true );
}

void Q_Free( void *ptr )
{
	Fuzz_Untrack( ptr );
	free( ptr );
}

mempool_t *Q_ParentPool( void ) { return NULL; }
mempool_t *Q_CreatePool( mempool_t *parent, const char *name ) { return NULL; }
void Q_LinkToPool( void *ptr, mempool_t *pool ) { }
void Q_FreePool( mempool_t *pool ) { }
void Q_EmptyPool( mempool_t *pool ) { }
void Mem_ValidationAllAllocations( void ) { }

struct mempool_stats_s Q_PoolStats( mempool_t *pool )
{
	struct mempool_stats_s stats = { 0 };
	return stats;
}

// ============================================================================
// renderer stubs
// ============================================================================

static jmp_buf fuzz_abortJmp;
static bool fuzz_inLoader;
static bool fuzz_verbose;

static void Fuzz_ComError( com_error_code_t code, const char *format, ... )
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

	// the real client longjmps out of ERR_DROP too
	if( fuzz_inLoader )
		longjmp( fuzz_abortJmp, 1 );

	abort();
}

static void Fuzz_ComPrintf( const char *format, ... )
{
	if( fuzz_verbose )
	{
		va_list ap;
		va_start( ap, format );
		vfprintf( stderr, format, ap );
		va_end( ap );
	}
}

// q_shared.c and a couple of spots in the loader call these directly rather
// than going through ri
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

ref_import_t ri;
mapconfig_t mapConfig;
r_shared_t rsh;
model_t *r_prevworldmodel;

// cvars the loader reads
static cvar_t fuzz_cvars[8];
cvar_t *r_fullbright = &fuzz_cvars[0];
cvar_t *r_lighting_vertexlight = &fuzz_cvars[1];
cvar_t *r_lighting_maxlmblocksize = &fuzz_cvars[2];
cvar_t *r_lighting_deluxemapping = &fuzz_cvars[3];
cvar_t *r_lighting_grayscale = &fuzz_cvars[4];
cvar_t *r_mapoverbrightbits = &fuzz_cvars[5];
cvar_t *r_subdivisions = &fuzz_cvars[6];

static shader_t fuzz_shader;

shader_t *R_RegisterShader( const char *name, shaderType_e type ) { return &fuzz_shader; }
void R_TouchShadersByName( const char *name ) { }
void R_FreeUnusedShadersByType( const shaderType_e *types, unsigned int numTypes ) { }
void R_FreeUnusedImagesByTags( int tags ) { }
void R_InitLightStyles( model_t *mod ) { }
void R_SortSuperLightStyles( model_t *mod ) { }

superLightStyle_t *R_AddSuperLightStyle( model_t *mod, const int *lightmaps,
	const uint8_t *lightmapStyles, const uint8_t *vertexStyles, mlightmapRect_t **lmRects )
{
	return NULL;
}

void R_BuildLightmaps( model_t *mod, int numLightmaps, int w, int h, const uint8_t *data, mlightmapRect_t *rects )
{
	int i;

	// the real implementation packs the lightmaps into atlases; the loader only
	// cares that every rect gets a texNum it can hand back out
	for( i = 0; i < numLightmaps; i++ )
	{
		rects[i].texNum = 0;
		rects[i].texLayer = 0;
		rects[i].texMatrix[0][0] = 1; rects[i].texMatrix[0][1] = 0;
		rects[i].texMatrix[1][0] = 1; rects[i].texMatrix[1][1] = 0;
	}
}

void R_BuildTangentVectors( int numVertexes, vec4_t *xyzArray, vec4_t *normalsArray,
	vec2_t *stArray, int numTris, elem_t *elems, vec4_t *sVectorsArray )
{
	// deliberately still reads what the loader handed it, so an out-of-range
	// element count shows up as an ASan report rather than being skipped
	int i;

	for( i = 0; i < numTris * 3; i++ )
	{
		elem_t e = elems[i];
		if( e >= (elem_t)numVertexes )
			continue;
		sVectorsArray[e][3] = xyzArray[e][0] + normalsArray[e][0] + stArray[e][0];
	}
}

// ============================================================================
// harness
// ============================================================================

static const modelFormatDescr_t fuzz_formats[] =
{
	{ "*", 4, q3BSPFormats, 0, ( const modelLoader_t )Mod_LoadQ3BrushModel },
	{ NULL, 0, NULL, 0, NULL }
};

static void Fuzz_Reset( void )
{
	memset( &mapConfig, 0, sizeof( mapConfig ) );
	mapConfig.lightmapsPacking = false;
	mapConfig.lightmapArrays = false;
	mapConfig.overbrightBits = 0;
	mapConfig.pow2MapOvrbr = 0;
	mapConfig.lightingIntensity = 1.0f;

	memset( &rsh, 0, sizeof( rsh ) );
	rsh.registrationSequence = 1;
	r_prevworldmodel = NULL;

	memset( fuzz_cvars, 0, sizeof( fuzz_cvars ) );
	r_lighting_maxlmblocksize->integer = 2048;
	r_lighting_maxlmblocksize->name = "r_lighting_maxlmblocksize";
	r_subdivisions->value = 5;
	r_mapoverbrightbits->dvalue = "2";

	memset( &fuzz_shader, 0, sizeof( fuzz_shader ) );
	fuzz_shader.name = "fuzz";
}

int Fuzz_LoadBSP( const uint8_t *data, size_t size )
{
	model_t mod;
	uint8_t *buf;
	const modelFormatDescr_t *descr;
	bspFormatDesc_t *bspFormat = NULL;
	int rejected = 0;

	Fuzz_Reset();

	descr = Q_FindFormatDescriptor( fuzz_formats, data, size, (const bspFormatDesc_t **)&bspFormat );
	if( !descr || !bspFormat )
		return -1;

	// mirror the trailing NUL the real file loader appends
	buf = malloc( size + 1 );
	if( !buf )
		abort();
	memcpy( buf, data, size );
	buf[size] = 0;

	memset( &mod, 0, sizeof( mod ) );
	mod.name = "fuzz.bsp";

	fuzz_inLoader = true;
	if( setjmp( fuzz_abortJmp ) == 0 )
		Mod_LoadQ3BrushModel( &mod, NULL, buf, size, bspFormat );
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

	memset( &ri, 0, sizeof( ri ) );
	ri.Com_Error = Fuzz_ComError;
	ri.Com_Printf = Fuzz_ComPrintf;
	ri.Com_DPrintf = Fuzz_ComPrintf;

	fuzz_verbose = getenv( "FUZZ_VERBOSE" ) != NULL;
}

#ifdef FUZZ_LIBFUZZER

int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	Fuzz_Init();
	Fuzz_LoadBSP( data, size );
	return 0;
}

#else

// corpus replay driver: every file given on the command line, or every file in
// the corpus directory, is pushed through the same entry point

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

	result = Fuzz_LoadBSP( data, (size_t)size );
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
			printf( "r_q3bsp_fuzz: no corpus at %s, nothing to replay\n", FUZZ_CORPUS_DIR );
			return 0;
		}

		printf( "r_q3bsp_fuzz: replaying corpus %s\n", FUZZ_CORPUS_DIR );
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
		printf( "r_q3bsp_fuzz: FAILED\n" );
		return 1;
	}

	printf( "r_q3bsp_fuzz: OK\n" );
	return 0;
}

#endif
