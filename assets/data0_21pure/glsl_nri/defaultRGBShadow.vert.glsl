#include "include/global.glsl" 
#include "include/qf_vert_utils.glsl"

layout(location = 0) out float v_Depth;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec3 Tangent = vec3(0,0,0);
	QF_TransformVerts(Position, Normal, Tangent, TexCoord);

	gl_Position = obj.mvp * Position;
	v_Depth = gl_Position.z;


	// fix for opengl
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
