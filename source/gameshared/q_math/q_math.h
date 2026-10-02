#ifndef Q_MATH_H_ 
#define Q_MATH_H_

#include "../q_arch.h"


#ifdef __cplusplus
	#define Q_MATH_METHOD(METHOD_C, METHOD_CPP) METHOD_CPP 
#else
	#define Q_MATH_METHOD(METHOD_C, METHOD_CPP) METHOD_C 
#endif


//#elif (VECTORMATH_CPU_HAS_SSE1_OR_BETTER && !VECTORMATH_FORCE_SCALAR_MODE) // SSE
//    #include "sse/vectormath.hpp"
//    using namespace Vectormath::SSE;
//#elif (VECTORMATH_CPU_HAS_NEON && !VECTORMATH_FORCE_SCALAR_MODE) // NEON
//	#include "neon/vectormath.hpp"
//	using namespace Vectormath::Neon;
//#else // !SSE
//#endif // Vectormath mode selection

#include "q_vector_math.h"
#include "q_math_common.h"

#ifdef __cplusplus
#else
#endif


#endif 

