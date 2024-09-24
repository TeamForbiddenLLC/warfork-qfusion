#include "include/global.glsl"
#include "defaultDistortion.res.glsl"

layout(set = DESCRIPTOR_PASS_SET, binding = 0) uniform sampler u_BaseSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 0) uniform texture2D u_BaseTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform texture2D u_ColorLUT;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outFragColor;

#include "include/qf_vert_utils.glsl"

void main(void)
{
	vec3 coords = texture(sampler2D(u_BaseTexture, u_BaseSampler), v_TexCoord).rgb;
	coords.rg = coords.rg * vec2(0.9375) + vec2(0.03125);
	coords *= vec3(1.0, 0.0625, 15.0);
	float blueMix = fract(coords.b);
	vec2 blueOffset = vec2(0.0, floor(coords.b) * 0.0625);
	vec3 color1 = texture(sampler2D(u_ColorLUT, u_BaseSampler), coords.rg + blueOffset).rgb;
	blueOffset.y = min(blueOffset.y + 0.0625, 0.9375);
	vec3 color2 = texture(sampler2D(u_ColorLUT, u_BaseSampler), coords.rg + blueOffset).rgb;
	outFragColor = vec4(mix(color1, color2, blueMix), 1.0);
}
