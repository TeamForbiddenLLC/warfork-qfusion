#include "include/global.glsl" 
#include "defaultFXAA.res.glsl"

layout(location = 0) out vec2 v_TexCoord;

#include "include/qf_vert_utils.glsl"

void main(void)
{
  gl_Position = pass.mvp * a_Position;
	v_TexCoord = TextureMatrix2x3Mul(pass.textureMatrix, a_TexCoord);
}



