#include_if(APPLY_CELSHADING) "material_celshading.frag.glsl"

vec3 DirectionalLightColor(in vec3 surfaceNormalModelspace, out vec3 weightedDiffuseNormalModelspace)
{
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
	color.rgb += u_LightDiffuse.rgb * float(max (diffuseProduct, 0.0)) + u_LightAmbient;
#endif

#endif // APPLY_CELSHADING

	return color;
}
