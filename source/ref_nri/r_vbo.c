/*
Copyright (C) 2011 Victor Luchits

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

#include "r_gpu_ring_buffer.h"
#include "r_local.h"
#include "qmesa.h"
#include "r_resource_upload.h"

#include "stb_ds.h"
/*
=========================================================

VERTEX BUFFER OBJECTS

=========================================================
*/

typedef struct vbohandle_s
{
	unsigned int index;
	mesh_vbo_t *vbo;
	struct vbohandle_s *prev, *next;
} vbohandle_t;

#define MAX_MESH_VERTEX_BUFFER_OBJECTS 	0x8000

#define VBO_USAGE_FOR_TAG(tag) \
	(GLenum)((tag) == VBO_TAG_STREAM ? GL_DYNAMIC_DRAW_ARB : GL_STATIC_DRAW_ARB)

static mesh_vbo_t r_mesh_vbo[MAX_MESH_VERTEX_BUFFER_OBJECTS];

static vbohandle_t r_vbohandles[MAX_MESH_VERTEX_BUFFER_OBJECTS];
static vbohandle_t r_vbohandles_headnode, *r_free_vbohandles;

static elem_t *r_vbo_tempelems;
static unsigned r_vbo_numtempelems;

static void *r_vbo_tempvsoup;
static size_t r_vbo_tempvsoupsize;

static int r_num_active_vbos;

static elem_t *R_VBOElemBuffer( unsigned numElems );
static void *R_VBOVertBuffer( unsigned numVerts, size_t vertSize );

/*
 * R_InitVBO
 */
void R_InitVBO( void )
{
	int i;

	r_vbo_tempelems = NULL;
	r_vbo_numtempelems = 0;

	r_vbo_tempvsoup = NULL;
	r_vbo_tempvsoupsize = 0;

	r_num_active_vbos = 0;

	memset( r_mesh_vbo, 0, sizeof( r_mesh_vbo ) );
	memset( r_vbohandles, 0, sizeof( r_vbohandles ) );

	// link vbo handles
	r_free_vbohandles = r_vbohandles;
	r_vbohandles_headnode.prev = &r_vbohandles_headnode;
	r_vbohandles_headnode.next = &r_vbohandles_headnode;
	for( i = 0; i < MAX_MESH_VERTEX_BUFFER_OBJECTS; i++ ) {
		r_vbohandles[i].index = i;
		r_vbohandles[i].vbo = &r_mesh_vbo[i];
	}
	for( i = 0; i < MAX_MESH_VERTEX_BUFFER_OBJECTS - 1; i++ ) {
		r_vbohandles[i].next = &r_vbohandles[i+1];
	}
}




/*
 * R_CreateMeshVBO
 *
 * Create two static buffer objects: vertex buffer and elements buffer, the real
 * data is uploaded by calling R_UploadVBOVertexData and R_UploadVBOElemData.
 *
 * Tag allows vertex buffer objects to be grouped and released simultaneously.
 */
mesh_vbo_t *R_CreateMeshVBO(const struct mesh_vbo_desc_s* desc)
{
	assert( desc );
	assert( desc->numVerts > 0 );
	assert( desc->numElems > 0 );

	vattribbit_t lmattrbit;
	vattribbit_t halfFloatVattribs = desc->halfFloatVattribs; 
	if( !( halfFloatVattribs & VATTRIB_POSITION_BIT ) ) {
		halfFloatVattribs &= ~( VATTRIB_AUTOSPRITE_BIT );
	}

	halfFloatVattribs &= ~VATTRIB_COLORS_BITS;
	halfFloatVattribs &= ~VATTRIB_BONES_BITS;

	// TODO: convert quaternion component of instance_t to half-float
	// when uploading instances data
	halfFloatVattribs &= ~VATTRIB_INSTANCES_BITS;

	size_t vertexByteStride = 0;
	const size_t instanceByteStride = desc->numInstances * sizeof( instancePoint_t );
	const bool hasInstanceBuffer = ( ( desc->vattribs & VATTRIB_INSTANCES_BITS ) == VATTRIB_INSTANCES_BITS ) && desc->numInstances;

	vbohandle_t *vboh = r_free_vbohandles;
	mesh_vbo_t *vbo = &r_mesh_vbo[vboh->index];
	memset( vbo, 0, sizeof( *vbo ) );
	vbo->index = vboh->index + 1;
	r_free_vbohandles = vboh->next;

	// link to the list of active vbo handles
	vboh->prev = &r_vbohandles_headnode;
	vboh->next = r_vbohandles_headnode.next;
	vboh->next->prev = vboh;
	vboh->prev->next = vboh;

	// vertex buffer
	{
		vertexByteStride += FLOAT_VATTRIB_SIZE( VATTRIB_POSITION_BIT, halfFloatVattribs ) * 4;
		// normals data
		if( desc->vattribs & VATTRIB_NORMAL_BIT ) {
			assert( !( vertexByteStride & 3 ) );
			vbo->normalsOffset = vertexByteStride;
			vertexByteStride += FLOAT_VATTRIB_SIZE( VATTRIB_NORMAL_BIT, halfFloatVattribs ) * 4;
		}

		// s-vectors (tangent vectors)
		if( desc->vattribs & VATTRIB_SVECTOR_BIT ) {
			assert( !( vertexByteStride & 3 ) );
			vbo->sVectorsOffset = vertexByteStride;
			vertexByteStride += FLOAT_VATTRIB_SIZE( VATTRIB_SVECTOR_BIT, halfFloatVattribs ) * 4;
		}

		// texture coordinates
		if( desc->vattribs & VATTRIB_TEXCOORDS_BIT ) {
			assert( !( vertexByteStride & 3 ) );
			vbo->stOffset = vertexByteStride;
			vertexByteStride += FLOAT_VATTRIB_SIZE( VATTRIB_TEXCOORDS_BIT, halfFloatVattribs ) * 2;
		}

		// lightmap texture coordinates
		lmattrbit = VATTRIB_LMCOORDS0_BIT;
		for( size_t i = 0; i < ( MAX_LIGHTMAPS + 1 ) / 2; i++ ) {
			if( !( desc->vattribs & lmattrbit ) ) {
				break;
			}
			assert( !( vertexByteStride & 3 ) );
			vbo->lmstOffset[i] = vertexByteStride;
			vbo->lmstSize[i] = ( desc->vattribs & ( lmattrbit << 1 ) ) ? 4 : 2;
			vertexByteStride += FLOAT_VATTRIB_SIZE( VATTRIB_LMCOORDS0_BIT, halfFloatVattribs ) * vbo->lmstSize[i];
			lmattrbit = (vattribbit_t)( (vattribmask_t)lmattrbit << 2 );
		}

		// vertex colors
		if( desc->vattribs & VATTRIB_COLOR0_BIT )
		{
			assert( !( vertexByteStride & 3 ) );
			vbo->colorsOffset[0] = vertexByteStride;
			vertexByteStride += sizeof( int );
		}
	
		// bones data for skeletal animation
		if( ( desc->vattribs & VATTRIB_BONES_BITS ) == VATTRIB_BONES_BITS ) {
			assert( SKM_MAX_WEIGHTS == 4 );

			assert( !( vertexByteStride & 3 ) );
			vbo->bonesIndicesOffset = vertexByteStride;
			vertexByteStride += sizeof( int );

			assert( !( vertexByteStride & 3 ) );
			vbo->bonesWeightsOffset = vertexByteStride;
			vertexByteStride += sizeof( uint32_t );
		}

		// autosprites
		// FIXME: autosprite2 requires waaaay too much data for such a trivial
		// transformation..
		if( ( desc->vattribs & VATTRIB_AUTOSPRITE_BIT ) == VATTRIB_AUTOSPRITE_BIT ) {
			assert( !( vertexByteStride & 3 ) );
			vbo->spritePointsOffset = vertexByteStride;
			vertexByteStride += FLOAT_VATTRIB_SIZE( VATTRIB_AUTOSPRITE_BIT, halfFloatVattribs ) * 4;
		}
		NriBufferDesc vertexBufferDesc = { 
			.size = vertexByteStride * desc->numVerts, 
			.usage = NriBufferUsageBits_VERTEX_BUFFER 
		};
		rsh.nri.coreI.CreateBuffer( rsh.nri.device, &vertexBufferDesc, &vbo->vertexBuffer );
	}

	NriBufferDesc indexBufferDesc = { .size = desc->numElems * sizeof( elem_t ), .usage = NriBufferUsageBits_INDEX_BUFFER };
	rsh.nri.coreI.CreateBuffer( rsh.nri.device, &indexBufferDesc, &vbo->indexBuffer );

	if( hasInstanceBuffer ) {
		vbo->instancesOffset = instanceByteStride;
		NriBufferDesc instanceBufferDesc = { 
			.size = instanceByteStride * desc->numInstances, 
			.usage = NriBufferUsageBits_CONSTANT_BUFFER };
		rsh.nri.coreI.CreateBuffer( rsh.nri.device, &instanceBufferDesc, &vbo->instanceBuffer );
	}

	NriBuffer *nriBuffers[] = {
		vbo->vertexBuffer,
		vbo->indexBuffer,
		vbo->instanceBuffer,
	};

	NriResourceGroupDesc resourceGroupDesc = {
		.bufferNum = 2 + ( hasInstanceBuffer ? 1 : 0 ),
		.buffers = nriBuffers,
		.memoryLocation = desc->memoryLocation,
	};
	const uint32_t allocationNum = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
	vbo->numAllocations = allocationNum;
	R_VK_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, vbo->memory ) );

	if(hasInstanceBuffer) {
		vbo->ringOffsetInstAlloc = (struct r_ring_offset_alloc_s){
			.currentOffset = 0,
			.maxSize = instanceByteStride * desc->numInstances,
			.bufferAlignment = 1
		};
	}

	vbo->ringOffsetIndexAlloc = (struct r_ring_offset_alloc_s){
		.currentOffset = 0,
		.maxSize = desc->numElems * sizeof( elem_t ),
		.bufferAlignment = 1
	};
	vbo->ringOffsetVertAlloc = (struct r_ring_offset_alloc_s){
		.currentOffset = 0,
		.maxSize =  vertexByteStride * desc->numVerts,
		.bufferAlignment = 1
	};

	r_num_active_vbos++;

	vbo->registrationSequence = rsh.registrationSequence;
	vbo->vertexSize = vertexByteStride;
	vbo->numVerts = desc->numVerts;
	vbo->numElems = desc->numElems;
	vbo->owner = desc->owner;
	vbo->tag = desc->tag;
	vbo->vertexAttribs = desc->vattribs;
	vbo->halfFloatAttribs = halfFloatVattribs;
	return vbo;
}

void R_UploadVBOVertexRawData( mesh_vbo_t *vbo, int vertsOffset, int numVerts, const void *data )
{
	assert( vbo != NULL );
	if( !vbo || !vbo->vertexBuffer ) {
		return;
	}

	if( vbo->tag != VBO_TAG_STREAM ) {
		R_DeferDataSync();
	}

	buffer_upload_desc_t uploadDesc = {
		.target = vbo->vertexBuffer,
		.numBytes = numVerts * vbo->vertexSize,
		.byteOffset = vertsOffset * vbo->vertexSize,
		.after = ( NriAccessStage ){
			.access = NriAccessBits_VERTEX_BUFFER,
			.stages = NriStageBits_VERTEX_SHADER
		},
	};
	// vbo->vertexStage = R_ResourceTransitionBuffer( vbo->vertexBuffer, ( NriAccessStage ){} );
	R_ResourceBeginCopyBuffer( &uploadDesc );
	memcpy( uploadDesc.data, data, uploadDesc.numBytes );
	R_ResourceEndCopyBuffer( &uploadDesc );
}

/*
 * R_TouchMeshVBO
 */
void R_TouchMeshVBO( mesh_vbo_t *vbo )
{
	vbo->registrationSequence = rsh.registrationSequence;
}

/*
 * R_VBOByIndex
 */
mesh_vbo_t *R_GetVBOByIndex( int index )
{
	if( index >= 1 && index <= MAX_MESH_VERTEX_BUFFER_OBJECTS ) {
		return r_mesh_vbo + index - 1;
	}
	return NULL;
}

void R_ReleaseMeshVBO( mesh_vbo_t *vbo )
{
	// TODO: need to see if this crashes because of unrefrenced assets
	
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();	
	if( vbo->vertexBuffer ) {
		// rsh.nri.coreI.DestroyBuffer( vbo->vertexBuffer );
		arrpush( cmd->freeBuffers, vbo->vertexBuffer );
	}
	if( vbo->indexBuffer ) {
		// rsh.nri.coreI.DestroyBuffer( vbo->indexBuffer );
		arrpush( cmd->freeBuffers, vbo->indexBuffer );
	}
	if( vbo->instanceBuffer ) {
		// rsh.nri.coreI.DestroyBuffer( vbo->instanceBuffer );
		arrpush( cmd->freeBuffers, vbo->instanceBuffer );
	}
	for( size_t i = 0; i < vbo->numAllocations; i++ ) {
		// rsh.nri.coreI.FreeMemory( vbo->memory[i] );
		arrpush( cmd->freeMemory, vbo->memory[i] );
		
	}

	if( vbo->index >= 1 && vbo->index <= MAX_MESH_VERTEX_BUFFER_OBJECTS ) {
		vbohandle_t *vboh = &r_vbohandles[vbo->index - 1];

		// remove from linked active list
		vboh->prev->next = vboh->next;
		vboh->next->prev = vboh->prev;

		// insert into linked free list
		vboh->next = r_free_vbohandles;
		r_free_vbohandles = vboh;

		r_num_active_vbos--;
	}

	memset( vbo, 0, sizeof( *vbo ) );
	vbo->tag = VBO_TAG_NONE;
}

int R_GetNumberOfActiveVBOs( void )
{
	return r_num_active_vbos;
}

/*
 * R_FillVertexBuffer
 */
#define R_FillVertexBuffer( intype, outtype, in, size, stride, numVerts, out ) R_FillVertexBuffer##intype##outtype( in, size, stride, numVerts, (void *)( out ) )

#define R_FillVertexBuffer_f( intype, outtype, conv )                                                                          \
	static void R_FillVertexBuffer##intype##outtype( intype *in, size_t size, size_t stride, unsigned numVerts, outtype *out ) \
	{                                                                                                                          \
		size_t i, j;                                                                                                           \
		for( i = 0; i < numVerts; i++ ) {                                                                                      \
			for( j = 0; j < size; j++ ) {                                                                                      \
				out[j] = conv( *in++ );                                                                                        \
			}                                                                                                                  \
			out = (outtype *)( (uint8_t *)out + stride );                                                                      \
		}                                                                                                                      \
	}

R_FillVertexBuffer_f( float, float, );
R_FillVertexBuffer_f( float, GLhalfARB, _mesa_float_to_half );
R_FillVertexBuffer_f( int, int, );
#define R_FillVertexBuffer_float_or_half( gl_type, in, size, stride, numVerts, out ) \
	do {                                                                             \
		if( gl_type == GL_HALF_FLOAT ) {                                             \
			R_FillVertexBuffer( float, GLhalfARB, in, size, stride, numVerts, out ); \
		} else {                                                                     \
			R_FillVertexBuffer( float, float, in, size, stride, numVerts, out );     \
		}                                                                            \
	} while( 0 )


void R_FillNriVertexAttrib(mesh_vbo_t* vbo, NriVertexAttributeDesc* desc, size_t* numDesc) {
	desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
		.offset = 0, 
		.format = ( vbo->halfFloatAttribs & VATTRIB_POSITION_BIT ) ? NriFormat_RGBA16_SFLOAT : NriFormat_RGBA32_SFLOAT, 
		.vk = { VATTRIB_POSITION }, 
		.d3d = {.semanticName = "POSITION", .semanticIndex = VATTRIB_POSITION  },
		.streamIndex = 0 };

	if(  ( vbo->vertexAttribs & VATTRIB_NORMAL_BIT ) ) {
		desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
			.offset = vbo->normalsOffset, 
			.format = ( vbo->halfFloatAttribs & VATTRIB_NORMAL_BIT ) ? NriFormat_RGBA16_SFLOAT : NriFormat_RGBA32_SFLOAT, 
			.vk = { VATTRIB_NORMAL }, 
			.d3d = {.semanticName = "NORMAL", .semanticIndex = VATTRIB_NORMAL   },
			.streamIndex = 0 };
	}
	
	if( vbo->vertexAttribs & VATTRIB_SVECTOR_BIT ) {
		desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
			.offset = vbo->sVectorsOffset, 
			.format = ( vbo->halfFloatAttribs & VATTRIB_NORMAL_BIT ) ? NriFormat_RGBA16_SFLOAT : NriFormat_RGBA32_SFLOAT, 
			.vk = { VATTRIB_SVECTOR }, 
			.d3d = {.semanticName = "TANGENT", .semanticIndex = VATTRIB_SVECTOR   },
			.streamIndex = 0 };
	}
	
	if( vbo->vertexAttribs & VATTRIB_COLOR0_BIT ) {
		desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
			.offset = vbo->colorsOffset[0], 
			.format = NriFormat_RGBA8_UNORM, 
			.vk = { VATTRIB_COLOR0 }, 
			.d3d = {.semanticName = "COLOR0", .semanticIndex = VATTRIB_COLOR0 },
			.streamIndex = 0 
		};
	}

	if( ( vbo->vertexAttribs & VATTRIB_TEXCOORDS_BIT ) ) {
		desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
			.offset = vbo->stOffset, 
			.format = ( vbo->halfFloatAttribs & VATTRIB_TEXCOORDS_BIT  ) ? NriFormat_RG16_SFLOAT : NriFormat_RG32_SFLOAT, 
			.vk = { VATTRIB_TEXCOORDS }, 
			.d3d = {.semanticName = "TEXCOORD0", .semanticIndex = VATTRIB_TEXCOORDS },
			.streamIndex = 0 };
	}

	if( (vbo->vertexAttribs & VATTRIB_AUTOSPRITE_BIT) == VATTRIB_AUTOSPRITE_BIT ) {
		desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
			.offset = vbo->spritePointsOffset, 
			.format = ( vbo->halfFloatAttribs & VATTRIB_AUTOSPRITE_BIT  ) ? NriFormat_RGBA16_SFLOAT : NriFormat_RGBA32_SFLOAT, 
			.vk = { VATTRIB_SPRITEPOINT }, 
			.d3d = {.semanticName = "TEXCOORD1", .semanticIndex = VATTRIB_SPRITEPOINT },
			.streamIndex = 0 };
	}
	
	if( (vbo->vertexAttribs & VATTRIB_BONES_BITS) == VATTRIB_BONES_BITS ) {
		desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
			.offset = vbo->bonesIndicesOffset , 
			.format = NriFormat_RGBA8_UINT, 
			.vk = { VATTRIB_BONESINDICES }, 
			.d3d = {.semanticName = "TEXCOORD2", .semanticIndex = VATTRIB_BONESINDICES  },
			.streamIndex = 0 };
		
		desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
			.offset = vbo->bonesWeightsOffset , 
			.format = NriFormat_RGBA8_UNORM, 
			.vk = { VATTRIB_BONESWEIGHTS }, 
			.d3d = {.semanticName = "TEXCOORD3", .semanticIndex = VATTRIB_BONESWEIGHTS  },
			.streamIndex = 0 };

	} else {

		// lightmap texture coordinates - aliasing bones, so not disabling bones
		vattrib_t lmattr = VATTRIB_LMCOORDS01;
		vattribbit_t lmattrbit = VATTRIB_LMCOORDS0_BIT;

		for(size_t i = 0; i < ( MAX_LIGHTMAPS + 1 ) / 2; i++ ) {
			if( vbo->vertexAttribs & lmattrbit ) {

				NriFormat format =  ( vbo->halfFloatAttribs & VATTRIB_LMCOORDS0_BIT  ) ? NriFormat_R16_SFLOAT: NriFormat_R32_SFLOAT;
				switch (vbo->lmstSize[i]) {
					case 2:
						format =  ( vbo->halfFloatAttribs & VATTRIB_LMCOORDS0_BIT  ) ? NriFormat_RG16_SFLOAT: NriFormat_RG32_SFLOAT;
						break;
					case 4:
						format =  ( vbo->halfFloatAttribs & VATTRIB_LMCOORDS0_BIT  ) ? NriFormat_RGBA16_SFLOAT: NriFormat_RGBA32_SFLOAT;
						break;
					default:
						assert(false);
						break;
				}
				desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
					.offset = vbo->lmstOffset[i], 
					.format = format, 
					.vk = { lmattr }, 
					.d3d = {.semanticName = "TEXCOORD4", .semanticIndex = lmattr  },
					.streamIndex = 0 
				};
				

				
				//qglVertexAttribPointerARB( lmattr, vbo->lmstSize[i], 
				//	FLOAT_VATTRIB_GL_TYPE( VATTRIB_LMCOORDS0_BIT, hfa ), 
				//	GL_FALSE, vbo->vertexSize, ( const GLvoid * )vbo->lmstOffset[i] );
			}

			lmattr++;
			lmattrbit <<= 2;
		}

		// lightmap array texture layers
		lmattr = VATTRIB_LMLAYERS0123;

		for(size_t i = 0; i < ( MAX_LIGHTMAPS + 3 ) / 4; i++ ) {
			if( vbo->vertexAttribs & ( VATTRIB_LMLAYERS0123_BIT << i ) ) {
				desc[( *numDesc )++] = ( NriVertexAttributeDesc ){
					.offset = vbo->lmlayersOffset[i], 
					.format = NriFormat_R8_UINT, 
					.vk = { lmattr }, 
					.d3d = {.semanticName = "TEXCOORD5", .semanticIndex = lmattr  },
					.streamIndex = 0 
				};
				//qglVertexAttribPointerARB( lmattr, 4, GL_UNSIGNED_BYTE,
				//	GL_FALSE, vbo->vertexSize, ( const GLvoid * )vbo->lmlayersOffset[i] );
			}

			lmattr++;
		}

	}

	const vattribbit_t lightMapAttrs[] = {
		VATTRIB_LMCOORDS0_BIT,	
	};

	if( vbo->lmstOffset[0] && ( vbo->vertexAttribs & VATTRIB_LMCOORDS0_BIT ) ) {
	}

}


/*
 * R_FillVBOVertexDataBuffer
 *
 * Generates required vertex data to be uploaded to the buffer.
 *
 * Vertex attributes masked by halfFloatVattribs will use half-precision floats
 * to save memory, if GL_ARB_half_float_vertex is available. Note that if
 * VATTRIB_POSITION_BIT is not set, it will also reset bits for other positional
 * attributes such as autosprite pos and instance pos.
 */
vattribmask_t R_FillVBOVertexDataBuffer( mesh_vbo_t *vbo, vattribmask_t vattribs, const mesh_t *mesh, void *outData )
{
	int i, j;
	unsigned numVerts;
	size_t vertSize;
	vattribmask_t errMask;
	vattribmask_t hfa;
	uint8_t *data = outData;

	assert( vbo != NULL );
	assert( mesh != NULL );

	if( !vbo ) {
		return 0;
	}

	errMask = 0;
	numVerts = mesh->numVerts;
	vertSize = vbo->vertexSize;

	hfa = vbo->halfFloatAttribs;

	// upload vertex xyz data
	if( vattribs & VATTRIB_POSITION_BIT ) {
		if( !mesh->xyzArray ) {
			errMask |= VATTRIB_POSITION_BIT;
		} else {
			R_FillVertexBuffer_float_or_half( FLOAT_VATTRIB_GL_TYPE( VATTRIB_POSITION_BIT, hfa ), mesh->xyzArray[0], 4, vertSize, numVerts, data + 0 );
		}
	}

	// upload normals data
	if( vbo->normalsOffset && ( vattribs & VATTRIB_NORMAL_BIT ) ) {
		if( !mesh->normalsArray ) {
			errMask |= VATTRIB_NORMAL_BIT;
		} else {
			R_FillVertexBuffer_float_or_half( FLOAT_VATTRIB_GL_TYPE( VATTRIB_NORMAL_BIT, hfa ), mesh->normalsArray[0], 4, vertSize, numVerts, data + vbo->normalsOffset );
		}
	}

	// upload tangent vectors
	if( vbo->sVectorsOffset && ( ( vattribs & ( VATTRIB_SVECTOR_BIT | VATTRIB_AUTOSPRITE2_BIT ) ) == VATTRIB_SVECTOR_BIT ) ) {
		if( !mesh->sVectorsArray ) {
			errMask |= VATTRIB_SVECTOR_BIT;
		} else {
			R_FillVertexBuffer_float_or_half( FLOAT_VATTRIB_GL_TYPE( VATTRIB_SVECTOR_BIT, hfa ), mesh->sVectorsArray[0], 4, vertSize, numVerts, data + vbo->sVectorsOffset );
		}
	}

	// upload texture coordinates
	if( vbo->stOffset && ( vattribs & VATTRIB_TEXCOORDS_BIT ) ) {
		if( !mesh->stArray ) {
			errMask |= VATTRIB_TEXCOORDS_BIT;
		} else {
			R_FillVertexBuffer_float_or_half( FLOAT_VATTRIB_GL_TYPE( VATTRIB_TEXCOORDS_BIT, hfa ), mesh->stArray[0], 2, vertSize, numVerts, data + vbo->stOffset );
		}
	}

	// upload lightmap texture coordinates
	if( vbo->lmstOffset[0] && ( vattribs & VATTRIB_LMCOORDS0_BIT ) ) {
		int i;
		vattribbit_t lmattrbit;
		int type = FLOAT_VATTRIB_GL_TYPE( VATTRIB_LMCOORDS0_BIT, hfa );
		int lmstSize = ( ( type == GL_HALF_FLOAT ) ? 2 * sizeof( GLhalfARB ) : 2 * sizeof( float ) );

		lmattrbit = VATTRIB_LMCOORDS0_BIT;

		for( i = 0; i < ( MAX_LIGHTMAPS + 1 ) / 2; i++ ) {
			if( !( vattribs & lmattrbit ) ) {
				break;
			}
			if( !mesh->lmstArray[i * 2 + 0] ) {
				errMask |= lmattrbit;
				break;
			}

			R_FillVertexBuffer_float_or_half( type, mesh->lmstArray[i * 2 + 0][0], 2, vertSize, numVerts, data + vbo->lmstOffset[i] );

			if( vattribs & ( lmattrbit << 1 ) ) {
				if( !mesh->lmstArray[i * 2 + 1] ) {
					errMask |= lmattrbit << 1;
					break;
				}
				R_FillVertexBuffer_float_or_half( type, mesh->lmstArray[i * 2 + 1][0], 2, vertSize, numVerts, data + vbo->lmstOffset[i] + lmstSize );
			}

			lmattrbit <<= 2;
		}
	}

	// upload lightmap array texture layers
	if( vbo->lmlayersOffset[0] && ( vattribs & VATTRIB_LMLAYERS0123_BIT ) ) {
		int i;
		vattribbit_t lmattrbit;

		lmattrbit = VATTRIB_LMLAYERS0123_BIT;

		for( i = 0; i < ( MAX_LIGHTMAPS + 3 ) / 4; i++ ) {
			if( !( vattribs & lmattrbit ) ) {
				break;
			}
			if( !mesh->lmlayersArray[i] ) {
				errMask |= lmattrbit;
				break;
			}

			R_FillVertexBuffer( int, int, (int *)&mesh->lmlayersArray[i][0], 1, vertSize, numVerts, data + vbo->lmlayersOffset[i] );

			lmattrbit <<= 1;
		}
	}

	// upload vertex colors (although indices > 0 are never used)
	if( vbo->colorsOffset[0] && ( vattribs & VATTRIB_COLOR0_BIT ) ) {
		if( !mesh->colorsArray[0] ) {
			errMask |= VATTRIB_COLOR0_BIT;
		} else {
			R_FillVertexBuffer( int, int, (int *)&mesh->colorsArray[0][0], 1, vertSize, numVerts, data + vbo->colorsOffset[0] );
		}
	}

	// upload centre and radius for autosprites
	// this code assumes that the mesh has been properly pretransformed
	if( vbo->spritePointsOffset && ( ( vattribs & VATTRIB_AUTOSPRITE2_BIT ) == VATTRIB_AUTOSPRITE2_BIT ) ) {
		// for autosprite2 also upload vertices that form the longest axis
		// the remaining vertex can be trivially computed in vertex shader
		vec3_t vd[3];
		float d[3];
		int longest_edge = -1, longer_edge = -1, short_edge;
		float longest_dist = 0, longer_dist = 0;
		const int edges[3][2] = { { 1, 0 }, { 2, 0 }, { 2, 1 } };
		vec4_t centre[4];
		vec4_t axes[4];
		vec4_t *verts = mesh->xyzArray;
		const elem_t *elems = mesh->elems, trifanElems[6] = { 0, 1, 2, 0, 2, 3 };
		int numQuads;
		size_t bufferOffset0 = vbo->spritePointsOffset;
		size_t bufferOffset1 = vbo->sVectorsOffset;

		assert( ( mesh->elems && mesh->numElems ) || ( numVerts == 4 ) );
		if( mesh->elems && mesh->numElems ) {
			numQuads = mesh->numElems / 6;
		} else if( numVerts == 4 ) {
			// single quad as triangle fan
			numQuads = 1;
			elems = trifanElems;
		} else {
			numQuads = 0;
		}

		for( i = 0; i < numQuads; i++, elems += 6 ) {
			// find the longest edge, the long edge and the short edge
			longest_edge = longer_edge = -1;
			longest_dist = longer_dist = 0;
			for( j = 0; j < 3; j++ ) {
				float len;

				VectorSubtract( verts[elems[edges[j][0]]], verts[elems[edges[j][1]]], vd[j] );
				len = VectorLength( vd[j] );
				if( !len ) {
					len = 1;
				}
				d[j] = len;

				if( longest_edge == -1 || longest_dist < len ) {
					longer_dist = longest_dist;
					longer_edge = longest_edge;
					longest_dist = len;
					longest_edge = j;
				} else if( longer_dist < len ) {
					longer_dist = len;
					longer_edge = j;
				}
			}

			short_edge = 3 - ( longest_edge + longer_edge );
			if( short_edge > 2 ) {
				continue;
			}

			// centre
			VectorAdd( verts[elems[edges[longest_edge][0]]], verts[elems[edges[longest_edge][1]]], centre[0] );
			VectorScale( centre[0], 0.5, centre[0] );
			// radius
			centre[0][3] = d[longest_edge] * 0.5; // unused
			// right axis, normalized
			VectorScale( vd[short_edge], 1.0 / d[short_edge], vd[short_edge] );
			// up axis, normalized
			VectorScale( vd[longer_edge], 1.0 / d[longer_edge], vd[longer_edge] );

			NormToLatLong( vd[short_edge], &axes[0][0] );
			NormToLatLong( vd[longer_edge], &axes[0][2] );

			for( j = 1; j < 4; j++ ) {
				Vector4Copy( centre[0], centre[j] );
				Vector4Copy( axes[0], axes[j] );
			}

			R_FillVertexBuffer_float_or_half( FLOAT_VATTRIB_GL_TYPE( VATTRIB_AUTOSPRITE_BIT, hfa ), centre[0], 4, vertSize, 4, data + bufferOffset0 );
			R_FillVertexBuffer_float_or_half( FLOAT_VATTRIB_GL_TYPE( VATTRIB_SVECTOR_BIT, hfa ), axes[0], 4, vertSize, 4, data + bufferOffset1 );

			bufferOffset0 += 4 * vertSize;
			bufferOffset1 += 4 * vertSize;
		}
	} else if( vbo->spritePointsOffset && ( ( vattribs & VATTRIB_AUTOSPRITE_BIT ) == VATTRIB_AUTOSPRITE_BIT ) ) {
		vec4_t *verts;
		vec4_t centre[4];
		int numQuads = numVerts / 4;
		size_t bufferOffset = vbo->spritePointsOffset;

		for( i = 0, verts = mesh->xyzArray; i < numQuads; i++, verts += 4 ) {
			// centre
			for( j = 0; j < 3; j++ ) {
				centre[0][j] = ( verts[0][j] + verts[1][j] + verts[2][j] + verts[3][j] ) * 0.25;
			}
			// radius
			centre[0][3] = Distance( verts[0], centre[0] ) * 0.707106f; // 1.0f / sqrt(2)

			for( j = 1; j < 4; j++ ) {
				Vector4Copy( centre[0], centre[j] );
			}

			R_FillVertexBuffer_float_or_half( FLOAT_VATTRIB_GL_TYPE( VATTRIB_AUTOSPRITE_BIT, hfa ), centre[0], 4, vertSize, 4, data + bufferOffset );

			bufferOffset += 4 * vertSize;
		}
	}

	if( vattribs & VATTRIB_BONES_BITS ) {
		if( vbo->bonesIndicesOffset ) {
			if( !mesh->blendIndices ) {
				errMask |= VATTRIB_BONESINDICES_BIT;
			} else {
				R_FillVertexBuffer( int, int, (int *)&mesh->blendIndices[0], 1, vertSize, numVerts, data + vbo->bonesIndicesOffset );
			}
		}
		if( vbo->bonesWeightsOffset ) {
			if( !mesh->blendWeights ) {
				errMask |= VATTRIB_BONESWEIGHTS_BIT;
			} else {
				R_FillVertexBuffer( int, int, (int *)&mesh->blendWeights[0], 1, vertSize, numVerts, data + vbo->bonesWeightsOffset );
			}
		}
	}

	return errMask;
}

// void R_UploadVBOVertexRawData( mesh_vbo_t *vbo, int vertsOffset, int numVerts, const void *data )
//{
//	assert( vbo != NULL );
//	if( !vbo || !vbo->vertexId ) {
//		return;
//	}
//
//	if( vbo->tag != VBO_TAG_STREAM ) {
//		R_DeferDataSync();
//	}
//
//	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexId );
//	qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vertsOffset * vbo->vertexSize, numVerts * vbo->vertexSize, data );
// }

/*
 * R_UploadVBOVertexData
 */
vattribmask_t R_UploadVBOVertexData( mesh_vbo_t *vbo, int vertsOffset, vattribmask_t vattribs, const mesh_t *mesh )
{
	void *data;
	vattribmask_t errMask;

	assert( vbo != NULL );
	assert( mesh != NULL );
	// if( !vbo || !vbo->vertexId ) {
	// 	return 0;
	// }CmdCopyBuffer

	if( vbo->tag != VBO_TAG_STREAM ) {
		R_DeferDataSync();
	}

	data = R_VBOVertBuffer( mesh->numVerts, vbo->vertexSize );
	errMask = R_FillVBOVertexDataBuffer( vbo, vattribs, mesh, data );
	R_UploadVBOVertexRawData( vbo, vertsOffset, mesh->numVerts, data );
	return errMask;
}

/*
 * R_VBOElemBuffer
 */
static elem_t *R_VBOElemBuffer( unsigned numElems )
{
	if( numElems > r_vbo_numtempelems ) {
		if( r_vbo_numtempelems )
			R_Free( r_vbo_tempelems );
		r_vbo_numtempelems = numElems;
		r_vbo_tempelems = (elem_t *)R_Malloc( sizeof( *r_vbo_tempelems ) * numElems );
	}

	return r_vbo_tempelems;
}

/*
 * R_VBOVertBuffer
 */
static void *R_VBOVertBuffer( unsigned numVerts, size_t vertSize )
{
	size_t size = numVerts * vertSize;
	if( size > r_vbo_tempvsoupsize ) {
		if( r_vbo_tempvsoup )
			R_Free( r_vbo_tempvsoup );
		r_vbo_tempvsoupsize = size;
		r_vbo_tempvsoup = (float *)R_Malloc( size );
	}
	return r_vbo_tempvsoup;
}

/*
 *
 * Upload elements into the buffer, properly offsetting them (batching)
 */
void R_UploadVBOElemData( mesh_vbo_t *vbo, int vertsOffset, int elemsOffset, const mesh_t *mesh )
{
	assert( vbo != NULL );

	buffer_upload_desc_t uploadDesc = {
		.target = vbo->indexBuffer,
		.numBytes = mesh->numElems * sizeof( elem_t ),
		.byteOffset = elemsOffset * sizeof( elem_t ),
		.after = ( NriAccessStage ){
			.access = NriAccessBits_INDEX_BUFFER,
			.stages = NriStageBits_INDEX_INPUT
		},
	};
	//vbo->indexStage = R_ResourceTransitionBuffer( vbo->indexBuffer, (NriAccessStage){});
	R_ResourceBeginCopyBuffer( &uploadDesc );
	elem_t *dest = (elem_t *)uploadDesc.data;
	for( size_t i = 0; i < mesh->numElems; i++ ) {
		dest[i] = vertsOffset + mesh->elems[i];
	}
	R_ResourceEndCopyBuffer( &uploadDesc );
}

vattribmask_t R_UploadVBOInstancesData( mesh_vbo_t *vbo, int instOffset, int numInstances, instancePoint_t *instances )
{
	vattribmask_t errMask = 0;

	assert( vbo != NULL );

	if( !vbo->instanceBuffer) {
		return 0;
	}

	if( !instances ) {
		errMask |= VATTRIB_INSTANCES_BITS;
	}

	if( errMask ) {
		return errMask;
	}

	if( vbo->tag != VBO_TAG_STREAM ) {
		R_DeferDataSync();
	}

	if( vbo->instancesOffset ) {
		buffer_upload_desc_t uploadDesc = {
			.target = vbo->instanceBuffer,
			.numBytes = numInstances * sizeof( instancePoint_t ),
			.byteOffset = instOffset * sizeof( instancePoint_t ),
			.after = ( NriAccessStage ){
				.access = NriAccessBits_CONSTANT_BUFFER,
				.stages = NriStageBits_VERTEX_SHADER
			},
		};
		// vbo->instanceStage = R_ResourceTransitionBuffer( vbo->instanceBuffer, ( NriAccessStage ){} );
		R_ResourceBeginCopyBuffer( &uploadDesc );
		instancePoint_t *dest = (instancePoint_t *)uploadDesc.data;
		for( size_t i = 0; i < numInstances; i++ ) {
			memcpy( dest[i], instances[i], sizeof( instancePoint_t ) );
		}
		R_ResourceEndCopyBuffer( &uploadDesc );
	}
	return 0;
}

void R_FreeVBOsByTag( vbo_tag_t tag )
{
	mesh_vbo_t *vbo;
	vbohandle_t *vboh, *next, *hnode;

	if( !r_num_active_vbos ) {
		return;
	}

	hnode = &r_vbohandles_headnode;
	for( vboh = hnode->prev; vboh != hnode; vboh = next ) {
		next = vboh->prev;
		vbo = &r_mesh_vbo[vboh->index];

		if( vbo->tag == tag ) {
			R_ReleaseMeshVBO( vbo );
		}
	}

	R_DeferDataSync();
}

void R_FreeUnusedVBOs( void )
{
	mesh_vbo_t *vbo;
	vbohandle_t *vboh, *next, *hnode;

	if( !r_num_active_vbos ) {
		return;
	}

	hnode = &r_vbohandles_headnode;
	for( vboh = hnode->prev; vboh != hnode; vboh = next ) {
		next = vboh->prev;
		vbo = &r_mesh_vbo[vboh->index];

		if( vbo->registrationSequence != rsh.registrationSequence ) {
			R_ReleaseMeshVBO( vbo );
		}
	}

	R_DeferDataSync();
}

/*
 * R_ShutdownVBO
 */
void R_ShutdownVBO( void )
{
	mesh_vbo_t *vbo;
	vbohandle_t *vboh, *next, *hnode;

	if( !r_num_active_vbos ) {
		return;
	}

	hnode = &r_vbohandles_headnode;
	for( vboh = hnode->prev; vboh != hnode; vboh = next ) {
		next = vboh->prev;
		vbo = &r_mesh_vbo[vboh->index];

		R_ReleaseMeshVBO( vbo );
	}

	if( r_vbo_tempelems ) {
		R_Free( r_vbo_tempelems );
	}
	r_vbo_numtempelems = 0;
}
