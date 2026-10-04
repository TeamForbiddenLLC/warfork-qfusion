#include "include/global.glsl"

layout(location = 0) in vec2 v_FogCoord;
#ifdef QF_DEPTH_ONLY
vec4 outFragColor; // no colour attachment: keep it a plain global so the shader has no fragment output
#else
layout(location = 0) out vec4 outFragColor;
#endif

void main(void)
{
	float fogDensity = FogDensity(v_FogCoord);
	outFragColor = vec4(frame.fogColor, fogDensity);
}
