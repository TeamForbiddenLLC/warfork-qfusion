#include "r_math_util.h"

// sin values are generated from this:
// [math.sin((i/ 255.0) * 2 * math.pi) for i in  range(0,256)]
// for( size_t i = 0; i < 256; i++ )
//		rsh.sinTableByte[i] = sin( (float)i / 255.0 * M_TWOPI );
static float sinTable[256] = {
#include "r_math_util_sintable.h.inl"
};

void R_NormToLatLong( const vec_t *normal, uint8_t latlong[2] )
{
	float flatlong[2];

	NormToLatLong( normal, flatlong );
	latlong[0] = (int)( flatlong[0] * 255.0 / M_TWOPI ) & 255;
	latlong[1] = (int)( flatlong[1] * 255.0 / M_TWOPI ) & 255;
}

void R_LatLongToNorm4( const uint8_t latlong[2], vec4_t out )
{
	const float cos_a = sinTable[( latlong[0] + 64 ) & 255];
	const float sin_a = sinTable[latlong[0]];
	const float cos_b = sinTable[( latlong[1] + 64 ) & 255];
	const float sin_b = sinTable[latlong[1]];

	Vector4Set( out, cos_b * sin_a, sin_b * sin_a, cos_a, 0 );
}

void R_LatLongToNorm( const uint8_t latlong[2], vec3_t out )
{
	vec4_t t;
	R_LatLongToNorm4( latlong, t );
	VectorCopy( t, out );
}

void R_TransformBounds( const vec3_t origin, const mat3_t axis, const vec3_t mins, const vec3_t maxs, vec3_t bbox[8] )
{
	vec3_t tmp;
	mat3_t axis_;

	Matrix3_Transpose( axis, axis_ ); // switch row-column order

	// rotate local bounding box and compute the full bounding box
	for(size_t i = 0; i < 8; i++ ) {
		vec_t *corner = bbox[i];

		corner[0] = ( ( i & 1 ) ? mins[0] : maxs[0] );
		corner[1] = ( ( i & 2 ) ? mins[1] : maxs[1] );
		corner[2] = ( ( i & 4 ) ? mins[2] : maxs[2] );

		Matrix3_TransformVector( axis_, corner, tmp );
		VectorAdd( tmp, origin, corner );
	}
}
