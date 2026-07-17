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
#ifndef R_PROGRAM_CACHE_H
#define R_PROGRAM_CACHE_H

#include "r_program.h"

// On-disk caches that let a warm start skip work the first run had to do:
//
//   cache/nri_pipeline.cache  - VkPipelineCache blob. Device-specific: only valid for the exact
//                               vendor/device/driver/pipelineCacheUUID that produced it.
//   cache/nri_glsl.cache      - text index: which (type, features, baseName, deformsKey)
//                               permutations to warm, and where each one's SPIR-V lives.
//   cache/nri_glsl.cache.bin  - the SPIR-V blobs themselves. Device-INDEPENDENT, unlike GL program
//                               binaries, so it is keyed on GLSL_BITS_VERSION alone and survives
//                               driver updates and GPU swaps. Kept separate from the pipeline cache
//                               for exactly that reason.
//
// All of it is best-effort. Any failure degrades to "no cache" -- it must never fail init, and it
// must never hand the driver bytes it has not validated.

void RP_InitPipelineCache( void );
void RP_StorePipelineCache( void );
void RP_ShutdownPipelineCache( void );

// SPIR-V blob cache. Read side.
void RP_SpirvCacheOpen( void );	 // load the index + open the blob; safe to call when disabled
void RP_SpirvCacheClose( void ); // release the index and the blob handle

// Fills program->shaderBin[] from the cache. Returns false on a miss or on any validation failure,
// in which case nothing is written to the program and the caller must compile.
bool RP_SpirvCacheLookup( struct glsl_program_s *program, int type, r_glslfeat_t features, const char *deformsKey );

// The precache index doubles as the SPIR-V blob directory, so both are rewritten together from the
// live program table. Call Begin, then Program for each program worth storing, then End to commit.
bool RP_SpirvCacheStoreBegin( void );
void RP_SpirvCacheStoreProgram( const struct glsl_program_s *program );
void RP_SpirvCacheStoreEnd( void );

// Number of entries the index held at load, for the precache replay to iterate.
size_t RP_SpirvCacheEntryCount( void );
bool RP_SpirvCacheEntryAt( size_t i, int *outType, r_glslfeat_t *outFeatures, const char **outBaseName, const char **outDeformsKey );

#if ( DEVICE_IMPL_VULKAN )
// VK_NULL_HANDLE is a legal argument to vkCreateGraphicsPipelines and is what this returns when the
// cache is disabled or could not be created.
VkPipelineCache RP_PipelineCache( void );
#endif

#endif // R_PROGRAM_CACHE_H
