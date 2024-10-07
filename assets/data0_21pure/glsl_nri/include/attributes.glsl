#ifdef VERTEX_SHADER
  layout(location = 0) in vec4 a_Position;
  layout(location = 1) in vec4 a_SVector;
  layout(location = 2) in vec4 a_Normal;
  layout(location = 3) in vec4 a_Color;
  layout(location = 4) in vec2 a_TexCoord;
  layout(location = 5) in vec4 a_LightmapCoord01;
  layout(location = 6) in vec4 a_LightmapCoord23;
  layout(location = 7) in vec4 a_LightmapLayer0123;


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
  
  #ifdef QF_NUM_BONE_INFLUENCES
	  layout(location = 10) in vec4 a_BonesIndices;
	  layout(location = 11) in vec4 a_BonesWeights;
  #endif
#endif // VERTEX_SHADER