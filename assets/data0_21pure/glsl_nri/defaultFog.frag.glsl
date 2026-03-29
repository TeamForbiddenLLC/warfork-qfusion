#include "include/global.glsl"

layout(location = 0) in vec2 v_FogCoord;
layout(location = 0) out vec4 outFragColor;

void main(void)
{
	float fogDensity = FogDensity(v_FogCoord);
	outFragColor = vec4(frame.fogColor, fogDensity);
}
