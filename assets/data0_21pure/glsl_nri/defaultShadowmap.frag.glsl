#include "include/global.glsl"
#include "defaultShadowmap.res.glsl"

#ifndef NUM_SHADOWS
#define NUM_SHADOWS 1
#endif

layout(location = 0) in vec4 v_Position[4]; 
layout(location = 5) in vec4 v_ShadowProjVector[4];
layout(location = 10) in vec3 v_Normal;

layout(location = 0) out vec4 outFragColor;

layout(set = DESCRIPTOR_PASS_SET, binding = 2) uniform samplerShadow shadowmapSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform texture2D shadowmapTexture[4];

#include "include/qf_vert_utils.glsl"

void main(void)
{
	float finalcolor = 1.0;

#if NUM_SHADOWS >= 1
#define SHADOW_INDEX 0
#define SHADOW_INDEX_COMPONENT x
#define SHADOW_TEXTURE shadowmapTexture[0]
#include "defaultShadowmap.fragment.glsl"
#undef SHADOW_TEXTURE
#undef SHADOW_INDEX_COMPONENT
#undef SHADOW_INDEX
#endif

#if NUM_SHADOWS >= 2
#define SHADOW_INDEX 1
#define SHADOW_INDEX_COMPONENT y
#define SHADOW_TEXTURE shadowmapTexture[1]
#include "defaultShadowmap.fragment.glsl"
#undef SHADOW_TEXTURE
#undef SHADOW_INDEX_COMPONENT
#undef SHADOW_INDEX
#endif

#if NUM_SHADOWS >= 3
#define SHADOW_INDEX 2
#define SHADOW_INDEX_COMPONENT z
#define SHADOW_TEXTURE shadowmapTexture[2]
#include "defaultShadowmap.fragment.glsl"
#undef SHADOW_TEXTURE
#undef SHADOW_INDEX_COMPONENT
#undef SHADOW_INDEX
#endif

#if NUM_SHADOWS >= 4
#define SHADOW_INDEX 3
#define SHADOW_INDEX_COMPONENT w
#define SHADOW_TEXTURE shadowmapTexture[3]
#include "defaultShadowmap.fragment.glsl"
#undef SHADOW_TEXTURE
#undef SHADOW_INDEX_COMPONENT
#undef SHADOW_INDEX
#endif

	outFragColor = vec4(vec3(finalcolor),1.0);
}
