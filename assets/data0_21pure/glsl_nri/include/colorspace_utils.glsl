#define decodedepthmacro24(d) dot((d).rgb, vec3(1.0, 1.0 / 255.0, 1.0 / 65025.0))
#define encodedepthmacro24(d) fract(d * vec4(1.0, 255.0, 65025.0, 0.0))
// Use green for near because it has one more bit of precision
#define decodedepthmacro16(d) dot((d).rgb, vec3(1.0, 1.0 / 961.0, 1.0 / 31.0))
#define encodedepthmacro16(d) fract(d * vec4(1.0, 961.0, 31.0, 0.0))


vec3 Greyscale(vec3 color) {
	return vec3(dot(color, vec3(0.299, 0.587, 0.114)));
}

vec3 YUV2RGB(vec3 yuv) {
	// Standard definition TV color matrix
	const mat3 yuv2rgb = mat3(
		1.0, 1.0, 1.0, 
		0.0, -0.343, -0.711,
		1.400, -0.711, 0.0
	);
	const vec3 yuv_sub = vec3(0.0, 0.5, 0.5);

	return clamp( (yuv2rgb * (yuv - yuv_sub)).xyz, 0.0, 1.0 );
}

vec3 YUV2RGB_HDTV(vec3 yuv)
{
	// High Definition TV color matrix
	const mat3 yuv2rgb = mat3(
		1.164, 1.164, 1.164, 
		0.0, -0.213, 2.212,
		1.793, -0.533, 0.0
	);
	const vec3 yuv_sub = vec3(0.0625, 0.5, 0.5);

	return clamp( (yuv2rgb * (yuv - yuv_sub)).xyz, 0.0, 1.0 );
}

float FogDensity(vec2 coord) {
  return sqrt(clamp(coord.x,0.0,1.0)) * step(FOG_TEXCOORD_STEP,coord.y);
}

//vec4 VertexRGBGen(
//  vec4 constColor,
//  vec4 rgbGenFuncArgs,
//  vec3 lightAmbient,
//  float entityDist,
//  vec3 lightDiffuse,
//  vec3 lightDir,
//  in vec4 Position, 
//  in vec3 Normal, 
//  in vec4 VertexColor, 
//  VertexRGBoptions)
//{
//
//#if defined(APPLY_RGB_CONST) && defined(APPLY_ALPHA_CONST)
//	vec4 Color = constColor;
//#else
//	vec4 Color = vec4(1.0);
//
//  #if defined(APPLY_RGB_CONST)
//	  Color.rgb = constColor.rgb;
//  #elif defined(APPLY_RGB_VERTEX)
//	  Color.rgb = VertexColor.rgb;
//  #elif defined(APPLY_RGB_ONE_MINUS_VERTEX)
//	  Color.rgb = vec3(1.0) - VertexColor.rgb;
//  #elif defined(APPLY_RGB_GEN_DIFFUSELIGHT)
//	  Color.rgb = vec3(lightAmbient.rgb + max(dot(lightDir.xyz, Normal), 0.0) * lightDiffuse.xyz);
//  #endif
//
//  #if defined(APPLY_ALPHA_CONST)
//	  Color.a = constColor.a;
//  #elif defined(APPLY_ALPHA_VERTEX)
//	  Color.a = VertexColor.a;
//  #elif defined(APPLY_ALPHA_ONE_MINUS_VERTEX)
//	  Color.a = 1.0 - VertexColor.a;
//  #endif
//
//#endif
//
//#define DISTANCERAMP(x1,x2,y1,y2) mix(y1, y2, smoothstep(x1, x2, float(dot(entityDist - Position.xyz, Normal))))
//#ifdef APPLY_RGB_DISTANCERAMP
//	Color.rgb *= DISTANCERAMP(rgbGenFuncArgs[2], rgbGenFuncArgs[3], rgbGenFuncArgs[0], rgbGenFuncArgs[1]);
//#endif
//
//#ifdef APPLY_ALPHA_DISTANCERAMP
//	Color.a *= DISTANCERAMP(rgbGenFuncArgs[2], rgbGenFuncArgs[3], rgbGenFuncArgs[0], rgbGenFuncArgs[1]);
//#endif
//#undef DISTANCERAMP
//
//	return Color;
//}


//void FogGenColor(
//  vec4 fogEyePlane,
//  vec4 fogPlane,
//  float fogScale,
//  float eyeDist,
//  in vec4 Position, 
//  inout vec4 outColor, 
//  in vec2 blendMix)
//{
//	// side = vec2(inside, outside)
//	vec2 side = vec2(step(eyeDist, 0.0), step(0.0, eyeDist));
//	float FDist = dot(Position.xyz, fogEyePlane.xyz) - fogEyePlane.w;
//	float FVdist = dot(Position.xyz, fogPlane.xyz) - fogPlane.w;
//	float VToEyeDist = FVdist - eyeDist;
//
//	// prevent calculations that might result in infinities:
//	// always ensure that abs(NudgedVToEyeDist) >= FOG_DIST_NUDGE_FACTOR
//	float NudgedVToEyeDist = step(FOG_DIST_NUDGE_FACTOR, VToEyeDist    ) * VToEyeDist +
//				step(FOG_DIST_NUDGE_FACTOR, VToEyeDist * -1.0) * VToEyeDist + 
//				(step(abs(VToEyeDist), FOG_DIST_NUDGE_FACTOR)) * FOG_DIST_NUDGE_FACTOR;
//
//	float FogDistScale = FVdist / NudgedVToEyeDist;
//
//	float FogDist = FDist * dot(side, vec2(1.0, FogDistScale));
//	float FogScale = float(clamp(1.0 - FogDist * fogScale, 0.0, 1.0));
//	outColor *= mix(vec4(1.0), vec4(FogScale), blendMix.xxxy);
//}

//void FogGenCoordTexCoord(
//  vec4 fogEyePlane,
//  vec4 fogPlane,
//  float fogScale,
//  float eyeDist,
//  in vec4 Position, 
//  out vec2 outTexCoord)
//{
//	// side = vec2(inside, outside)
//	vec2 side = vec2(step(eyeDist, 0.0), step(0.0, eyeDist));
//	float FDist = dot(Position.xyz, fogEyePlane.xyz) - fogEyePlane.w;
//	float FVdist = dot(Position.xyz, fogPlane.xyz) - fogPlane.w;
//	float VToEyeDist = FVdist - eyeDist;
//
//	// prevent calculations that might result in infinities:
//	// always ensure that abs(NudgedVToEyeDist) >= FOG_DIST_NUDGE_FACTOR
//	float NudgedVToEyeDist = step(FOG_DIST_NUDGE_FACTOR, VToEyeDist    ) * VToEyeDist +
//				step(FOG_DIST_NUDGE_FACTOR, VToEyeDist * -1.0) * VToEyeDist + 
//				(step(abs(VToEyeDist), FOG_DIST_NUDGE_FACTOR)) * FOG_DIST_NUDGE_FACTOR;
//
//	float FogDistScale = FVdist / NudgedVToEyeDist;
//
//	float FogS = FDist * dot(side, vec2(1.0, step(FVdist, 0.0) * FogDistScale));
//	float FogT = -FVdist;
//	outTexCoord = vec2(FogS * fogScale, FogT * fogScale + 1.5*FOG_TEXCOORD_STEP);
//}

