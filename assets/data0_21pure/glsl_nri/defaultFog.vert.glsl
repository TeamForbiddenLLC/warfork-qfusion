#include "include/global.glsl"
#include "defaultFXAA.res.glsl"

layout(location = 0) out vec2 v_FogCoord;

#include "include/qf_vert_utils.glsl"

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec3 Tangent = vec3(0,0,0);

	QF_TransformVerts(Position, Normal, Tangent, TexCoord);
	QF_FogGenCoordTexCoord(Position, v_FogCoord);

	gl_Position = obj.mvp * Position;
}
