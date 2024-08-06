#include "include/attributes.glsl"
#include "include/resource.glsl"
#include "include/math_utils.glsl"

layout(location = 0) out float v_Depth;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;

	QF_TransformVerts(Position, Normal, TexCoord);

	gl_Position = u_ModelViewProjectionMatrix * Position;
	v_Depth = gl_Position.z;
}
