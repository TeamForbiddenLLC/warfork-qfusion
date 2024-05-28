#include "include/attributes.glsl"
#include "include/resource.glsl"
#include "include/math_utils.glsl"

layout(location = 0) out vec2 v_TexCoord;

void main(void)
{
	gl_Position = obj.mvp * a_Position;
	v_TexCoord = v_TexCoord;
}
