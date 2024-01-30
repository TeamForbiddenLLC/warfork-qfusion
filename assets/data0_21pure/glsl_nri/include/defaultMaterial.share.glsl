layout( push_constant ) uniform Constants
{
	vec4 texMatrix[2];
#ifdef APPLY_RGB_DISTANCERAMP
	vec4 RGBGenFuncArgs;
#endif
#ifdef APPLY_ALPHA_DISTANCERAMP
	vec4 AlphaGenFuncArgs;
#endif
#if defined(APPLY_RGB_CONST)
	vec4 constColor;
#endif
	vec2 blendMix;
    vec2 glossFactors;

    vec3 wallColor;
    float __padding0;
    vec3 floorColor;
    float __padding1;
} constants;
