#define DRAWFLAT_NORMAL_STEP	0.5		// floor or ceiling if < abs(normal.z)
#define FOG_TEXCOORD_STEP 1.0/256.0
#define FOG_DIST_NUDGE_FACTOR 0.001
#define M_PI 3.14159265358979323846
#define M_TWOPI 6.28318530717958647692
#define QF_LatLong2Norm(ll) vec3(cos((ll).y) * sin((ll).x), sin((ll).y) * sin((ll).x), cos((ll).x))
#define TextureMatrix2x3Mul(m2x3,tc) (vec2(dot((m2x3)[0].xy, (tc)), dot((m2x3)[0].zw, (tc))) + (m2x3)[1].xy)

float QF_WaveFunc_Sin(float x) {
	return sin(fract(x) * M_TWOPI);
}

float QF_WaveFunc_Triangle(float x) {
	x = fract(x);
	return step(x, 0.25) * x * 4.0 + (2.0 - 4.0 * step(0.25, x) * step(x, 0.75) * x) + ((step(0.75, x) * x - 0.75) * 4.0 - 1.0);
}

float QF_WaveFunc_Square(float x) {
	return step(fract(x), 0.5) * 2.0 - 1.0;
}

float QF_WaveFunc_Sawtooth(float x) {
	return fract(x);
}

float QF_WaveFunc_InverseSawtooth(float x) {
	return 1.0 - fract(x);
}

#define WAVE_SIN(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Sin((phase)+(time)*(freq))))
#define WAVE_TRIANGLE(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Triangle((phase)+(time)*(freq))))
#define WAVE_SQUARE(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Square((phase)+(time)*(freq))))
#define WAVE_SAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Sawtooth((phase)+(time)*(freq))))
#define WAVE_INVERSESAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_InverseSawtooth((phase)+(time)*(freq))))

