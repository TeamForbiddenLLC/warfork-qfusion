#include "include/global.glsl"

layout(location = 0) out vec2 v_TexCoord;

#include "include/qf_vert_utils.glsl"

void main(void)
{
	gl_Position = obj.mvp * a_Position;
	v_TexCoord = v_TexCoord;
	

	// fix for opengl
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
