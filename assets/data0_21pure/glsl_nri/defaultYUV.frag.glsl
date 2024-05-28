#include "include/global.glsl"

qf_varying vec2 v_TexCoord;

layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform sampler u_YUVSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 4) uniform texture2D  u_YUVTextureY;
layout(set = DESCRIPTOR_PASS_SET, binding = 5) uniform texture2D  u_YUVTextureU;
layout(set = DESCRIPTOR_PASS_SET, binding = 6) uniform texture2D  u_YUVTextureV;

layout(location = 0) out vec4 outFragColor;

void main(void)
{
	outFragColor = vec4(YUV2RGB_HDTV(vec3(
		texture(Sampler2D(u_YUVTextureY,u_YUVSampler), v_TexCoord).r,
		texture(Sampler2D(u_YUVTextureU,u_YUVSampler), v_TexCoord).r, 
		texture(Sampler2D(u_YUVTextureV,u_YUVSampler), v_TexCoord).r
	)), 1.0);
}
