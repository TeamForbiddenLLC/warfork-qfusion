#ifndef R_MATH_UTIL_H
#define R_MATH_UTIL_H

#include "r_math.h"

void R_NormToLatLong( const vec_t *normal, uint8_t latlong[2] );
void R_LatLongToNorm( const uint8_t latlong[2], vec3_t out );
void R_LatLongToNorm4( const uint8_t latlong[2], vec4_t out );

#endif
