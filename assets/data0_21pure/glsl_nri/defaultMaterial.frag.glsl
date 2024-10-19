#include "include/global.glsl" 
#include "defaultMaterial.res.glsl"

layout(set = DESCRIPTOR_GLOBAL_SET, binding = 0) uniform sampler lightmapTextureSample;
layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1) uniform texture2D lightmapTexture[4];
//layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1 + (16 * 1)) uniform texture2D lightmapTexture1[16];
//layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1 + (16 * 2)) uniform texture2D lightmapTexture2[16];
//layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1 + (16 * 3)) uniform texture2D lightmapTexture3[16];

layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform sampler   u_BaseSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 4) uniform texture2D u_BaseTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 5) uniform sampler   u_NormalSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 6) uniform texture2D u_NormalTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 7) uniform sampler   u_GlossSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 8) uniform texture2D u_GlossTexture;

layout(set = DESCRIPTOR_PASS_SET, binding = 9) uniform sampler   u_DecalSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 10) uniform texture2D u_DecalTexture;

layout(set = DESCRIPTOR_PASS_SET, binding = 11) uniform sampler    u_EntityDecalSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 12) uniform texture2D u_EntityDecalTexture;

layout(location = 0) in vec3 v_Position; 
layout(location = 1) in vec3 v_EyeVector; 
layout(location = 2) in vec4 v_LightmapTexCoord01;
layout(location = 3) in vec4 v_LightmapTexCoord23;
layout(location = 4) flat in ivec4 v_LightmapLayer0123;
layout(location = 5) in vec4 frontColor; 
layout(location = 6) in vec4 v_TexCoord_FogCoord;

layout(location = 7) in vec3 v_Tangent; 
layout(location = 8) in vec3 v_Normal; 
layout(location = 9) in vec3 v_Binormal; 

layout(location = 0) out vec4 outFragColor;

#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
//// The following reliefmapping and offsetmapping routine was taken from DarkPlaces
//// The credit goes to LadyHavoc (as always)
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
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector *  step(texture(NormalmapTexture, RT.xy).a, RT.z);
	RT += OffsetVector * (step(texture(NormalmapTexture, RT.xy).a, RT.z)          - 0.5);
	RT += OffsetVector * (step(texture(NormalmapTexture, RT.xy).a, RT.z) * 0.5    - 0.25);
	RT += OffsetVector * (step(texture(NormalmapTexture, RT.xy).a, RT.z) * 0.25   - 0.125);
	RT += OffsetVector * (step(texture(NormalmapTexture, RT.xy).a, RT.z) * 0.125  - 0.0625);
	RT += OffsetVector * (step(texture(NormalmapTexture, RT.xy).a, RT.z) * 0.0625 - 0.03125);
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
	TexCoord_ -= OffsetVector * texture(NormalmapTexture, TexCoord_).a;
	TexCoord_ -= OffsetVector * texture(NormalmapTexture, TexCoord_).a;
	return TexCoord_;
#endif // APPLY_RELIEFMAPPING
}
#endif // defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)

void main()
{
	mat3 strMat;
	strMat[0] = v_Tangent;
	strMat[2] = v_Normal;
	strMat[1] = v_Binormal;

//#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
//	// apply offsetmapping
//	vec2 TexCoordOffset = OffsetMapping(sampler2D(u_NormalTexture,u_NormalSampler), v_TexCoord_FogCoord.st, v_EyeVector, pass.offsetScale);
//	#define v_TexCoord TexCoordOffset
//#else
	#define v_TexCoord v_TexCoord_FogCoord.st
//#endif

	vec3 surfaceNormal;
	vec3 surfaceNormalModelspace;
	vec3 weightedDiffuseNormalModelspace;

#if !defined(APPLY_DIRECTIONAL_LIGHT) && !defined(NUM_LIGHTMAPS)
	vec4 color = vec4 (1.0, 1.0, 1.0, 1.0);
#else
	vec4 color = vec4 (0.0, 0.0, 0.0, 1.0);
#endif

	vec4 decal = vec4 (0.0, 0.0, 0.0, 1.0);

	// get the surface normal
	surfaceNormal = normalize(vec3(texture(sampler2D(u_NormalTexture,u_NormalSampler), v_TexCoord)) - vec3 (0.5));
	surfaceNormalModelspace = normalize(strMat * surfaceNormal);

#ifdef APPLY_DIRECTIONAL_LIGHT 
	{
		#ifdef APPLY_DIRECTIONAL_LIGHT_FROM_NORMAL
			vec3 diffuseNormalModelspace = strMat[2];
		#else
			vec3 diffuseNormalModelspace = obj.lightDir;
		#endif // APPLY_DIRECTIONAL_LIGHT_FROM_NORMAL

		weightedDiffuseNormalModelspace = diffuseNormalModelspace;

		{
		#ifdef APPLY_CELSHADING
		
			#ifdef APPLY_HALFLAMBERT
				float diffuseProduct = dot (surfaceNormalModelspace, diffuseNormalModelspace);
				float diffuseProductPositive = clamp(diffuseProduct, 0.0, 1.0) * 0.5 + 0.5;
				diffuseProductPositive *= diffuseProductPositive;
				float diffuseProductNegative =  clamp(diffuseProduct, -1.0, 0.0) * 0.5 - 0.5;
				diffuseProductNegative = diffuseProductNegative * diffuseProductNegative - 0.25;
				diffuseProduct = diffuseProductPositive;
			#else
				float diffuseProduct = float(dot (surfaceNormalModelspace, diffuseNormalModelspace));
				float diffuseProductPositive = max (diffuseProduct, 0.0);
				float diffuseProductNegative = (-min (diffuseProduct, 0.0) - 0.3);
			#endif // APPLY_HALFLAMBERT

			// smooth the hard shadow edge
			float hardShadow += floor(max(diffuseProduct + 0.1, 0.0) * 2.0);
			hardShadow += floor(max(diffuseProduct + 0.055, 0.0) * 2.0);
			hardShadow += floor(diffuseProductPositive * 2.0);

			diffuseProduct = 0.6 + hardShadow * 0.09 + diffuseProductPositive * 0.14;

			// backlight
			diffuseProduct += ceil(diffuseProductNegative * 2.0) * 0.085 + diffuseProductNegative * 0.085;
			color.rgb += vec3(diffuseProduct);
		
		#else
		}

		#ifdef APPLY_HALFLAMBERT
			float diffuseProduct = float ( clamp(dot (surfaceNormalModelspace, diffuseNormalModelspace), 0.0, 1.0) * 0.5 + 0.5 );
			diffuseProduct *= diffuseProduct;
		#else
			float diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
		#endif // APPLY_HALFLAMBERT

		#ifdef APPLY_DIRECTIONAL_LIGHT_MIX
			color.rgb += frontColor.rgb;
		#else
			color.rgb += obj.lightDiffuse.rgb * float(max (diffuseProduct, 0.0)) + obj.lightAmbient;
		#endif

		#endif // APPLY_CELSHADING

	}
	//color.rgb += DirectionalLightColor(surfaceNormalModelspace, weightedDiffuseNormalModelspace);
#endif

#ifdef NUM_LIGHTMAPS
{
	// get light normal
	vec3 diffuseNormalModelspace = normalize(texture(sampler2D(lightmapTexture[0],lightmapTextureSample), v_LightmapTexCoord01.st + vec2(pass.deluxLightMapScale.x, 0.0)).rgb - vec3(0.5));
	// calculate directional shading
	float diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));

#ifdef APPLY_FBLIGHTMAP
	weightedDiffuseNormalModelspace = diffuseNormalModelspace;
	// apply lightmap color
	color.rgb += vec3(max (diffuseProduct, 0.0) * texture(sampler2D(lightmapTexture[0],lightmapTextureSample), v_LightmapTexCoord01.st).rgb);
#else
	#define NORMALIZE_DIFFUSE_NORMAL
	weightedDiffuseNormalModelspace = pass.lightstyleColor[0] * diffuseNormalModelspace;
	// apply lightmap color
	color.rgb += pass.lightstyleColor[0] * float(max (diffuseProduct, 0.0)) * texture(sampler2D(lightmapTexture[0],lightmapTextureSample), v_LightmapTexCoord01.st).rgb;
#endif // APPLY_FBLIGHTMAP

#ifdef APPLY_AMBIENT_COMPENSATION
	// compensate for ambient lighting
	color.rgb += float((1.0 - max (diffuseProduct, 0.0))) * obj.lightAmbient;
#endif

#if NUM_LIGHTMAPS >= 2
	diffuseNormalModelspace = normalize(texture(sampler2D(lightmapTexture[1],lightmapTextureSample), v_LightmapTexCoord01.pq+vec2(pass.deluxLightMapScale.y,0.0)).rgb - vec3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += pass.lightstyleColor[1] * diffuseNormalModelspace;
	color.rgb += pass.lightstyleColor[1].rgb * max (diffuseProduct, 0.0) * texture(sampler2D(lightmapTexture[1],lightmapTextureSample), v_LightmapTexCoord01.pq).rgb;
#endif 
#if NUM_LIGHTMAPS >= 3
	diffuseNormalModelspace = normalize(texture(sampler2D(lightmapTexture[2],lightmapTextureSample), v_LightmapTexCoord23.st+vec2(pass.deluxLightMapScale.z,0.0)).rgb - vec3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += pass.lightstyleColor[2] * diffuseNormalModelspace;
	color.rgb += pass.lightstyleColor[2].rgb * max (diffuseProduct, 0.0) * texture(sampler2D(lightmapTexture[2],lightmapTextureSample), v_LightmapTexCoord23.st).rgb;
#endif 
#if NUM_LIGHTMAPS >= 4
	diffuseNormalModelspace = normalize(texture(sampler2D(lightmapTexture[3],lightmapTextureSample), v_LightmapTexCoord23.pq+vec2(pass.deluxLightMapScale.w,0.0)).rgb - vec3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += pass.lightstyleColor[3] * diffuseNormalModelspace;
	color.rgb += pass.lightstyleColor[3].rgb * max (diffuseProduct, 0.0) * texture(sampler2D(lightmapTexture[3],lightmapTextureSample), v_LightmapTexCoord23.pq).rgb;
#endif 

}
#endif

#if defined(NUM_DLIGHTS)
{
	for (int dlight = 0; dlight < min(lights.numberLights, 16); dlight += 4)
	{
		vec3 STR0 = vec3(lights.dynLights[dlight].position.xyz - v_Position);
		vec3 STR1 = vec3(lights.dynLights[dlight + 1].position.xyz - v_Position);
		vec3 STR2 = vec3(lights.dynLights[dlight + 2].position.xyz - v_Position);
		vec3 STR3 = vec3(lights.dynLights[dlight + 3].position.xyz - v_Position);
		vec4 distance = vec4(length(STR0), length(STR1), length(STR2), length(STR3));
		vec4 falloff = clamp(vec4(1.0) - (distance / lights.dynLights[dlight + 3].diffuseAndInvRadius), 0.0, 1.0);

		falloff *= falloff;

		distance = vec4(1.0) / distance;
		falloff *= max(vec4(
			dot(STR0 * distance.xxx, surfaceNormalModelspace),
			dot(STR1 * distance.yyy, surfaceNormalModelspace),
			dot(STR2 * distance.zzz, surfaceNormalModelspace),
			dot(STR3 * distance.www, surfaceNormalModelspace)), 0.0);

		color.rgb += vec3(
			dot(lights.dynLights[dlight].diffuseAndInvRadius, falloff),
			dot(lights.dynLights[dlight + 1].diffuseAndInvRadius, falloff),
			dot(lights.dynLights[dlight + 2].diffuseAndInvRadius, falloff));
	}
}
#endif

#ifdef APPLY_SPECULAR

	#ifdef NORMALIZE_DIFFUSE_NORMAL
		vec3 specularNormal = normalize (vec3(normalize (weightedDiffuseNormalModelspace)) + vec3 (normalize (obj.entityDist - v_Position)));
	#else
		vec3 specularNormal = normalize (weightedDiffuseNormalModelspace + vec3 (normalize (obj.entityDist - v_Position)));
	#endif

	float specularProduct = float(dot (surfaceNormalModelspace, specularNormal));
	color.rgb += (vec3(texture(sampler2D(u_GlossTexture, u_GlossSampler), v_TexCoord)) * pass.glossIntensity) * pow(float(max(specularProduct, 0.0)), pass.glossExponent);
#endif // APPLY_SPECULAR

#if defined(APPLY_BASETEX_ALPHA_ONLY) && !defined(APPLY_DRAWFLAT)
	color = min(color, vec4(texture(sampler2D(u_BaseTexture,u_BaseSampler), v_TexCoord).a));
#else
	vec4 diffuse;

#ifdef APPLY_DRAWFLAT
	float n = float(step(DRAWFLAT_NORMAL_STEP, abs(strMat[2].z)));
	diffuse = vec4(mix(pass.wallColor, pass.floorColor, n), float(texture(sampler2D(u_BaseTexture,u_BaseSampler), v_TexCoord).a));
#else
	diffuse = vec4(texture(sampler2D(u_BaseTexture,u_BaseSampler), v_TexCoord));
#endif

#ifdef APPLY_ENTITY_DECAL

	#ifdef APPLY_ENTITY_DECAL_ADD
		decal.rgb = texture(sampler2D(u_EntityDecalTexture, u_EntityDecalSampler), v_TexCoord).rgb;
		diffuse.rgb += pass.entityColor.rgb * decal.rgb;
	#else
		decal = vec4(pass.entityColor.rgb, 1.0) * vec4(texture(sampler2D(u_EntityDecalTexture, u_EntityDecalSampler), v_TexCoord));
		diffuse.rgb = mix(diffuse.rgb, decal.rgb, decal.a);
	#endif // APPLY_ENTITY_DECAL_ADD

#endif // APPLY_ENTITY_DECAL

	color = color * diffuse;
#endif // defined(APPLY_BASETEX_ALPHA_ONLY) && !defined(APPLY_DRAWFLAT)

#ifdef APPLY_DECAL

#ifdef APPLY_DECAL_ADD
	decal.rgb = vec3(frontColor.rgb) * vec3(texture(sampler2D(u_DecalTexture, u_DecalSampler), v_TexCoord));
	color.rgb += decal.rgb;
#else
	decal = vec4(frontColor.rgb, 1.0) * vec4(texture(sampler2D(u_DecalTexture, u_DecalSampler), v_TexCoord));
	color.rgb = mix(color.rgb, decal.rgb, decal.a);
#endif // APPLY_DECAL_ADD
	color.a *= frontColor.a;
#else

#if !defined (APPLY_DIRECTIONAL_LIGHT) || !defined(APPLY_DIRECTIONAL_LIGHT_MIX)
# if defined(APPLY_ENV_MODULATE_COLOR)
	color *= vec4(frontColor);
# endif
#else
	color.a *= float(frontColor.a);
#endif

#endif // APPLY_DECAL

#ifdef QF_ALPHATEST
	QF_ALPHATEST(color.a);
#endif

#ifdef APPLY_GREYSCALE
	color.rgb = Greyscale(color.rgb);
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	float fogDensity = FogDensity(v_TexCoord_FogCoord.pq);
	color.rgb = mix(color.rgb, frame.fogColor, fogDensity);
#endif

	outFragColor = vec4(color);
}
