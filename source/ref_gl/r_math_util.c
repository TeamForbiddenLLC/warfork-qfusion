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
