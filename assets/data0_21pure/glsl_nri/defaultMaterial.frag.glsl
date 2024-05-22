#include "include/common.glsl"
#include "include/resource.glsl"

layout(set = 0, binding = 0) uniform sampler lightmapTextureSample;
layout(set = 0, binding = 1) uniform texture2D lightmapTexture0[16];
layout(set = 0, binding = 2) uniform texture2D lightmapTexture1[16];
layout(set = 0, binding = 3) uniform texture2D lightmapTexture2[16];
layout(set = 0, binding = 4) uniform texture2D lightmapTexture3[16];

layout(set = 1, binding = 6) uniform sampler2D u_BaseTexture;
layout(set = 1, binding = 7) uniform sampler2D u_NormalmapTexture;
layout(set = 1, binding = 8) uniform sampler2D u_GlossTexture;
#ifdef APPLY_DECAL
	layout(set = 1, binding = 9) uniform sampler2D u_DecalTexture;
#endif

#ifdef APPLY_ENTITY_DECAL
	layout(set = 1, binding = 10) uniform sampler2D u_EntityDecalTexture;
#endif

#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
	layout(set = 1, binding = 11) uniform float u_OffsetMappingScale;
#endif

layout(location = 0) in vec3 v_Position 
layout(location = 1) in vec4 v_EyeVector 
#ifdef NUM_LIGHTMAPS
	layout(location = 2) in qf_lmvec01 v_LightmapTexCoord01;
	#if NUM_LIGHTMAPS > 2 
		layout(location = 3) in qf_lmvec23 v_LightmapTexCoord23;
	#endif 
  # ifdef LIGHTMAP_ARRAYS
  	layout(location = 4) in vec4 a_LightmapLayer0123;
  # endif // LIGHTMAP_ARRAYS
#endif
layout(location = 5) in vec4 v_LightmapLayer0123;
layout(location = 6) in mat3 v_StrMatrix; // directions of S/T/R texcoords (tangent, binormal, normal)



#include "include/lightmap.glsl"
#include "include/varying_material.glsl"

#include "include/dlights.glsl"
#include "include/fog.glsl"
#include "include/greyscale.glsl"

#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
// The following reliefmapping and offsetmapping routine was taken from DarkPlaces
// The credit goes to LadyHavoc (as always)
vec2 OffsetMapping(sampler2D NormalmapTexture, in vec2 TexCoord, in vec3 EyeVector, in float OffsetMappingScale)
{
#ifdef APPLY_RELIEFMAPPING
	// 14 sample relief mapping: linear search and then binary search
	// this basically steps forward a small amount repeatedly until it finds
	// itself inside solid, then jitters forward and back using decreasing
	// amounts to find the impact
	//vec3 OffsetVector = vec3(EyeVector.xy * ((1.0 / EyeVector.z) * OffsetMappingScale) * vec2(-1, 1), -1);
	//vec3 OffsetVector = vec3(normalize(EyeVector.xy) * OffsetMappingScale * vec2(-1, 1), -1);
	vec3 OffsetVector = vec3(normalize(EyeVector).xy * OffsetMappingScale * vec2(-1, 1), -1);
	vec3 RT = vec3(TexCoord, 1);
	OffsetVector *= 0.1;
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(qf_texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector * (step(qf_texture(NormalmapTexture, RT.xy).a, RT.z)          - 0.5);
	RT += OffsetVector * (step(qf_texture(NormalmapTexture, RT.xy).a, RT.z) * 0.5    - 0.25);
	RT += OffsetVector * (step(qf_texture(NormalmapTexture, RT.xy).a, RT.z) * 0.25   - 0.125);
	RT += OffsetVector * (step(qf_texture(NormalmapTexture, RT.xy).a, RT.z) * 0.125  - 0.0625);
	RT += OffsetVector * (step(qf_texture(NormalmapTexture, RT.xy).a, RT.z) * 0.0625 - 0.03125);
	return RT.xy;
#else
	// 2 sample offset mapping (only 2 samples because of ATI Radeon 9500-9800/X300 limits)
	// this basically moves forward the full distance, and then backs up based
	// on height of samples
	//vec2 OffsetVector = vec2(EyeVector.xy * ((1.0 / EyeVector.z) * OffsetMappingScale) * vec2(-1, 1));
	//vec2 OffsetVector = vec2(normalize(EyeVector.xy) * OffsetMappingScale * vec2(-1, 1));
	vec2 OffsetVector = vec2(normalize(EyeVector).xy * OffsetMappingScale * vec2(-1, 1));
	vec2 TexCoord_ = TexCoord + OffsetVector;
	OffsetVector *= 0.5;
	TexCoord_ -= OffsetVector * qf_texture(NormalmapTexture, TexCoord_).a;
	TexCoord_ -= OffsetVector * qf_texture(NormalmapTexture, TexCoord_).a;
	return TexCoord_;
#endif // APPLY_RELIEFMAPPING
}
#endif // defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)



myhalf3 CelShading(in myhalf3 surfaceNormalModelspace, in myhalf3 diffuseNormalModelspace)
{ 
	myhalf diffuseProductPositive;
	myhalf diffuseProductNegative;
	myhalf diffuseProduct;
	myhalf hardShadow = 0.0;

#ifdef APPLY_HALFLAMBERT
	diffuseProduct = myhalf (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	diffuseProductPositive = myhalf ( clamp(diffuseProduct, 0.0, 1.0) * 0.5 + 0.5 );
	diffuseProductPositive *= diffuseProductPositive;
	diffuseProductNegative = myhalf ( clamp(diffuseProduct, -1.0, 0.0) * 0.5 - 0.5 );
	diffuseProductNegative = diffuseProductNegative * diffuseProductNegative - 0.25;
	diffuseProduct = diffuseProductPositive;
#else
	diffuseProduct = myhalf (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	diffuseProductPositive = max (diffuseProduct, 0.0);
	diffuseProductNegative = (-min (diffuseProduct, 0.0) - 0.3);
#endif // APPLY_HALFLAMBERT

	// smooth the hard shadow edge
	hardShadow += floor(max(diffuseProduct + 0.1, 0.0) * 2.0);
	hardShadow += floor(max(diffuseProduct + 0.055, 0.0) * 2.0);
	hardShadow += floor(diffuseProductPositive * 2.0);

	diffuseProduct = myhalf(0.6 + hardShadow * 0.09 + diffuseProductPositive * 0.14);

	// backlight
	diffuseProduct += myhalf (ceil(diffuseProductNegative * 2.0) * 0.085 + diffuseProductNegative * 0.085);

	return myhalf3(diffuseProduct);
}

vec3 DirectionalLightColor(in myhalf3 surfaceNormalModelspace, out myhalf3 weightedDiffuseNormalModelspace) {
	vec3 diffuseNormalModelspace;
	float diffuseProduct;
	vec3 color = vec3(0.0);

#ifdef APPLY_DIRECTIONAL_LIGHT_FROM_NORMAL
	diffuseNormalModelspace = v_StrMatrix[2];
#else
	diffuseNormalModelspace = u_LightDir;
#endif // APPLY_DIRECTIONAL_LIGHT_FROM_NORMAL

	weightedDiffuseNormalModelspace = diffuseNormalModelspace;

#ifdef APPLY_CELSHADING
	color.rgb += CelShading(surfaceNormalModelspace, diffuseNormalModelspace);
#else

#ifdef APPLY_HALFLAMBERT
	diffuseProduct = float ( clamp(dot (surfaceNormalModelspace, diffuseNormalModelspace), 0.0, 1.0) * 0.5 + 0.5 );
	diffuseProduct *= diffuseProduct;
#else
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
#endif // APPLY_HALFLAMBERT

#ifdef APPLY_DIRECTIONAL_LIGHT_MIX
	color.rgb += qf_FrontColor.rgb;
#else
	color.rgb += u_LightDiffuse.rgb * myhalf(max (diffuseProduct, 0.0)) + u_LightAmbient;
#endif

#endif // APPLY_CELSHADING

	return color;
}


myhalf3 LightmapColor(in myhalf3 surfaceNormalModelspace, out myhalf3 weightedDiffuseNormalModelspace)
{
	myhalf3 diffuseNormalModelspace;
	myhalf diffuseProduct;
	myhalf3 color = myhalf3(0.0);
	
	// get light normal
	diffuseNormalModelspace = normalize(myhalf3 (Lightmap(u_LightmapTexture0, v_LightmapTexCoord01.st+vec2(u_DeluxemapOffset[0].x, 0.0), v_LightmapLayer0123.x)) - myhalf3 (0.5));
	// calculate directional shading
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));

#ifdef APPLY_FBLIGHTMAP
	weightedDiffuseNormalModelspace = diffuseNormalModelspace;
	// apply lightmap color
	color.rgb += myhalf3 (max (diffuseProduct, 0.0) * myhalf3 (Lightmap(u_LightmapTexture0, v_LightmapTexCoord01.st, v_LightmapLayer0123.x)));
#else
#define NORMALIZE_DIFFUSE_NORMAL
	weightedDiffuseNormalModelspace = u_LightstyleColor[0] * diffuseNormalModelspace;
	// apply lightmap color
	color.rgb += u_LightstyleColor[0] * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (Lightmap(u_LightmapTexture0, v_LightmapTexCoord01.st, v_LightmapLayer0123.x));
#endif // APPLY_FBLIGHTMAP

#ifdef APPLY_AMBIENT_COMPENSATION
	// compensate for ambient lighting
	color.rgb += myhalf((1.0 - max (diffuseProduct, 0.0))) * u_LightAmbient;
#endif

#if NUM_LIGHTMAPS >= 2
	diffuseNormalModelspace = normalize(myhalf3 (Lightmap(u_LightmapTexture1, v_LightmapTexCoord01.pq+vec2(u_DeluxemapOffset[0].y,0.0), v_LightmapLayer0123.y)) - myhalf3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += u_LightstyleColor[1] * diffuseNormalModelspace;
	color.rgb += u_LightstyleColor[1] * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (Lightmap(u_LightmapTexture1, v_LightmapTexCoord01.pq, v_LightmapLayer0123.y));
#endif 
#if NUM_LIGHTMAPS >= 3
	diffuseNormalModelspace = normalize(myhalf3 (Lightmap(u_LightmapTexture2, v_LightmapTexCoord23.st+vec2(u_DeluxemapOffset[0].z,0.0), v_LightmapLayer0123.z)) - myhalf3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += u_LightstyleColor[2] * diffuseNormalModelspace;
	color.rgb += u_LightstyleColor[2] * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (Lightmap(u_LightmapTexture2, v_LightmapTexCoord23.st, v_LightmapLayer0123.z));
#endif 
#if NUM_LIGHTMAPS >= 4
	diffuseNormalModelspace = normalize(myhalf3 (Lightmap(u_LightmapTexture3, v_LightmapTexCoord23.pq+vec2(u_DeluxemapOffset[0].w,0.0), v_LightmapLayer0123.w)) - myhalf3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += u_LightstyleColor[3] * diffuseNormalModelspace;
	color.rgb += u_LightstyleColor[3] * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (Lightmap(u_LightmapTexture3, v_LightmapTexCoord23.pq, v_LightmapLayer0123.w));
#endif 

	return color;
}

void main()
{
#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
	// apply offsetmapping
	vec2 TexCoordOffset = OffsetMapping(u_NormalmapTexture, v_TexCoord_FogCoord.st, v_EyeVector, u_OffsetMappingScale);
#define v_TexCoord TexCoordOffset
#else
#define v_TexCoord v_TexCoord_FogCoord.st
#endif

	myhalf3 surfaceNormal;
	myhalf3 surfaceNormalModelspace;
	myhalf3 weightedDiffuseNormalModelspace;

#if !defined(APPLY_DIRECTIONAL_LIGHT) && !defined(NUM_LIGHTMAPS)
	myhalf4 color = myhalf4 (1.0, 1.0, 1.0, 1.0);
#else
	myhalf4 color = myhalf4 (0.0, 0.0, 0.0, 1.0);
#endif

	myhalf4 decal = myhalf4 (0.0, 0.0, 0.0, 1.0);

	// get the surface normal
	surfaceNormal = normalize(myhalf3(qf_texture (u_NormalmapTexture, v_TexCoord)) - myhalf3 (0.5));
	surfaceNormalModelspace = normalize(v_StrMatrix * surfaceNormal);

#ifdef APPLY_DIRECTIONAL_LIGHT
	color.rgb += DirectionalLightColor(surfaceNormalModelspace, weightedDiffuseNormalModelspace);
#endif

#ifdef NUM_LIGHTMAPS
	color.rgb += LightmapColor(surfaceNormalModelspace, weightedDiffuseNormalModelspace);
#endif

#if defined(NUM_DLIGHTS)
	color.rgb += DynamicLightsSurfaceColor(v_Position, surfaceNormalModelspace);
#endif

#ifdef APPLY_SPECULAR

#ifdef NORMALIZE_DIFFUSE_NORMAL
	myhalf3 specularNormal = normalize (myhalf3 (normalize (weightedDiffuseNormalModelspace)) + myhalf3 (normalize (u_EntityDist - v_Position)));
#else
	myhalf3 specularNormal = normalize (weightedDiffuseNormalModelspace + myhalf3 (normalize (u_EntityDist - v_Position)));
#endif

	myhalf specularProduct = myhalf(dot (surfaceNormalModelspace, specularNormal));
	color.rgb += (myhalf3(qf_texture(u_GlossTexture, v_TexCoord)) * constants.glossFactors.x) * pow(myhalf(max(specularProduct, 0.0)), constants.glossFactors.y);
#endif // APPLY_SPECULAR

#if defined(APPLY_BASETEX_ALPHA_ONLY) && !defined(APPLY_DRAWFLAT)
	color = min(color, myhalf4(qf_texture(u_BaseTexture, v_TexCoord).a));
#else
	myhalf4 diffuse;

#ifdef APPLY_DRAWFLAT
	myhalf n = myhalf(step(DRAWFLAT_NORMAL_STEP, abs(v_StrMatrix[2].z)));
	diffuse = myhalf4(mix(constants.wallColor, constants.floorColor, n), myhalf(qf_texture(u_BaseTexture, v_TexCoord).a));
#else
	diffuse = myhalf4(qf_texture(u_BaseTexture, v_TexCoord));
#endif

#ifdef APPLY_ENTITY_DECAL

#ifdef APPLY_ENTITY_DECAL_ADD
	decal.rgb = myhalf3(qf_texture(u_EntityDecalTexture, v_TexCoord));
	diffuse.rgb += u_EntityColor.rgb * decal.rgb;
#else
	decal = myhalf4(u_EntityColor.rgb, 1.0) * myhalf4(qf_texture(u_EntityDecalTexture, v_TexCoord));
	diffuse.rgb = mix(diffuse.rgb, decal.rgb, decal.a);
#endif // APPLY_ENTITY_DECAL_ADD

#endif // APPLY_ENTITY_DECAL

	color = color * diffuse;
#endif // defined(APPLY_BASETEX_ALPHA_ONLY) && !defined(APPLY_DRAWFLAT)

#ifdef APPLY_DECAL

#ifdef APPLY_DECAL_ADD
	decal.rgb = myhalf3(qf_FrontColor.rgb) * myhalf3(qf_texture(u_DecalTexture, v_TexCoord));
	color.rgb += decal.rgb;
#else
	decal = myhalf4(qf_FrontColor.rgb, 1.0) * myhalf4(qf_texture(u_DecalTexture, v_TexCoord));
	color.rgb = mix(color.rgb, decal.rgb, decal.a);
#endif // APPLY_DECAL_ADD
	color.a *= myhalf(qf_FrontColor.a);

#else

#if !defined (APPLY_DIRECTIONAL_LIGHT) || !defined(APPLY_DIRECTIONAL_LIGHT_MIX)
# if defined(APPLY_ENV_MODULATE_COLOR)
	color *= myhalf4(qf_FrontColor);
# endif
#else
	color.a *= myhalf(qf_FrontColor.a);
#endif

#endif // APPLY_DECAL

#ifdef QF_ALPHATEST
	QF_ALPHATEST(color.a);
#endif

#ifdef APPLY_GREYSCALE
	color.rgb = Greyscale(color.rgb);
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	myhalf fogDensity = FogDensity(v_TexCoord_FogCoord.pq);
	color.rgb = mix(color.rgb, u_FogColor, fogDensity);
#endif

	qf_FragColor = vec4(color);
}
