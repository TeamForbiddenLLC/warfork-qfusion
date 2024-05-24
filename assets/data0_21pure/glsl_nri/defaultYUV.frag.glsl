#include "include/global.glsl"

qf_varying vec2 v_TexCoord;

uniform texture2D u_YUVTextureY;
uniform texture2D  u_YUVTextureU;
uniform texture2D  u_YUVTextureV;

void main(void)
{
	qf_FragColor = vec4(YUV2RGB_HDTV(vec3(
		qf_texture(u_YUVTextureY, v_TexCoord).r,
		qf_texture(u_YUVTextureU, v_TexCoord).r, 
		qf_texture(u_YUVTextureV, v_TexCoord).r
	)), 1.0);
}
