#ifndef R_MATH_UTIL_H
#define R_MATH_UTIL_H

#include "r_math.h"

void R_TransformBounds( const vec3_t origin, const mat3_t axis, const vec3_t mins, const vec3_t maxs, vec3_t bbox[8] );

#endif

