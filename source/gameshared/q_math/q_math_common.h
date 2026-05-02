

//----------------------------------------------------------------------------
// floatX
//----------------------------------------------------------------------------
// A simple structure containing 3 floating point values.
// float3 is always guaranteed to be 3 floats in size. Only use when a very
// specific size is required (like defining structures that need to be the
// same across platforms, or the same on CPU and GPU (like constant and
// structured buffers) In all other cases you should opt to use Vector3, since
// it uses SIMD optimizations whenever possible. float3 does not.
//----------------------------------------------------------------------------

#ifndef Q_MATH_COMMON_H_ 
#define Q_MATH_COMMON_H_ 

#if defined( _MSC_VER )
// Visual Studio (MS compiler)
#define Q_MATH_ALIGNED( type ) __declspec( align( 16 ) ) type
#define Q_MATH_ALIGNED_TYPE_PRE __declspec( align( 16 ) )
#define Q_MATH_ALIGNED_TYPE_POST /* nothing */
#elif defined( __GNUC__ )
// GCC or Clang
#define Q_MATH_ALIGNED( type ) type __attribute__( ( aligned( 16 ) ) )
#define Q_MATH_ALIGNED_TYPE_PRE /* nothing */
#define Q_MATH_ALIGNED_TYPE_POST __attribute__( ( aligned( 16 ) ) )
#else
// Unknown compiler
#error "Define Q_MATH_ALIGNED for your compiler or platform!"
#endif


#define q_abs(x) (( (x) > 0 ) ? (x) : -(x))
#define q_max( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define q_min( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )

struct float4 {
  union {
	  struct {
		  float x, y, z, w;
	  };
	  float v[4];
  };
};

struct float3 {
  union {
	  struct {
		  float x, y, z;
	  };
	  float v[3];
  };
};

struct float2 {
  union {
	  struct {
		  float x, y;
	  };
	  float v[2];
  };
};

#endif
