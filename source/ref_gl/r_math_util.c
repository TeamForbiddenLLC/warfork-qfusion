#include "r_math_util.h"

void R_TransformBounds( const vec3_t origin, const mat3_t axis, const vec3_t mins, const vec3_t maxs, vec3_t bbox[8] )
{
	vec3_t tmp;
	mat3_t axis_;

	Matrix3_Transpose( axis, axis_ ); // switch row-column order

	// rotate local bounding box and compute the full bounding box
	for( size_t i = 0; i < 8; i++ ) {
		vec_t *corner = bbox[i];

		corner[0] = ( ( i & 1 ) ? mins[0] : maxs[0] );
		corner[1] = ( ( i & 2 ) ? mins[1] : maxs[1] );
		corner[2] = ( ( i & 4 ) ? mins[2] : maxs[2] );

		Matrix3_TransformVector( axis_, corner, tmp );
		VectorAdd( tmp, origin, corner );
	}
}
