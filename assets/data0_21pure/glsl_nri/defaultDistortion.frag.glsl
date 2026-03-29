#include "include/global.glsl"
#include "defaultDistortion.res.glsl"

layout(location = 0) in vec4 v_TexCoord;
layout(location = 1) in vec4 v_FrontColor;
layout(location = 2) in vec4 v_ProjVector;
layout(location = 3) in vec3 v_EyeVector;

layout(set = DESCRIPTOR_PASS_SET, binding = 0) uniform texture2D u_DuDvMapTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform sampler u_DuDvMapSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 2) uniform texture2D u_NormalmapTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform sampler u_NormalmapSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 4) uniform texture2D u_ReflectionTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 5) uniform sampler u_ReflectionSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 6) uniform texture2D u_RefractionTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 7) uniform sampler u_RefractionSampler;

layout(location = 0) out vec4 outFragColor;

void main(void)
{
	vec3 color;

#ifdef APPLY_DUDV
	vec3 displacement = vec3(texture(sampler2D(u_DuDvMapTexture,u_DuDvMapSampler), vec2(v_TexCoord.pq) * vec2(0.25)));
	vec2 coord = vec2(v_TexCoord.st) + vec2(displacement) * vec2 (0.2);

	vec3 fdist = vec3 (normalize(vec3(texture(sampler2D(u_DuDvMapTexture,u_DuDvMapSampler), coord)) - vec3 (0.5))) * vec3(0.005);
#else
	vec3 fdist = vec3(0.0);
#endif

	// get projective texcoords
	float scale = float(1.0 / float(v_ProjVector.w));
	float inv2NW = (1.0/push.textureWidth) * 0.5; // .z - inverse width
	float inv2NH = (1.0/push.textureHeight) * 0.5; // .w - inverse height
	vec2 projCoord = (vec2(v_ProjVector.xy) * scale + vec2 (1.0)) * vec2 (0.5) + vec2(fdist.xy);
	projCoord.s = float (clamp (float(projCoord.s), inv2NW, 1.0 - inv2NW));
	projCoord.t = float (clamp (float(projCoord.t), inv2NH, 1.0 - inv2NH));

	vec3 refr = vec3(0.0);
	vec3 refl = vec3(0.0);

#ifdef APPLY_EYEDOT
	// calculate dot product between the surface normal and eye vector
	// great for simulating qf_varying water translucency based on the view angle
	vec3 surfaceNormal = normalize(texture(Sampler2D(u_NormalmapTexture,u_NormalmapSampler), coord).rgb - vec3(0.5));
	vec3 eyeNormal = normalize(vec3(v_EyeVector));

	float refrdot = float(dot(surfaceNormal, eyeNormal));
	//refrdot = float (clamp (refrdot, 0.0, 1.0));
	float refldot = 1.0 - refrdot;
	// get refraction and reflection

#ifdef APPLY_REFRACTION
	refr = vec3(texture(sampler2D(u_RefractionTexture,u_RefractionSampler), projCoord)) * refrdot;
#endif
#ifdef APPLY_REFLECTION
	refl = (vec3(texture(sampler2D(u_ReflectionTexture,u_ReflectionSampler), projCoord))) * refldot;
#endif

#else

#ifdef APPLY_REFRACTION
	refr = (vec3(texture(sampler2D(u_RefractionTexture,u_RefractionSampler), projCoord)));
#endif
#ifdef APPLY_REFLECTION
	refl = (vec3(texture(sampler2D(u_ReflectionTexture,u_ReflectionSampler), projCoord)));
#endif

#endif // APPLY_EYEDOT

	// add reflection and refraction
#ifdef APPLY_DISTORTION_ALPHA
	color = vec3(v_FrontColor.rgb) + vec3(mix (refr, refl, v_FrontColor.a));
#else
	color = vec3(v_FrontColor.rgb) + refr + refl;
#endif

#ifdef APPLY_GREYSCALE
	outFragColor = vec4(vec3(Greyscale(color)),1.0);
#else
	outFragColor = vec4(vec3(color),1.0);
#endif
}
