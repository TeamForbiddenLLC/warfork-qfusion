#ifndef R_VBO_H
#define R_VBO_H



#include "r_nri.h"
#include "r_vattribs.h"
#include "../gameshared/q_shared.h"
#include "../../qcommon/qfiles.h"
#include "r_math.h"

typedef unsigned short elem_t;
typedef vec_t instancePoint_t[8]; // quaternion for rotation + xyz pos + uniform scale

typedef struct mesh_s mesh_t;

typedef enum
{
	VBO_TAG_NONE,
	VBO_TAG_WORLD,
	VBO_TAG_MODEL,
	VBO_TAG_STREAM
} vbo_tag_t;

typedef struct mesh_vbo_s
{
	union {
		struct {
			unsigned int 		vertexId;
			unsigned int		elemId;
		} gl;
		struct {
			 uint8_t numAllocations;
			 NriMemory** memory; 
		   NriBuffer* vertexBuffer;
		   NriBuffer* indexBuffer;
			 NriBuffer* instanceBuffer;
		} nri;
	};

	unsigned int		index;
	int					registrationSequence;
	vbo_tag_t			tag;

	void 				*owner;
	unsigned int 		visframe;

	unsigned int 		numVerts;
	unsigned int 		numElems;

	size_t				vertexSize;
	size_t				arrayBufferSize;
	size_t				elemBufferSize;

	vattribmask_t		vertexAttribs;
	vattribmask_t		halfFloatAttribs;

	size_t 				normalsOffset;
	size_t 				sVectorsOffset;
	size_t 				stOffset;
	size_t 				lmstOffset[( MAX_LIGHTMAPS + 1 ) / 2];
	size_t 				lmstSize[( MAX_LIGHTMAPS + 1 ) / 2];
	size_t				lmlayersOffset[( MAX_LIGHTMAPS + 3 ) / 4];
	size_t 				colorsOffset[MAX_LIGHTMAPS];
	size_t				bonesIndicesOffset;
	size_t				bonesWeightsOffset;
	size_t				spritePointsOffset; // autosprite or autosprite2 centre + radius
	size_t				instancesOffset;
} mesh_vbo_t;

DECLARE_STUB_IMPL(mesh_vbo_t*, R_CreateMeshVBO,  void *owner, int numVerts, int numElems, int numInstances, vattribmask_t vattribs, vbo_tag_t tag, vattribmask_t halfFloatVattribs );
DECLARE_STUB_IMPL(void, R_ReleaseMeshVBO, mesh_vbo_t *vbo);
DECLARE_STUB_IMPL(void, R_UploadVBOVertexRawData, mesh_vbo_t *vbo, int vertsOffset, int numVerts, const void *data );
DECLARE_STUB_IMPL(void, R_UploadVBOElemData, mesh_vbo_t *vbo, int vertsOffset, int elemsOffset, const mesh_t *mesh );
DECLARE_STUB_IMPL(vattribmask_t, R_UploadVBOInstancesData, mesh_vbo_t *vbo, int instOffset, int numInstances, instancePoint_t *instances );


#endif
