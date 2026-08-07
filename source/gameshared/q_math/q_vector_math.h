
#include <math.h>
#include <stdint.h>
#include "q_math_common.h"


#ifndef Q_VECTOR_MATH_H 
#define Q_VECTOR_MATH_H 

static const float VECTORMATH_SLERP_TOL = 0.999f;
static const double VECTORMATH_SLERP_TOL_D = 0.999;

//Q_MATH_ALIGNED_TYPE_PRE struct vec4i_s {
//	int x, y, z, w;
//} Q_MATH_ALIGNED_TYPE_POST;

Q_MATH_ALIGNED_TYPE_PRE struct vec3_s {
	float x, y, z;
} Q_MATH_ALIGNED_TYPE_POST;

Q_MATH_ALIGNED_TYPE_PRE struct vec4_s {
	float x, y, z, w;
} Q_MATH_ALIGNED_TYPE_POST;

Q_MATH_ALIGNED_TYPE_PRE struct vec2_s {
	float x, y;
} Q_MATH_ALIGNED_TYPE_POST;

Q_MATH_ALIGNED_TYPE_PRE struct mat3_s {
	struct vec3_s col0;
	struct vec3_s col1;
	struct vec3_s col2;
	struct vec3_s col3;
} Q_MATH_ALIGNED_TYPE_POST;

Q_MATH_ALIGNED_TYPE_PRE struct mat4_s {
	struct vec4_s col0;
	struct vec4_s col1;
	struct vec4_s col2;
	struct vec4_s col3;
} Q_MATH_ALIGNED_TYPE_POST;

Q_MATH_ALIGNED_TYPE_PRE struct quat_s {
	float x, y, z, w;
} Q_MATH_ALIGNED_TYPE_POST;

// quaternion
inline struct quat_s q_quat_create( float x, float y, float z, float w);

inline float q_quat_ele(uint_fast8_t index, const struct quat_s v1 );
inline float q_quat_x(const struct quat_s v1 );
inline float q_quat_y(const struct quat_s v1 );
inline float q_quat_z(const struct quat_s v1 );
inline float q_quat_w(const struct quat_s v1 );

// vec2
inline struct vec2_s q_vec2_create( float x, float y );

inline float q_vec2_ele( uint_fast8_t index, const struct vec2_s v1 );
inline float q_vec2_x( const struct vec2_s v1 );
inline float q_vec2_y( const struct vec2_s v1 );

// vec3
inline struct vec3_s q_vec3_create( float x, float y, float z );

inline float q_vec3_dot( const struct vec3_s v1, const struct vec3_s v2);
inline float q_vec3_sum(const struct vec3_s vec);
inline struct vec3_s q_vec3_slerp( float t, const struct vec3_s unitVec0, const struct vec3_s unitVec1 );
inline struct vec3_s q_vec3_add( const struct vec3_s v1, const struct vec3_s v2 );
inline struct vec3_s q_vec3_div( const struct vec3_s v1, const struct vec3_s v2 );
inline struct vec3_s q_vec3_sub( const struct vec3_s v1, const struct vec3_s v2 );
inline struct vec3_s q_vec3_mul( const struct vec3_s v1, const struct vec3_s v2 );

inline float q_vec3_ele( uint_fast8_t index, const struct vec3_s v1 );
inline float q_vec3_x( const struct vec3_s v1 );
inline float q_vec3_y( const struct vec3_s v1 );
inline float q_vec3_z( const struct vec3_s v1 );

inline struct vec3_s q_vec3_add_scalar( const struct vec3_s v1, float value );
inline struct vec3_s q_vec3_sub_scalar( const struct vec3_s v1, float value );
inline struct vec3_s q_vec3_mul_scalar( const struct vec3_s v1, float value );
inline struct vec3_s q_vec3_div_scalar( const struct vec3_s v1, float value );

inline struct vec3_s q_vec3_max_per_element( const struct vec3_s v1, const struct vec3_s v2);
inline struct vec3_s q_vec3_min_per_element( const struct vec3_s v1, const struct vec3_s v2);
inline const struct vec3_s q_vec3_copy_sign_element( const struct vec3_s v1, const struct vec3_s v2);

inline float q_vec3_max_element( struct vec3_s v1);
inline float q_vec3_min_element( struct vec3_s v1);

inline struct vec3_s q_vec3_normalize( const struct vec3_s v1);
inline float q_vec3_length( const struct vec3_s v1);
inline float q_vec3_length_sqr( const struct vec3_s v1);

inline const struct vec3_s q_vec3_cross( const struct vec3_s v1, const struct vec3_s v2);

// vec4
inline float q_vec4_ele(uint_fast8_t index, const struct vec4_s v1 );
inline float q_vec4_x(const struct vec4_s v1 );
inline float q_vec4_y(const struct vec4_s v1 );
inline float q_vec4_z(const struct vec4_s v1 );
inline float q_vec4_w(const struct vec4_s v1 );

inline struct vec3_s q_vec4_as_vec3( const struct vec4_s v );
inline struct vec2_s q_vec4_as_vec2( const struct vec4_s v );

inline struct vec4_s q_vec4_create( float x, float y, float z, float w);

inline struct vec4_s q_vec4_slerp( float t, const struct vec4_s unitVec0, const struct vec4_s unitVec1 );

inline struct vec4_s q_vec4_add_scalar( const struct vec4_s v1, float value);
inline struct vec4_s q_vec4_sub_scalar( const struct vec4_s v1, float value );
inline struct vec4_s q_vec4_mul_scalar( const struct vec4_s v1, float value );

inline struct vec4_s q_vec4_max_per_ele( const struct vec4_s v1, const struct vec4_s v2);
inline struct vec4_s q_vec4_min_per_ele( const struct vec4_s v1, const struct vec4_s v2);
inline struct vec4_s q_vec4_copy_sign_ele( const struct vec4_s v1, const struct vec4_s v2);

inline float q_vec4_max_ele( const struct vec4_s v1);
inline float q_vec4_min_ele( const struct vec4_s v1);

inline struct vec4_s q_vec4_normalize( const struct vec4_s v1);
inline float q_vec4_length( const struct vec4_s v1);
inline float q_vec4_length_sqr( const struct vec4_s v1);
inline float q_vec4_dot( const struct vec4_s v1,const struct vec4_s v2);

inline float q_vec4_sum(const struct vec4_s vec);
inline struct vec4_s q_vec4_add( const struct vec4_s v1, const struct vec4_s v2 );
inline struct vec4_s q_vec4_sub( const struct vec4_s v1, const struct vec4_s v2 );
inline struct vec4_s q_vec4_mul( const struct vec4_s v1, const struct vec4_s v2 );

// mat3
inline struct mat3_s q_mat3_create( 
	const struct vec3_s col0, 
	const struct vec3_s col1, 
	const struct vec3_s col2 );
inline struct mat3_s q_mat3_create_quat(const struct quat_s q);
inline float q_mat3_ele( const struct mat3_s v1, uint_fast8_t col, uint_fast8_t row );

// mat4

inline struct mat4_s q_mat4_create( const struct vec4_s col0, 
																		const struct vec4_s col1, 
																		const struct vec4_s col2, 
																		const struct vec4_s col3 );
inline float q_mat4_ele( const struct mat4_s v1, uint_fast8_t col, uint_fast8_t row );
inline struct mat4_s q_mat4_create_quat(const struct quat_s q);

// ---------------------------

#ifdef __cplusplus
	inline struct vec3_s q_vec_add(const struct vec3_s a, const struct vec3_s b) { return q_vec3_add(a,b); }
	inline struct vec3_s q_vec_add(const struct vec3_s a, float b) { return q_vec3_add_scalar (a,b); }
#else
	#define __Q_MATH_VEC_GENERIC_2(a,b) (void(*)(typeof(a), typeof(b)))0	
	#define q_vec_mul(a,b) _Generic(__Q_MATH_VEC_GENERIC_2(a,b), \
		void(*)(const struct vec3_s, const struct vec3_s): q_vec3_mul, \
		void(*)(const struct vec3_s, float): q_vec3_mul_scalar \
	)(a,b)
	#define q_vec_add(a,b) _Generic(__Q_MATH_VEC_GENERIC_2(a,b), \
		void(*)(const struct vec3_s, const struct vec3_s): q_vec3_add, \
		void(*)(const struct vec3_s, float): q_vec3_add_scalar \
	)(a,b)
	#define q_vec_sub(a,b) _Generic(__Q_MATH_VEC_GENERIC_2(a,b), \
		void(*)(const struct vec3_s, const struct vec3_s): q_vec3_sub, \
		void(*)(const struct vec3_s, float): q_vec3_sub_scalar \
	)(a,b)
	#define q_vec_div(a,b) _Generic(__Q_MATH_VEC_GENERIC_2(a,b), \
		void(*)(const struct vec3_s, const struct vec3_s): q_vec3_div_scalar, \
		void(*)(const struct vec3_s, float): q_vec3_div_scalar \
	)(a,b)
#endif


struct mat4_s q_mat4_create( const struct vec4_s col0, 
																			 	const struct vec4_s col1, 
																			  const struct vec4_s col2, 
																			  const struct vec4_s col3 ) {
	struct mat4_s res;
	res.col0 = col0; 
	res.col1 = col1; 
	res.col2 = col2; 
	res.col3 = col3; 
	return res;
}
struct mat4_s q_mat4_create_quat(const struct quat_s q) {

}

struct mat3_s q_mat3_create_quat(const struct quat_s q) {
    float qx, qy, qz, qw, qx2, qy2, qz2, qxqx2, qyqy2, qzqz2, qxqy2, qyqz2, qzqw2, qxqz2, qyqw2, qxqw2;
    qx = q_quat_x(q);
    qy = q_quat_y(q);
    qz = q_quat_z(q);
    qw = q_quat_w(q);
    qx2 = (qx + qx);
    qy2 = (qy + qy);
    qz2 = (qz + qz);
    qxqx2 = (qx * qx2);
    qxqy2 = (qx * qy2);
    qxqz2 = (qx * qz2);
    qxqw2 = (qw * qx2);
    qyqy2 = (qy * qy2);
    qyqz2 = (qy * qz2);
    qyqw2 = (qw * qy2);
    qzqz2 = (qz * qz2);
    qzqw2 = (qw * qz2);
    struct mat3_s res;
  	res.col0 = q_vec3_create(((1.0f - qyqy2) - qzqz2), (qxqy2 + qzqw2), (qxqz2 - qyqw2));
    res.col1 = q_vec3_create((qxqy2 - qzqw2), ((1.0f - qxqx2) - qzqz2), (qyqz2 + qxqw2));
    res.col1 = q_vec3_create((qxqz2 + qyqw2), (qyqz2 - qxqw2), ((1.0f - qxqx2) - qyqy2));
		return res;
}


float q_mat4_ele( const struct mat4_s v1, uint_fast8_t col, uint_fast8_t row ) {
	switch(col) {
		case 0:
			return q_vec4_ele(row, v1.col0);
		case 1:
			return q_vec4_ele(row, v1.col1);
		case 2:
			return q_vec4_ele(row, v1.col2);
		case 3:
			return q_vec4_ele(row, v1.col3);
	}
	return 0.0f;
}

struct mat3_s q_mat3_create( const struct vec3_s col0, 
																			 	const struct vec3_s col1, 
																			  const struct vec3_s col2 ) {
	struct mat3_s res;
	res.col0 = col0; 
	res.col1 = col1; 
	res.col2 = col2; 
	return res;
}

float q_mat3_ele( const struct mat3_s v1, uint_fast8_t col, uint_fast8_t row ) {
	switch(col) {
		case 0:
			return q_vec3_ele(row, v1.col0);
		case 1:
			return q_vec3_ele(row, v1.col1);
		case 2:
			return q_vec3_ele(row, v1.col2);
	}
	return 0.0f;
}

struct vec2_s q_vec2_create( float x, float y ) {
	struct vec2_s res;
	res.x = x;
	res.y = y;
	return res;

}
struct vec4_s q_vec4_create( float x, float y, float z, float w )
{
	struct vec4_s res;
	res.x = x;
	res.y = y;
	res.z = z;
	res.w = z;
	return res;
}

struct vec3_s q_vec3_create( float x, float y, float z )
{
	struct vec3_s res;
	res.x = x;
	res.y = y;
	res.z = z;
	return res;
}

float q_vec3_sum(const struct vec3_s vec) {
	return vec.x + vec.y + vec.z; 
}

float q_vec4_dot( const struct vec4_s v1,const struct vec4_s v2) {
	float result = ( v1.x * v2.x );
	result += ( v1.y * v2.y );
	result += ( v1.z * v2.z );
	result += ( v1.z * v2.z );
	return result;

}

float q_vec3_dot( const struct vec3_s v1, const struct vec3_s v2 )
{
	float result = ( v1.x * v2.x );
	result += ( v1.y * v2.y );
	result += ( v1.z * v2.z );
	return result;
}

float q_vec4_sum(const struct vec4_s vec) {
	return vec.x + vec.y + vec.z + vec.w;
}

struct vec4_s q_vec4_add( const struct vec4_s v1, const struct vec4_s v2 ) {
	struct vec4_s res;
	res.x = v1.x + v2.x;
	res.y = v1.y + v2.x;
	res.z = v1.z + v2.y;
	res.w = v1.w + v2.z;
	return res;

}

struct vec4_s q_vec4_sub( const struct vec4_s v1, const struct vec4_s v2 ) {
	struct vec4_s res;
	res.x = v1.x - v2.x;
	res.y = v1.y - v2.x;
	res.z = v1.z - v2.y;
	res.w = v1.w - v2.z;
	return res;
}

struct vec4_s q_vec4_mul( const struct vec4_s v1, const struct vec4_s v2 ) {
	struct vec4_s res;
	res.x = v1.x * v2.x;
	res.y = v1.y * v2.x;
	res.z = v1.z * v2.y;
	res.w = v1.w * v2.z;
	return res;
}


struct vec3_s q_vec3_add( const struct vec3_s v1, const struct vec3_s v2 )
{
	struct vec3_s res;
	res.x = v1.x + v2.x;
	res.y = v1.y + v2.y;
	res.z = v1.z + v2.z;
	return res;
}

struct vec3_s q_vec3_div( const struct vec3_s v1, const struct vec3_s v2 )
{
	struct vec3_s res;
	res.x = v1.x / v2.x;
	res.y = v1.y / v2.y;
	res.z = v1.z / v2.z;
	return res;
}

struct vec3_s q_vec3_sub( const struct vec3_s v1, const struct vec3_s v2 )
{
	struct vec3_s res;
	res.x = v1.x - v2.x;
	res.y = v1.y - v2.y;
	res.z = v1.z - v2.z;
	return res;
}

struct vec3_s q_vec3_mul( const struct vec3_s v1, const struct vec3_s v2 )
{
	struct vec3_s res;
	res.x = v1.x * v2.x;
	res.y = v1.y * v2.y;
	res.z = v1.z * v2.z;
	return res;
}

float q_vec3_ele( uint_fast8_t index, const struct vec3_s v1 )
{
	switch( index ) {
		case 0:
			return v1.x;
		case 1:
			return v1.y;
		case 2:
			return v1.z;
	}
	assert( 0 ); // this is bad
	return 0.0f;
}

struct vec3_s q_vec3_slerp( float t, const struct vec3_s unitVec0, const struct vec3_s unitVec1 ) {
	float recipSinAngle, scale0, scale1, cosAngle, angle;
	cosAngle = q_vec3_dot(unitVec0, unitVec1);
	if (cosAngle < VECTORMATH_SLERP_TOL)
	{
		angle = acosf(cosAngle);
		recipSinAngle = (1.0f / sinf(angle));
		scale0 = (sinf(((1.0f - t) * angle)) * recipSinAngle);
		scale1 = (sinf((t * angle)) * recipSinAngle);
	}
	else
	{
		scale0 = (1.0f - t);
		scale1 = t;
	}

	return q_vec3_add(q_vec3_mul_scalar(unitVec0, scale0), q_vec3_mul_scalar(unitVec1, scale1));
}

struct vec4_s q_vec4_slerp( float t, const struct vec4_s unitVec0, const struct vec4_s unitVec1 ) {
	float recipSinAngle, scale0, scale1, cosAngle, angle;
	cosAngle = q_vec4_dot(unitVec0, unitVec1);
	if (cosAngle < VECTORMATH_SLERP_TOL)
	{
		angle = acosf(cosAngle);
		recipSinAngle = (1.0f / sinf(angle));
		scale0 = (sinf(((1.0f - t) * angle)) * recipSinAngle);
		scale1 = (sinf((t * angle)) * recipSinAngle);
	}
	else
	{
		scale0 = (1.0f - t);
		scale1 = t;
	}
	return q_vec4_add(q_vec4_mul_scalar(unitVec0, scale0),q_vec4_mul_scalar(unitVec1,scale1));

}


struct vec3_s q_vec3_div_scalar( const struct vec3_s v1, float value )
{
	struct vec3_s res;
	res.x = v1.x / value;
	res.y = v1.y / value;
	res.z = v1.z / value;
	return res;
}

struct vec4_s q_vec4_add_scalar( const struct vec4_s v1, float value) {
	struct vec4_s res;
	res.x = v1.x + value;
	res.y = v1.y + value;
	res.z = v1.z + value;
	res.z = v1.z + value;
	return res;
}

struct vec4_s q_vec4_sub_scalar( const struct vec4_s v1, float value ) {
	struct vec4_s res;
	res.x = v1.x - value;
	res.y = v1.y - value;
	res.z = v1.z - value;
	res.z = v1.z - value;
	return res;

}
struct vec4_s q_vec4_mul_scalar( const struct vec4_s v1, float value ) {
	struct vec4_s res;
	res.x = v1.x * value;
	res.y = v1.y * value;
	res.z = v1.z * value;
	res.z = v1.z * value;
	return res;
}

struct vec3_s q_vec3_add_scalar( const struct vec3_s v1, float value )
{
	struct vec3_s res;
	res.x = v1.x + value;
	res.y = v1.y + value;
	res.z = v1.z + value;
	return res;
}

struct vec3_s q_vec3_sub_scalar( const struct vec3_s v1, float value )
{
	struct vec3_s res;
	res.x = v1.x - value;
	res.y = v1.y - value;
	res.z = v1.z - value;
	return res;
}

struct vec3_s q_vec3_mul_scalar( const struct vec3_s v1, float value )
{
	struct vec3_s res;
	res.x = v1.x * value;
	res.y = v1.y * value;
	res.z = v1.z * value;
	return res;
}

struct vec4_s q_vec4_max_per_ele( const struct vec4_s v1, const struct vec4_s v2) {
	struct vec4_s res;
	res.x = q_max(v1.x, v2.x);
	res.y = q_max(v1.y, v2.y);
	res.z = q_max(v1.z, v2.z);
	res.w = q_max(v1.w, v2.w);
	return res;
} 

struct vec4_s q_vec4_min_per_ele( const struct vec4_s v1, const struct vec4_s v2) {
	struct vec4_s res;
	res.x = q_min(v1.x, v2.x);
	res.y = q_min(v1.y, v2.y);
	res.z = q_min(v1.z, v2.z);
	res.w = q_min(v1.w, v2.w);
	return res;
}

struct vec4_s q_vec4_copy_sign_ele( const struct vec4_s v1, const struct vec4_s v2) {
	struct vec4_s res;
	res.x = ( v1.x < 0 ) ? -fabsf( v2.x ) : fabsf( v2.x );
	res.y = ( v1.y < 0 ) ? -fabsf( v2.y ) : fabsf( v2.y );
	res.z = ( v1.z < 0 ) ? -fabsf( v2.z ) : fabsf( v2.z );
	res.w = ( v1.w < 0 ) ? -fabsf( v2.w ) : fabsf( v2.w );
	return res;
}

struct vec3_s q_vec3_max_per_element( const struct vec3_s v1, const struct vec3_s v2) {
	struct vec3_s res;
	res.x = q_max(v1.x, v2.x);
	res.y = q_max(v1.y, v2.y);
	res.z = q_max(v1.z, v2.z);
	return res;
}
struct vec3_s q_vec3_min_per_element( const struct vec3_s v1, const struct vec3_s v2) {
	struct vec3_s res;
	res.x = q_min(v1.x, v2.x);
	res.y = q_min(v1.y, v2.y);
	res.z = q_min(v1.z, v2.z);
	return res;
}
inline const struct vec3_s q_vec3_copy_sign_element( const struct vec3_s v1, const struct vec3_s v2) {
	struct vec3_s res;
	res.x = ( v2.x < 0 ) ? -q_abs(v1.x) : q_abs(v1.x);
	res.y = ( v2.y < 0 ) ? -q_abs(v1.y) : q_abs(v1.y);
	res.z = ( v2.z < 0 ) ? -q_abs(v1.z) : q_abs(v1.z);
	return res;
}

float q_vec4_ele(uint_fast8_t index, const struct vec4_s v1 ) {
	switch(index) {
		case 0:
			return v1.x;
		case 1:
			return v1.y;
		case 2:
			return v1.z;
		case 3:
			return v1.w;
	}
	assert(0);
	return 0;
}


float q_vec4_x( const struct vec4_s v1 )
{
	return v1.x;
}

float q_vec4_y( const struct vec4_s v1 )
{
	return v1.y;
}

float q_vec4_z( const struct vec4_s v1 )
{
	return v1.z;
}

float q_vec4_w( const struct vec4_s v1 )
{
	return v1.w;
}

float q_vec3_x( const struct vec3_s v1 )
{
	return v1.x;
}

float q_vec3_y( const struct vec3_s v1 )
{
	return v1.y;
}

float q_vec3_z( const struct vec3_s v1 )
{
	return v1.z;
}

float q_vec2_ele( uint_fast8_t index, const struct vec2_s v1 ) {
	switch( index ) {
		case 0:
			return v1.x;
		case 1:
			return v1.y;
	}
	assert( 0 );
	return 0;
}

float q_vec2_x( const struct vec2_s v1 ) {
	return v1.x;
}

float q_vec2_y( const struct vec2_s v1 ) {
	return v1.y;
}

float q_quat_ele(uint_fast8_t index, const struct quat_s v1 ) {
	switch(index) {
		case 0:
			return v1.x;
		case 1:
			return v1.y;
		case 2:
			return v1.z;
		case 3:
			return v1.w;
	}
	assert(0);
	return 0;
}
float q_quat_x(const struct quat_s v1 ) {
	return v1.x;
}
float q_quat_y(const struct quat_s v1 ) {
	return v1.y;
}
float q_quat_z(const struct quat_s v1 ) {
	return v1.z;
}
float q_quat_w(const struct quat_s v1 ) {
	return v1.w;
}

struct vec3_s q_vec4_as_vec3( const struct vec4_s v ) {
	return q_vec3_create(v.x, v.y, v.z);
}

struct vec2_s q_vec4_as_vec2( const struct vec4_s v ) {
	return q_vec2_create(v.x, v.y);
}

float q_vec4_max_ele( const struct vec4_s v1) {
	const float x = q_vec4_x(v1);
	const float y = q_vec4_y(v1);
	const float z = q_vec4_z(v1);
	const float w = q_vec4_z(v1);
	return q_max(q_max(x, y), q_max(z,w));
}

float q_vec4_min_ele( const struct vec4_s v1) {
	const float x = q_vec4_x(v1);
	const float y = q_vec4_y(v1);
	const float z = q_vec4_z(v1);
	const float w = q_vec4_z(v1);
	return q_min(q_min(x, y), q_min(z,w));
}

float q_vec3_max_element( struct vec3_s v1) {
	const float x = q_vec3_x(v1);
	const float y = q_vec3_y(v1);
	const float z = q_vec3_z(v1);
	return q_max(q_max(x, y), z);
}

float q_vec3_min_element( struct vec3_s v1) {
	const float x = q_vec3_x(v1);
	const float y = q_vec3_y(v1);
	const float z = q_vec3_z(v1);
	return q_min(q_min(x, y), z);
}

struct vec4_s q_vec4_normalize( const struct vec4_s v1) {
	const float lenSqr = q_vec4_length_sqr(v1);
	const float lenInv = (1.0 / sqrt(lenSqr));
	return q_vec4_create((q_vec4_x(v1) * lenInv),
				   (q_vec4_y(v1) * lenInv),
				   (q_vec4_z(v1) * lenInv),
					 (q_vec4_w(v1) * lenInv));

}

float q_vec4_length( const struct vec4_s v1) {
    return sqrt(q_vec4_x(v1) * q_vec4_x(v1) + 
  				 q_vec4_y(v1) * q_vec4_y(v1) + 
  				 q_vec4_z(v1) * q_vec4_z(v1) +
  				 q_vec4_w(v1) * q_vec4_w(v1));

}

float q_vec4_length_sqr( const struct vec4_s v1) {
    return q_vec4_x(v1) * q_vec4_x(v1) + 
  				 q_vec4_y(v1) * q_vec4_y(v1) + 
  				 q_vec4_z(v1) * q_vec4_z(v1) +
  				 q_vec4_w(v1) * q_vec4_w(v1);

}

struct vec3_s q_vec3_normalize( const struct vec3_s v1 )
{
	const float lenSqr = q_vec3_length_sqr(v1);
	const float lenInv = (1.0 / sqrt(lenSqr));
	return q_vec3_create((q_vec3_x(v1) * lenInv),
				   (q_vec3_y(v1) * lenInv),
				   (q_vec3_z(v1) * lenInv));
}

float q_vec3_length( const struct vec3_s v1 )
{
    return sqrtf(
    	q_vec3_x(v1) * q_vec3_x(v1) + 
    	q_vec3_y(v1) * q_vec3_y(v1) + 
    	q_vec3_z(v1) * q_vec3_z(v1));
}

float q_vec3_length_sqr( const struct vec3_s v1 )
{
    return q_vec3_x(v1) * q_vec3_x(v1) + 
  				 q_vec3_y(v1) * q_vec3_y(v1) + 
  				 q_vec3_z(v1) * q_vec3_z(v1);
}

inline const struct vec3_s q_vec3_cross( const struct vec3_s v1, const struct vec3_s v2){ 
	return q_vec3_create(((q_vec3_y(v1) * q_vec3_z(v2)) - (q_vec3_z(v1) * q_vec3_y(v2))),
											 ((q_vec3_z(v1) * q_vec3_x(v2)) - (q_vec3_x(v1) * q_vec3_z(v2))),
				               ((q_vec3_x(v1) * q_vec3_y(v2)) - (q_vec3_y(v1) * q_vec3_x(v2))));

}

struct quat_s q_quat_create( float x, float y, float z, float w )
{
	struct quat_s quat;
	quat.x = x;
	quat.y = y;
	quat.z = z;
	quat.w = w;
	return quat;
}

#endif
