#include "include/global.glsl"

layout(location = 0) out vec4 v_TexCoord;
layout(location = 1) out vec4 v_FrontColor;
layout(location = 2) out vec4 v_ProjVector;
layout(location = 3) out vec3 v_EyeVector;

#ifdef APPLY_DUDV
layout(set = DESCRIPTOR_PASS_SET, binding = 0) uniform texture2D u_DuDvMapTexture;
#endif

#ifdef APPLY_EYEDOT
layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform texture2D u_NormalmapTexture;
#endif
layout(set = DESCRIPTOR_PASS_SET, binding = 2) uniform texture2D u_ReflectionTexture;

layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform texture2D u_RefractionTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform sampler u_RefractionTextureSampler;

layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultDistortionCB pass;

void main(void)
{
	myhalf3 color;

#ifdef APPLY_DUDV
	vec3 displacement = vec3(qf_texture(u_DuDvMapTexture, vec2(v_TexCoord.pq) * vec2(0.25)));
	vec2 coord = vec2(v_TexCoord.st) + vec2(displacement) * vec2 (0.2);

	vec3 fdist = vec3 (normalize(vec3(qf_texture(u_DuDvMapTexture, coord)) - vec3 (0.5))) * vec3(0.005);
#else
	vec3 fdist = vec3(0.0);
#endif

	// get projective texcoords
	float scale = float(1.0 / float(v_ProjVector.w));
	float inv2NW = pass.textureParams.z * 0.5; // .z - inverse width
	float inv2NH = pass.textureParams.w * 0.5; // .w - inverse height
	vec2 projCoord = (vec2(v_ProjVector.xy) * scale + vec2 (1.0)) * vec2 (0.5) + vec2(fdist.xy);
	projCoord.s = float (clamp (float(projCoord.s), inv2NW, 1.0 - inv2NW));
	projCoord.t = float (clamp (float(projCoord.t), inv2NH, 1.0 - inv2NH));

	myhalf3 refr = myhalf3(0.0);
	myhalf3 refl = myhalf3(0.0);

#ifdef APPLY_EYEDOT
	// calculate dot product between the surface normal and eye vector
	// great for simulating qf_varying water translucency based on the view angle
	myhalf3 surfaceNormal = normalize(myhalf3(qf_texture(u_NormalmapTexture, coord)) - myhalf3 (0.5));
	vec3 eyeNormal = normalize(myhalf3(v_EyeVector));

	float refrdot = float(dot(surfaceNormal, eyeNormal));
	//refrdot = float (clamp (refrdot, 0.0, 1.0));
	float refldot = 1.0 - refrdot;
	// get refraction and reflection

#ifdef APPLY_REFRACTION
	refr = (myhalf3(qf_texture(u_RefractionTexture, projCoord))) * refrdot;
#endif
#ifdef APPLY_REFLECTION
	refl = (myhalf3(qf_texture(u_ReflectionTexture, projCoord))) * refldot;
#endif

#else

#ifdef APPLY_REFRACTION
	refr = (myhalf3(qf_texture(u_RefractionTexture, projCoord)));
#endif
#ifdef APPLY_REFLECTION
	refl = (myhalf3(qf_texture(u_ReflectionTexture, projCoord)));
#endif

#endif // APPLY_EYEDOT

	// add reflection and refraction
#ifdef APPLY_DISTORTION_ALPHA
	color = myhalf3(v_FrontColor.rgb) + myhalf3(mix (refr, refl, v_FrontColor.a));
#else
	color = myhalf3(v_FrontColor.rgb) + refr + refl;
#endif

#ifdef APPLY_GREYSCALE
	qf_FragColor = vec4(vec3(Greyscale(color)),1.0);
#else
	qf_FragColor = vec4(vec3(color),1.0);
#endif
}
