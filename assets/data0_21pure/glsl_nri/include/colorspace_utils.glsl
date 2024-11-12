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
