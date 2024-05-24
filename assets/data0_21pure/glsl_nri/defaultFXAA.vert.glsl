#include "include/attributes.glsl"
#include "include/resource.glsl"
#include "include/math_utils.glsl"

layout(set = DESCRIPTOR_OBJECT_SET, binding = 0) uniform DefaultFXAACB pass; 

layout(location = 0) out vec2 v_TexCoord;

void main(void)
{
  gl_Position = pass.mvp * a_Position;
	v_TexCoord = TextureMatrix2x3Mul(pass.textureMatrix, a_TexCoord);
}



