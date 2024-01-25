#ifndef R_VERTEX_UTIL_H
#define R_VERTEX_UTIL_H

#include "r_math.h"

void R_BuildTangentVectors( int numVertexes, vec4_t *xyzArray, vec4_t *normalsArray, vec2_t *stArray, int numTris, uint16_t *elems, vec4_t *sVectorsArray );
void R_CopyOffsetElements( const uint16_t *inelems, int numElems, int vertsOffset, uint16_t *outelems );
void R_CopyOffsetTriangles( const uint16_t *inelems, int numElems, int vertsOffset, uint16_t *outelems );
void R_BuildTrifanElements( int vertsOffset, int numVerts, uint16_t *elems );

#endif

