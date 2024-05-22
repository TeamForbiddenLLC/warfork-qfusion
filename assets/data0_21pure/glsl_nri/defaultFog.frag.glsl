#include "include/common.glsl"
#include "include/fog.glsl"

layout(location = 0) in vec2 v_FogCoord;

void main(void)
{
	float fogDensity = FogDensity(v_FogCoord);
	qf_FragColor = vec4(u_FogColor, fogDensity);
}
