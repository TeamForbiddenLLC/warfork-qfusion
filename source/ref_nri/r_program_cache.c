/*
Copyright (C) 2024 Warfork

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

#include "r_local.h"
#include "r_program_cache.h"
#include "stb_ds.h"

#define GLSL_CACHE_FILE_NAME "cache/nri_glsl.cache"
#define GLSL_BINARY_CACHE_FILE_NAME "cache/nri_glsl.cache.bin"

#define SPIRV_CACHE_MAGIC ( ( 'N' ) | ( 'S' << 8 ) | ( 'P' << 16 ) | ( 'V' << 24 ) )
#define SPIRV_RECORD_MAGIC ( ( 'S' ) | ( 'R' << 8 ) | ( 'E' << 16 ) | ( 'C' << 24 ) )
#define SPIRV_CACHE_HEADER_SIZE 12
#define SPIRV_WORD_MAGIC 0x07230203u // SpvMagicNumber -- a free, strong shape check
#define SPIRV_MIN_BYTES 20

#define MAX_CACHE_BASENAME 256
#define MAX_CACHE_DEFORMSKEY 1024

#define PIPELINE_CACHE_FILE_NAME "cache/nri_pipeline.cache"

#define PIPELINE_CACHE_MAGIC ( ( 'W' ) | ( 'F' << 8 ) | ( 'P' << 16 ) | ( 'C' << 24 ) )
#define PIPELINE_CACHE_FORMAT_VERSION 1u

// A blob larger than this is treated as corrupt rather than loaded. Nothing evicts, so this also
// bounds unbounded growth across driver updates and shader edits: trip the cap and we discard and
// start fresh, which costs exactly one cold session.
#define PIPELINE_CACHE_MAX_BYTES ( 128u * 1024u * 1024u )

// ---------------------------------------------------------------------------
// SPIR-V blob cache + precache index
// ---------------------------------------------------------------------------

struct spirv_cache_entry_s {
	int type;
	r_glslfeat_t features;
	int binaryPos; // byte offset into the blob; 0 means "no binary"
	char baseName[MAX_CACHE_BASENAME];
	char deformsKey[MAX_CACHE_DEFORMSKEY];
};

static struct spirv_cache_entry_s *r_spirvCacheIndex = NULL; // stb_ds array
static int r_spirvBlobHandle = 0;
static int r_spirvBlobSize = 0;

// store-side state
static int r_storeIndexHandle = 0;
static int r_storeBlobHandle = 0;
static bool r_storeActive = false;

size_t RP_SpirvCacheEntryCount( void )
{
	return (size_t)arrlen( r_spirvCacheIndex );
}

bool RP_SpirvCacheEntryAt( size_t i, int *outType, r_glslfeat_t *outFeatures, const char **outBaseName, const char **outDeformsKey )
{
	if( i >= (size_t)arrlen( r_spirvCacheIndex ) ) {
		return false;
	}
	const struct spirv_cache_entry_s *e = &r_spirvCacheIndex[i];
	*outType = e->type;
	*outFeatures = e->features;
	*outBaseName = e->baseName;
	*outDeformsKey = e->deformsKey;
	return true;
}

static void __RP_CloseSpirvBlob( void )
{
	if( r_spirvBlobHandle ) {
		FS_FCloseFile( r_spirvBlobHandle );
		r_spirvBlobHandle = 0;
	}
	r_spirvBlobSize = 0;
}

// Opens and header-checks the blob. The handle stays open for the session so a mid-frame miss is a
// seek+read rather than a reparse.
static void __RP_OpenSpirvBlob( void )
{
	uint32_t magic = 0;
	uint32_t version = 0;

	if( FS_FOpenFile( GLSL_BINARY_CACHE_FILE_NAME, &r_spirvBlobHandle, FS_READ | FS_CACHE ) == -1 ) {
		r_spirvBlobHandle = 0;
		return;
	}
	FS_Seek( r_spirvBlobHandle, 0, FS_SEEK_END );
	r_spirvBlobSize = FS_Tell( r_spirvBlobHandle );
	FS_Seek( r_spirvBlobHandle, 0, FS_SEEK_SET );

	if( r_spirvBlobSize < SPIRV_CACHE_HEADER_SIZE ) {
		ri.Com_DPrintf( "spirv cache: blob too small\n" );
		__RP_CloseSpirvBlob();
		return;
	}
	if( FS_Read( &magic, sizeof( magic ), r_spirvBlobHandle ) != sizeof( magic ) || magic != SPIRV_CACHE_MAGIC ) {
		ri.Com_DPrintf( "spirv cache: blob bad magic\n" );
		__RP_CloseSpirvBlob();
		return;
	}
	// version 0 means a previous run died between writing the blob and committing it
	if( FS_Read( &version, sizeof( version ), r_spirvBlobHandle ) != sizeof( version ) || version != GLSL_BITS_VERSION ) {
		ri.Com_DPrintf( "spirv cache: blob version %u, expected %u (or an interrupted write)\n", version, (unsigned)GLSL_BITS_VERSION );
		__RP_CloseSpirvBlob();
		return;
	}
}

void RP_SpirvCacheOpen( void )
{
	char *buffer = NULL;
	char *data;
	char **ptr;
	const char *token;
	char tempbuf[MAX_TOKEN_CHARS];

	if( !r_shaderCache->integer ) {
		return;
	}

	if( R_LoadCacheFile( GLSL_CACHE_FILE_NAME, (void **)&buffer ) == -1 || !buffer ) {
		return;
	}

	data = buffer;
	ptr = &data;

	token = COM_Parse_r( tempbuf, sizeof( tempbuf ), ptr );
	if( strcmp( token, rf.applicationName ) ) {
		ri.Com_DPrintf( "Ignoring %s: unknown application name \"%s\", expected \"%s\"\n", GLSL_CACHE_FILE_NAME, token, rf.applicationName );
		goto done;
	}

	token = COM_Parse_r( tempbuf, sizeof( tempbuf ), ptr );
	if( atoi( token ) != GLSL_BITS_VERSION ) {
		ri.Com_DPrintf( "Ignoring %s: found version %i, expected %i\n", GLSL_CACHE_FILE_NAME, atoi( token ), GLSL_BITS_VERSION );
		goto done;
	}

	__RP_OpenSpirvBlob();

	while( 1 ) {
		struct spirv_cache_entry_s entry = { 0 };

		token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, true );
		if( !token[0] ) {
			break;
		}
		// validate before it can ever index glsl_programtypes_features[type]
		entry.type = atoi( token );
		if( entry.type <= GLSL_PROGRAM_TYPE_NONE || entry.type >= GLSL_PROGRAM_TYPE_MAXTYPE ) {
			ri.Com_DPrintf( "spirv cache: bad program type %i, dropping the rest\n", entry.type );
			break;
		}

		// one unsigned hex token: a signed round-trip would corrupt bit 31/63, and ref_nri feature
		// bits already reach past 32
		token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
		if( !token[0] ) {
			break;
		}
		entry.features = (r_glslfeat_t)strtoull( token, NULL, 16 );

		token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
		if( !token[0] ) {
			break;
		}
		Q_strncpyz( entry.baseName, token, sizeof( entry.baseName ) );

		token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
		Q_strncpyz( entry.deformsKey, token, sizeof( entry.deformsKey ) );

		token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
		entry.binaryPos = atoi( token );

		if( !entry.baseName[0] ) {
			continue;
		}
		arrput( r_spirvCacheIndex, entry );
	}

	ri.Com_DPrintf( "spirv cache: %i entries indexed\n", (int)arrlen( r_spirvCacheIndex ) );

done:
	// single exit that always frees -- the equivalent ref_gl paths leak the buffer
	R_FreeFile( buffer );
}

void RP_SpirvCacheClose( void )
{
	__RP_CloseSpirvBlob();
	arrfree( r_spirvCacheIndex );
	r_spirvCacheIndex = NULL;
}

static const struct spirv_cache_entry_s *__RP_FindEntry( int type, r_glslfeat_t features, const char *deformsKey )
{
	// Linear, but only walked once per *new permutation* -- never per frame; r_glslprograms_hash
	// already absorbs the hot path.
	for( size_t i = 0; i < (size_t)arrlen( r_spirvCacheIndex ); i++ ) {
		const struct spirv_cache_entry_s *e = &r_spirvCacheIndex[i];
		if( e->type == type && e->features == features && !strcmp( e->deformsKey, deformsKey ) ) {
			return e;
		}
	}
	return NULL;
}

// Reads one 'SREC' record into program->shaderBin[]. Validates everything before the bytes could
// reach vkCreateShaderModule; on any failure frees whatever it read and returns false.
static bool __RP_ReadBlobRecord( struct glsl_program_s *program, int binaryPos )
{
	uint32_t recordMagic = 0;
	uint32_t numStages = 0;
	struct shader_bin_data_s loaded[GLSL_STAGE_MAX] = { 0 };
	size_t numLoaded = 0;

	if( binaryPos < SPIRV_CACHE_HEADER_SIZE || binaryPos >= r_spirvBlobSize ) {
		return false;
	}
	if( FS_Seek( r_spirvBlobHandle, binaryPos, FS_SEEK_SET ) < 0 ) {
		return false;
	}
	// a per-record magic is what keeps a stale offset from being read as a length
	if( FS_Read( &recordMagic, sizeof( recordMagic ), r_spirvBlobHandle ) != sizeof( recordMagic ) || recordMagic != SPIRV_RECORD_MAGIC ) {
		return false;
	}
	if( FS_Read( &numStages, sizeof( numStages ), r_spirvBlobHandle ) != sizeof( numStages ) || numStages == 0 || numStages > GLSL_STAGE_MAX ) {
		return false;
	}

	for( uint32_t i = 0; i < numStages; i++ ) {
		uint32_t stage = 0;
		uint32_t length = 0;
		uint32_t firstWord = 0;

		if( FS_Read( &stage, sizeof( stage ), r_spirvBlobHandle ) != sizeof( stage ) || stage >= GLSL_STAGE_MAX ) {
			goto fail;
		}
		if( FS_Read( &length, sizeof( length ), r_spirvBlobHandle ) != sizeof( length ) ) {
			goto fail;
		}
		if( length < SPIRV_MIN_BYTES || ( length % 4 ) != 0 || (int)length >= r_spirvBlobSize ) {
			goto fail;
		}

		uint8_t *bin = R_Malloc( length ); // allocator alignment, not a file offset
		if( FS_Read( bin, length, r_spirvBlobHandle ) != (int)length ) {
			R_Free( bin );
			goto fail;
		}
		memcpy( &firstWord, bin, sizeof( firstWord ) );
		if( firstWord != SPIRV_WORD_MAGIC ) {
			R_Free( bin );
			goto fail;
		}

		loaded[numLoaded].bin = (char *)bin;
		loaded[numLoaded].size = length;
		loaded[numLoaded].stage = (glsl_program_stage_t)stage;
		numLoaded++;
	}

	for( size_t i = 0; i < numLoaded; i++ ) {
		program->shaderBin[loaded[i].stage] = loaded[i];
	}
	return true;

fail:
	for( size_t i = 0; i < numLoaded; i++ ) {
		R_Free( loaded[i].bin );
	}
	return false;
}

bool RP_SpirvCacheLookup( struct glsl_program_s *program, int type, r_glslfeat_t features, const char *deformsKey )
{
	if( !r_shaderCache->integer || !r_spirvBlobHandle ) {
		return false;
	}
	const struct spirv_cache_entry_s *entry = __RP_FindEntry( type, features, deformsKey ? deformsKey : "" );
	if( !entry || !entry->binaryPos ) {
		return false;
	}
	if( !__RP_ReadBlobRecord( program, entry->binaryPos ) ) {
		// self-heal: a corrupt record means the blob is untrustworthy, so stop using it and let the
		// store at shutdown rewrite the whole thing from scratch
		Com_Printf( S_COLOR_YELLOW "Dropping corrupt SPIR-V cache.\n" );
		__RP_CloseSpirvBlob();
		return false;
	}
	return true;
}

bool RP_SpirvCacheStoreBegin( void )
{
	const uint32_t magic = SPIRV_CACHE_MAGIC;
	const uint32_t version = 0; // committed in RP_SpirvCacheStoreEnd
	const uint32_t reserved = 0;

	r_storeActive = false;
	if( !r_shaderCache->integer ) {
		return false;
	}

	// read-only install, full disk, ... -- degrade to "no cache", never assert
	if( FS_FOpenFile( GLSL_CACHE_FILE_NAME, &r_storeIndexHandle, FS_WRITE | FS_CACHE ) == -1 ) {
		Com_Printf( S_COLOR_YELLOW "Could not open %s for writing.\n", GLSL_CACHE_FILE_NAME );
		r_storeIndexHandle = 0;
		return false;
	}
	if( FS_FOpenFile( GLSL_BINARY_CACHE_FILE_NAME, &r_storeBlobHandle, FS_WRITE | FS_CACHE ) == -1 ) {
		Com_Printf( S_COLOR_YELLOW "Could not open %s for writing.\n", GLSL_BINARY_CACHE_FILE_NAME );
		FS_FCloseFile( r_storeIndexHandle );
		r_storeIndexHandle = 0;
		r_storeBlobHandle = 0;
		return false;
	}

	FS_Write( &magic, sizeof( magic ), r_storeBlobHandle );
	FS_Write( &version, sizeof( version ), r_storeBlobHandle );
	FS_Write( &reserved, sizeof( reserved ), r_storeBlobHandle );

	FS_Printf( r_storeIndexHandle, "%s\n", rf.applicationName );
	FS_Printf( r_storeIndexHandle, "%i\n", GLSL_BITS_VERSION );
	r_storeActive = true;
	return true;
}

void RP_SpirvCacheStoreProgram( const struct glsl_program_s *program )
{
	uint32_t numStages = 0;

	if( !r_storeActive ) {
		return;
	}
	// never cache a program whose stages failed to compile
	if( !program->valid || !program->baseName ) {
		return;
	}
	for( size_t i = 0; i < GLSL_STAGE_MAX; i++ ) {
		if( program->shaderBin[i].bin && program->shaderBin[i].size > 0 ) {
			numStages++;
		}
	}
	if( numStages == 0 ) {
		return;
	}

	const int binaryPos = FS_Tell( r_storeBlobHandle );
	const uint32_t recordMagic = SPIRV_RECORD_MAGIC;
	FS_Write( &recordMagic, sizeof( recordMagic ), r_storeBlobHandle );
	FS_Write( &numStages, sizeof( numStages ), r_storeBlobHandle );
	for( size_t i = 0; i < GLSL_STAGE_MAX; i++ ) {
		const struct shader_bin_data_s *bin = &program->shaderBin[i];
		if( !bin->bin || bin->size == 0 ) {
			continue;
		}
		const uint32_t stage = (uint32_t)i;
		const uint32_t length = (uint32_t)bin->size;
		FS_Write( &stage, sizeof( stage ), r_storeBlobHandle );
		FS_Write( &length, sizeof( length ), r_storeBlobHandle );
		FS_Write( bin->bin, length, r_storeBlobHandle );
	}

	FS_Printf( r_storeIndexHandle, "%i %016llx \"%s\" \"%s\" %i\n", program->type, (unsigned long long)program->features, program->baseName, program->deformsKey ? program->deformsKey : "", binaryPos );
}

void RP_SpirvCacheStoreEnd( void )
{
	if( !r_storeActive ) {
		return;
	}
	FS_FCloseFile( r_storeIndexHandle );
	FS_FCloseFile( r_storeBlobHandle );
	r_storeIndexHandle = 0;
	r_storeBlobHandle = 0;
	r_storeActive = false;

	// commit: stamp the real version only once the blob is safely on disk, so a crash mid-write
	// leaves version 0 and the loader rejects it
	int handle = 0;
	if( FS_FOpenFile( GLSL_BINARY_CACHE_FILE_NAME, &handle, FS_UPDATE | FS_CACHE ) != -1 ) {
		const uint32_t version = GLSL_BITS_VERSION;
		FS_Seek( handle, sizeof( uint32_t ), FS_SEEK_SET ); // past the magic
		FS_Write( &version, sizeof( version ), handle );
		FS_FCloseFile( handle );
	} else {
		Com_Printf( S_COLOR_YELLOW "Could not commit %s.\n", GLSL_BINARY_CACHE_FILE_NAME );
	}
}

#if ( DEVICE_IMPL_VULKAN )

// Our own header in front of the driver's blob. The driver validates its own embedded
// VkPipelineCacheHeaderVersionOne, but that header carries no length and no checksum -- a blob
// truncated by a crash or a full disk still has a byte-perfect 32-byte driver header, and truncation
// is the realistic corruption source. dataSize + dataHash are the fields that actually earn their
// keep; the vendor triple only catches a cache dir copied between machines.
//
// Field order keeps every member naturally aligned, so there is no implicit padding to hash over.
struct r_pipeline_cache_header_s {
	uint32_t magic;
	uint32_t formatVersion; // 0 while writing, PIPELINE_CACHE_FORMAT_VERSION once committed
	uint32_t vendorID;
	uint32_t deviceID;
	uint32_t driverVersion;
	uint32_t dataSize;
	uint64_t dataHash;
	uint8_t pipelineCacheUUID[VK_UUID_SIZE];
};

Q_COMPILE_ASSERT_MSG( sizeof( struct r_pipeline_cache_header_s ) == 48, "r_pipeline_cache_header_s has unexpected padding" );

static VkPipelineCache r_vkPipelineCache = VK_NULL_HANDLE;

VkPipelineCache RP_PipelineCache( void )
{
	return r_vkPipelineCache;
}

static void __RP_QueryDeviceProps( VkPhysicalDeviceProperties *props )
{
	memset( props, 0, sizeof( *props ) );
	vkGetPhysicalDeviceProperties( rsh.device.physicalAdapter.vk.physicalDevice, props );
}

// Returns true when payload is safe to hand to vkCreatePipelineCache as pInitialData.
static bool __RP_ValidatePipelineCacheBlob( const uint8_t *buf, size_t fileLen, const VkPhysicalDeviceProperties *props, const uint8_t **outPayload, uint32_t *outSize )
{
	struct r_pipeline_cache_header_s header;

	// checks are ordered cheapest-and-most-likely-to-fail first
	if( fileLen < sizeof( header ) ) {
		ri.Com_DPrintf( "pipeline cache: too small (%u bytes)\n", (unsigned)fileLen );
		return false;
	}
	memcpy( &header, buf, sizeof( header ) );

	if( header.magic != PIPELINE_CACHE_MAGIC ) {
		ri.Com_DPrintf( "pipeline cache: bad magic\n" );
		return false;
	}
	// formatVersion 0 means a previous run was killed between writing the payload and committing
	if( header.formatVersion != PIPELINE_CACHE_FORMAT_VERSION ) {
		ri.Com_DPrintf( "pipeline cache: version %u, expected %u (or an interrupted write)\n", header.formatVersion, PIPELINE_CACHE_FORMAT_VERSION );
		return false;
	}
	if( header.dataSize == 0 || header.dataSize > PIPELINE_CACHE_MAX_BYTES || header.dataSize != fileLen - sizeof( header ) ) {
		ri.Com_DPrintf( "pipeline cache: truncated (dataSize %u, have %u)\n", header.dataSize, (unsigned)( fileLen - sizeof( header ) ) );
		return false;
	}
	if( header.vendorID != props->vendorID || header.deviceID != props->deviceID || header.driverVersion != props->driverVersion ) {
		ri.Com_DPrintf( "pipeline cache: built for a different device/driver\n" );
		return false;
	}
	if( memcmp( header.pipelineCacheUUID, props->pipelineCacheUUID, VK_UUID_SIZE ) != 0 ) {
		ri.Com_DPrintf( "pipeline cache: pipelineCacheUUID changed, driver invalidated it\n" );
		return false;
	}

	const uint8_t *payload = buf + sizeof( header );
	if( hash_data_hsieh( HASH_INITIAL_VALUE, payload, header.dataSize ) != header.dataHash ) {
		ri.Com_DPrintf( "pipeline cache: checksum mismatch\n" );
		return false;
	}

	// last line of defense: sanity-check the driver's own header before it ever sees the bytes
	{
		uint32_t embHeaderSize = 0;
		uint32_t embHeaderVersion = 0;
		if( header.dataSize < 8 ) {
			return false;
		}
		memcpy( &embHeaderSize, payload, sizeof( embHeaderSize ) );
		memcpy( &embHeaderVersion, payload + 4, sizeof( embHeaderVersion ) );
		if( embHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE || embHeaderSize < 8 || embHeaderSize > header.dataSize ) {
			ri.Com_DPrintf( "pipeline cache: bad driver header\n" );
			return false;
		}
	}

	*outPayload = payload;
	*outSize = header.dataSize;
	return true;
}

void RP_InitPipelineCache( void )
{
	uint8_t *buf = NULL;
	const uint8_t *payload = NULL;
	uint32_t payloadSize = 0;
	VkPhysicalDeviceProperties props;

	r_vkPipelineCache = VK_NULL_HANDLE;
	if( !r_shaderCache->integer ) {
		return;
	}

	__RP_QueryDeviceProps( &props );

	const int fileLen = R_LoadCacheFile( PIPELINE_CACHE_FILE_NAME, (void **)&buf );
	if( buf && fileLen > 0 ) {
		if( !__RP_ValidatePipelineCacheBlob( buf, (size_t)fileLen, &props, &payload, &payloadSize ) ) {
			payload = NULL;
			payloadSize = 0;
		}
	}

	VkPipelineCacheCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	createInfo.initialDataSize = payloadSize;
	createInfo.pInitialData = payload;
	// deliberately not VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT: the driver's internal
	// locking is what keeps this safe if a pipeline is ever created off the render thread.
	if( vkCreatePipelineCache( rsh.device.vk.device, &createInfo, NULL, &r_vkPipelineCache ) != VK_SUCCESS ) {
		// not fatal -- VK_NULL_HANDLE is exactly what vkCreateGraphicsPipelines got before this existed
		Com_Printf( S_COLOR_YELLOW "Could not create pipeline cache, pipelines will compile cold.\n" );
		r_vkPipelineCache = VK_NULL_HANDLE;
	} else if( payloadSize > 0 ) {
		ri.Com_DPrintf( "pipeline cache: loaded %u bytes\n", payloadSize );
	}

	if( buf ) {
		R_FreeFile( buf );
	}
}

void RP_StorePipelineCache( void )
{
	size_t dataSize = 0;
	int handle = 0;

	if( !r_shaderCache->integer || r_vkPipelineCache == VK_NULL_HANDLE ) {
		return;
	}

	if( vkGetPipelineCacheData( rsh.device.vk.device, r_vkPipelineCache, &dataSize, NULL ) != VK_SUCCESS || dataSize == 0 ) {
		return;
	}
	if( dataSize > PIPELINE_CACHE_MAX_BYTES ) {
		Com_Printf( S_COLOR_YELLOW "Pipeline cache is %u bytes, over the %u cap; not storing.\n", (unsigned)dataSize, (unsigned)PIPELINE_CACHE_MAX_BYTES );
		return;
	}

	uint8_t *data = R_Malloc( dataSize );
	// a short read would mean writing a blob whose dataSize lies; drop it instead
	if( vkGetPipelineCacheData( rsh.device.vk.device, r_vkPipelineCache, &dataSize, data ) != VK_SUCCESS ) {
		R_Free( data );
		return;
	}

	struct r_pipeline_cache_header_s header = { 0 };
	VkPhysicalDeviceProperties props;
	__RP_QueryDeviceProps( &props );

	header.magic = PIPELINE_CACHE_MAGIC;
	header.formatVersion = 0; // committed at the end, so an interrupted write stays rejectable
	header.vendorID = props.vendorID;
	header.deviceID = props.deviceID;
	header.driverVersion = props.driverVersion;
	header.dataSize = (uint32_t)dataSize;
	header.dataHash = hash_data_hsieh( HASH_INITIAL_VALUE, data, dataSize );
	memcpy( header.pipelineCacheUUID, props.pipelineCacheUUID, VK_UUID_SIZE );

	if( FS_FOpenFile( PIPELINE_CACHE_FILE_NAME, &handle, FS_WRITE | FS_CACHE ) == -1 ) {
		// read-only install dir, full disk, ... -- degrade to "no cache", never assert
		Com_Printf( S_COLOR_YELLOW "Could not open %s for writing.\n", PIPELINE_CACHE_FILE_NAME );
		R_Free( data );
		return;
	}
	FS_Write( &header, sizeof( header ), handle );
	FS_Write( data, dataSize, handle );
	FS_FCloseFile( handle );
	R_Free( data );

	// commit: stamp the real version only once the payload is safely on disk
	handle = 0;
	if( FS_FOpenFile( PIPELINE_CACHE_FILE_NAME, &handle, FS_UPDATE | FS_CACHE ) != -1 ) {
		const uint32_t version = PIPELINE_CACHE_FORMAT_VERSION;
		FS_Seek( handle, offsetof( struct r_pipeline_cache_header_s, formatVersion ), FS_SEEK_SET );
		FS_Write( &version, sizeof( version ), handle );
		FS_FCloseFile( handle );
		ri.Com_DPrintf( "pipeline cache: stored %u bytes\n", (unsigned)dataSize );
	} else {
		Com_Printf( S_COLOR_YELLOW "Could not commit %s.\n", PIPELINE_CACHE_FILE_NAME );
	}
}

void RP_ShutdownPipelineCache( void )
{
	if( r_vkPipelineCache != VK_NULL_HANDLE ) {
		vkDestroyPipelineCache( rsh.device.vk.device, r_vkPipelineCache, NULL );
		r_vkPipelineCache = VK_NULL_HANDLE;
	}
}

#else

void RP_InitPipelineCache( void ) {}
void RP_StorePipelineCache( void ) {}
void RP_ShutdownPipelineCache( void ) {}

#endif // DEVICE_IMPL_VULKAN
