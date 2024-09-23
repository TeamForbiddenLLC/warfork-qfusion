#include "include/global.glsl" 

layout(location = 0) in float v_Depth;

layout(location = 0) out vec4 outFragColor; 

void main(void)
{
#ifdef APPLY_RGB_SHADOW_24BIT
	outFragColor = encodedepthmacro24(v_Depth);
#else
	outFragColor  = encodedepthmacro16(v_Depth);
#endif
}
