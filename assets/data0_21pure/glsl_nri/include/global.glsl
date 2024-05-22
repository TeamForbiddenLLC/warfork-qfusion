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


#include "math_utils.glsl"
#include "colorspace_utils.glsl"

#ifdef VERTEX_SHADER
  layout(location = 0) in vec4 a_Position;
  layout(location = 1) in vec4 a_SVector;
  layout(location = 2) in vec4 a_Normal;
  layout(location = 3) in vec4 a_Color;
  layout(location = 4) in vec2 a_TexCoord;
  # ifdef NUM_LIGHTMAPS
    layout(location = 5) in qf_lmvec01 a_LightmapCoord01;
    # if NUM_LIGHTMAPS > 2
    layout(location = 6) in qf_lmvec23 a_LightmapCoord23;
    # endif // NUM_LIGHTMAPS > 2
    # ifdef LIGHTMAP_ARRAYS
    layout(location = 7) in vec4 a_LightmapLayer0123;
    # endif // LIGHTMAP_ARRAYS
  #endif // NUM_LIGHTMAPS

	#define QF_APPLY_DEFORMVERTS
		#if defined(APPLY_AUTOSPRITE) || defined(APPLY_AUTOSPRITE2)
		layout(location = 8) in vec4 a_SpritePoint;
		#else
		#define a_SpritePoint vec4(0.0)
	#endif

	#if defined(APPLY_AUTOSPRITE2)
		layout(location = 9) in vec4 a_SpriteRightUpAxis;
	#else
		#define a_SpriteRightUpAxis vec4(0.0)
	#endif
#endif // VERTEX_SHADER

#ifdef APPLY_INSTANCED_TRANSFORMS
	layout(set = DESCRIPTOR_OBJECT_SET, binding = 0) uniform vec4 instancePoints[MAX_UNIFORM_INSTANCES * 2]; 
	#define a_InstanceQuat instancePoints[gl_InstanceID*2]
	#define a_InstancePosAndScale instancePoints[gl_InstanceID*2+1]
#endif

#	ifdef QF_NUM_BONE_INFLUENCES
	layout(set = DESCRIPTOR_OBJECT_SET, binding = 1) uniform vec4 dualQuats[MAX_UNIFORM_BONES*2]
	layout(location = 10) in vec4 a_BonesIndices;
	layout(location = 11) in vec4 a_BonesWeights;
#endif

#if defined(APPLY_AUTOSPRITE2)
	layout(location = 12) in vec4 a_SpriteRightUpAxis;
#else
	#define a_SpriteRightUpAxis vec4(0.0)
#endif

struct FrameCB {
	float shaderTime;
	vec3 viewOrigin;
	float mirrorSide;
	mat3 viewAxis;
};

struct ObjectCB {
	vec3 entityOrigin
};

layout(set = DESCRIPTOR_OBJECT_SET, binding = 2) uniform ObjectCB obj; 
layout(set = DESCRIPTOR_FRAME_SET, binding = 0) uniform FrameCB frame; 


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
	#	ifdef QF_APPLY_DEFORMVERTS
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
	#	endif
	#	ifdef APPLY_INSTANCED_TRANSFORMS
	{
		Position.xyz = (cross(a_InstanceQuat.xyz,
			cross(a_InstanceQuat.xyz, Position.xyz) + Position.xyz*a_InstanceQuat.w)*2.0 +
			Position.xyz) * a_InstancePosAndScale.w + a_InstancePosAndScale.xyz;
		Normal = cross(a_InstanceQuat.xyz, cross(a_InstanceQuat.xyz, Normal) + Normal*a_InstanceQuat.w)*2.0 + Normal;
	}
	#	endif
}


