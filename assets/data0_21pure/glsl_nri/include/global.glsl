#if defined(APPLY_FOG_COLOR)
  #define APPLY_ENV_MODULATE_COLOR
#else

  #if defined(APPLY_RGB_DISTANCERAMP) || defined(APPLY_RGB_CONST) || defined(APPLY_RGB_VERTEX) || defined(APPLY_RGB_ONE_MINUS_VERTEX) || defined(APPLY_RGB_GEN_DIFFUSELIGHT)
    #define APPLY_ENV_MODULATE_COLOR
  #else

    #if defined(APPLY_ALPHA_DISTANCERAMP) || defined(APPLY_ALPHA_CONST) || defined(APPLY_ALPHA_VERTEX) || defined(APPLY_ALPHA_ONE_MINUS_VERTEX)
    #define APPLY_ENV_MODULATE_COLOR
    #endif
  #endif
#endif

#define DESCRIPTOR_GLOBAL_SET 0
#define DESCRIPTOR_FRAME_SET 1
#define DESCRIPTOR_PASS_SET 2 
#define DESCRIPTOR_OBJECT_SET 3 

#include "resource.glsl"
#include "math_utils.glsl"
#include "colorspace_utils.glsl"

#include "attributes.glsl"

#ifdef APPLY_INSTANCED_TRANSFORMS
	layout(set = DESCRIPTOR_OBJECT_SET, binding = 0) uniform vec4 instancePoints[MAX_UNIFORM_INSTANCES * 2]; 
	#define a_InstanceQuat instancePoints[gl_InstanceID*2]
	#define a_InstancePosAndScale instancePoints[gl_InstanceID*2+1]
#endif

#ifdef QF_NUM_BONE_INFLUENCES
	layout(set = DESCRIPTOR_OBJECT_SET, binding = 1) uniform vec4 dualQuats[MAX_UNIFORM_BONES*2]
	layout(location = 10) in vec4 a_BonesIndices;
	layout(location = 11) in vec4 a_BonesWeights;
#endif

#if defined(APPLY_AUTOSPRITE2)
	layout(location = 12) in vec4 a_SpriteRightUpAxis;
#else
	#define a_SpriteRightUpAxis vec4(0.0)
#endif

layout(set = DESCRIPTOR_OBJECT_SET, binding = 2) uniform ObjectCB obj; 
layout(set = DESCRIPTOR_OBJECT_SET, binding = 3) uniform VertexColoringCB vc; 
layout(set = DESCRIPTOR_FRAME_SET, binding = 0) uniform FrameCB frame; 

void QF_FogGenCoordTexCoord(
  in vec4 Position, 
  out vec2 outTexCoord)
{
	// side = vec2(inside, outside)
	vec2 side = vec2(step(frame.eyeDist, 0.0), step(0.0, frame.eyeDist));
	float FDist = dot(Position.xyz, frame.fogEyePlane.xyz) - frame.fogEyePlane.w;
	float FVdist = dot(Position.xyz, frame.fogPlane.xyz) - frame.fogPlane.w;
	float VToEyeDist = FVdist - frame.eyeDist;

	// prevent calculations that might result in infinities:
	// always ensure that abs(NudgedVToEyeDist) >= FOG_DIST_NUDGE_FACTOR
	float NudgedVToEyeDist = step(FOG_DIST_NUDGE_FACTOR, VToEyeDist    ) * VToEyeDist +
				step(FOG_DIST_NUDGE_FACTOR, VToEyeDist * -1.0) * VToEyeDist + 
				(step(abs(VToEyeDist), FOG_DIST_NUDGE_FACTOR)) * FOG_DIST_NUDGE_FACTOR;

	float FogDistScale = FVdist / NudgedVToEyeDist;

	float FogS = FDist * dot(side, vec2(1.0, step(FVdist, 0.0) * FogDistScale));
	float FogT = -FVdist;
	outTexCoord = vec2(FogS * fogScale, FogT * fogScale + 1.5*FOG_TEXCOORD_STEP);
}

void QF_FogGenColor(
  in vec4 Position, 
  inout vec4 outColor)
{
	// side = vec2(inside, outside)
	vec2 side = vec2(step(frame.eyeDist, 0.0), step(0.0, frame.eyeDist));
	float FDist = dot(Position.xyz, frame.fogEyePlane.xyz) - frame.fogEyePlane.w;
	float FVdist = dot(Position.xyz, frame.fogPlane.xyz) - frame.fogPlane.w;
	float VToEyeDist = FVdist - frame.eyeDist;

	// prevent calculations that might result in infinities:
	// always ensure that abs(NudgedVToEyeDist) >= FOG_DIST_NUDGE_FACTOR
	float NudgedVToEyeDist = step(FOG_DIST_NUDGE_FACTOR, VToEyeDist    ) * VToEyeDist +
				step(FOG_DIST_NUDGE_FACTOR, VToEyeDist * -1.0) * VToEyeDist + 
				(step(abs(VToEyeDist), FOG_DIST_NUDGE_FACTOR)) * FOG_DIST_NUDGE_FACTOR;

	float FogDistScale = FVdist / NudgedVToEyeDist;

	float FogDist = FDist * dot(side, vec2(1.0, FogDistScale));
	float FogScale = float(clamp(1.0 - FogDist * frame.fogScale, 0.0, 1.0));
	if(obj.isAlphaBlending) {
		outColor *= mix(vec4(1.0), vec4(FogScale), vec4(0,0,0,1));
	} else {
		outColor *= mix(vec4(1.0), vec4(FogScale), vec4(1,1,1,0));
	}
}


vec4 QF_VertexRGBGen(
  in vec4 Position, 
  in vec3 Normal, 
  in vec4 VertexColor)
{

#if defined(APPLY_RGB_CONST) && defined(APPLY_ALPHA_CONST)
	vec4 Color = constColor;
#else
	vec4 Color = vec4(1.0);

  #if defined(APPLY_RGB_CONST)
	  Color.rgb = vc.colorConst.rgb;
  #elif defined(APPLY_RGB_VERTEX)
	  Color.rgb = VertexColor.rgb;
  #elif defined(APPLY_RGB_ONE_MINUS_VERTEX)
	  Color.rgb = vec3(1.0) - VertexColor.rgb;
  #elif defined(APPLY_RGB_GEN_DIFFUSELIGHT)
	  Color.rgb = vec3(vc.lightAmbient.rgb + max(dot(vc.lightDir.xyz, Normal), 0.0) * vc.lightDiffuse.xyz);
  #endif

  #if defined(APPLY_ALPHA_CONST)
	  Color.a = constColor.a;
  #elif defined(APPLY_ALPHA_VERTEX)
	  Color.a = VertexColor.a;
  #elif defined(APPLY_ALPHA_ONE_MINUS_VERTEX)
	  Color.a = 1.0 - VertexColor.a;
  #endif

#endif

#define DISTANCERAMP(x1,x2,y1,y2) mix(y1, y2, smoothstep(x1, x2, float(dot(obj.entityDist - Position.xyz, Normal))))
#ifdef APPLY_RGB_DISTANCERAMP
	Color.rgb *= DISTANCERAMP(vc.rgbGenFuncArgs[2], vc.rgbGenFuncArgs[3], vc.rgbGenFuncArgs[0], vc.rgbGenFuncArgs[1]);
#endif

#ifdef APPLY_ALPHA_DISTANCERAMP
	Color.a *= DISTANCERAMP(vc.alphaGenFuncArgs[2], vc.alphaGenFuncArgs[3], vc.alphaGenFuncArgs[0], vc.alphaGenFuncArgs[1]);
#endif
#undef DISTANCERAMP

	return Color;
}



void QF_TransformVerts(inout vec4 Position, inout vec3 Normal, inout vec3 Tangent, inout vec2 TexCoord) {
	#	ifdef QF_NUM_BONE_INFLUENCES
	{
			ivec4 Indices = ivec4(a_BonesIndices * 2.0);
			vec4 DQReal = dualQuats[Indices.x];
			vec4 DQDual = dualQuats[Indices.x + 1];
		#if QF_NUM_BONE_INFLUENCES >= 2
			DQReal *= a_BonesWeights.x;
			DQDual *= a_BonesWeights.x;
			vec4 DQReal1 = dualQuats[Indices.y];
			vec4 DQDual1 = dualQuats[Indices.y + 1];
			float Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.y;
			DQReal += DQReal1 * Scale;
			DQDual += DQDual1 * Scale;
  		#if QF_NUM_BONE_INFLUENCES >= 3
	  		DQReal1 = dualQuats[Indices.z];
	  		DQDual1 = dualQuats[Indices.z + 1];
	  		Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.z;
	  		DQReal += DQReal1 * Scale;
	  		DQDual += DQDual1 * Scale;
    		#if QF_NUM_BONE_INFLUENCES >= 4
	    		DQReal1 = dualQuats[Indices.w];
	    		DQDual1 = dualQuats[Indices.w + 1];
	    		Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.w;
	    		DQReal += DQReal1 * Scale;
	    		DQDual += DQDual1 * Scale;
    		#endif 
  		#endif
			float Len = 1.0 / length(DQReal);
			DQReal *= Len;
			DQDual *= Len;
		#endif 
			Position.xyz += (cross(DQReal.xyz, cross(DQReal.xyz, Position.xyz) + Position.xyz * DQReal.w + DQDual.xyz) +
				DQDual.xyz*DQReal.w - DQReal.xyz*DQDual.w) * 2.0;
			Normal += cross(DQReal.xyz, cross(DQReal.xyz, Normal) + Normal * DQReal.w) * 2.0;
			Tangent += cross(DQReal.xyz, cross(DQReal.xyz, Tangent) + Tangent * DQReal.w) * 2.0;
	}
	#	endif
	{
		#if defined(DEFORMV_WAVE)
			vec4 arg = DEFORMV_CONSTANT;
			Position.xyz += DEFORMV_FUNC(frame.shaderTime,arg.x,arg.y,arg.z+(arg.w > 0 ? arg.x : 0.0)*(Position.x+Position.y+Position.z),arg.w) * Normal.xyz;
		#elif defined(DEFORMV_MOVE)
			vec4 arg = DEFORMV_CONSTANT;
			Position.xyz += DEFORMV_FUNC(frame.shaderTime,arg.x,arg.y,arg.z,arg.w) * vec3(arg.x, arg.y, arg.z);
		#elif defined(DEFORMV_BULGE)
			vec4 arg = DEFORMV_CONSTANT;
			const float t = sin(TexCoord.s * arg.x + u_QF_ShaderTime * arg.z);
			Position.xyz += max (-1.0 + arg.w, t) * arg.y * Normal.xyz;
		#elif defined(DEFORMV_AUTOSPRITE)
		 	 vec3 right = (1.0 + step(0.5, TexCoord.s) * -2.0) * frame.viewAxis[1] * u_QF_MirrorSide;
		 	 vec3 up = (1.0 + step(0.5, TexCoord.t) * -2.0) * frame.viewAxis[2];
		 	 vec3 forward = -1.0 * frame.viewAxis[0];
		 	 Position.xyz = a_SpritePoint.xyz + (right + up) * a_SpritePoint.w;
		 	 Normal.xyz = forward;
		 	 TexCoord.st = vec2(step(0.5, TexCoord.s),step(0.5, TexCoord.t));
		#elif defined(DEFORMV_AUTOPARTICLE)
		 	 vec3 right = (1.0 + TexCoord.s * -2.0) * frame.viewAxis[1] * u_QF_MirrorSide;
		 	 vec3 up = (1.0 + TexCoord.t * -2.0) * frame.viewAxis[2];
		 	 vec3 forward = -1.0 * frame.viewAxis[0];
		 	 // prevent the particle from disappearing at large distances
		 	 float t = dot(a_SpritePoint.xyz + obj.entityOrigin - frame.viewOrigin, frame.viewAxis[0]);
		 	 t = 1.5 + step(20.0, t) * t * 0.006;
		 	 Position.xyz = a_SpritePoint.xyz + (right + up) * t * a_SpritePoint.w;
		 	 Normal.xyz = forward;
		#elif defined(DEFORMV_AUTOSPRITE2)
		 	 vec3 right = QF_LatLong2Norm(a_SpriteRightUpAxis.xy) * frame.mirrorSide;
		 	 vec3 up = QF_LatLong2Norm(a_SpriteRightUpAxis.zw);
		 	 // mid of quad to camera vector
		 	 vec3 dist = frame.viewOrigin - obj.entityOrigin - a_SpritePoint.xyz;
		 	 // filter any longest-axis-parts off the camera-direction
		 	 vec3 forward = normalize(dist - up * dot(dist, up));
		 	 // the right axis vector as it should be to face the camera
		 	 vec3 newright = cross(up, forward);

		 	 // rotate the quad vertex around the up axis vector
		 	 float t = dot(right, Position.xyz - a_SpritePoint.xyz);
		 	 Position.xyz += t * (newright - right);
		 	 Normal.xyz = forward;
		#endif
	}
	#	ifdef APPLY_INSTANCED_TRANSFORMS
	{
		Position.xyz = (cross(a_InstanceQuat.xyz,
			cross(a_InstanceQuat.xyz, Position.xyz) + Position.xyz*a_InstanceQuat.w)*2.0 +
			Position.xyz) * a_InstancePosAndScale.w + a_InstancePosAndScale.xyz;
		Normal = cross(a_InstanceQuat.xyz, cross(a_InstanceQuat.xyz, Normal) + Normal*a_InstanceQuat.w)*2.0 + Normal;
	}
	#	endif
}


