#include "include/global.glsl"

layout(set = DESCRIPTOR_OBJECT_SET, binding = 0) uniform DefaultColorCorrectCB {
  vec4 textureMatrix[2];
} pass; 

layout(location = 0) out vec2 v_TexCoord;

void main(void)
{
  gl_Position = obj.mvp * a_Position;
	v_TexCoord = TextureMatrix2x3Mul(pass.textureMatrix, a_TexCoord);
}



