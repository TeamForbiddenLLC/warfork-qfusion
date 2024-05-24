#include "include/global.glsl"

layout(location = 0) out vec2 v_FogCoord;

layout(set = 2, binding = 0) uniform DefaultFogUBO ubo;  

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;

	QF_TransformVerts(Position, Normal, TexCoord);
	QF_FogGenCoordTexCoord(Position, v_FogCoord);

	gl_Position = ubo.mvp * Position;
}
