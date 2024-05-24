//struct DefaultCellShadeUBO {
//    vec3 viewOrigin;
//    float entityDist;
//    mat4 mvp;
//    vec4 textureMatrix[2];
//    vec4 constColor;
//    vec4 rgbGenFuncArgs;
//    vec3 lightAmbient;
//    float entityDist;
//    vec3 lightDiffuse;
//    float frontPlane;
//    vec3 lightDir;
//};
//
//
//struct UBOLightMap {
//    float3 lightMapColors[4];
//};
//
//
//struct UBODefaultOutline {
//    float outlineCutOff;
//    float fogColor;
//};
//
//struct DefaultFogUBO {
//    mat4 mvp;
//    vec3 color;
//    float eyeDist;
//    vec4 eyePlane 
//    vec4 plane;
//    float scale;
//};

struct VertexColoringCB {
#ifdef APPLY_RGB_DISTANCERAMP
    vec4 rgbGenFuncArgs;
#endif
#ifdef APPLY_ALPHA_DISTANCERAMP
    vec4 alphaGenFuncArgs;
#endif  
#if defined(APPLY_RGB_CONST)
    vec4 colorConst;
#elif defined(APPLY_RGB_GEN_DIFFUSELIGHT)
    vec4 lightAmbient;
    vec4 lightDiffuse;
    vec3 lightDir;
#endif

};


struct FrameCB {
  vec4 fogEyePlane,
  vec4 fogPlane,

  vec3 viewOrigin;
  float shaderTime;
  vec2 blendMix;
  float fogScale;
  float eyeDist;
  float mirrorSide;
  mat3 viewAxis;
};

struct ObjectCB {
	vec3 entityOrigin
    float entityDist;
	mat4 mvp;
	float blendMix;
};

// pass

struct DefaultCellShadeCB {
    vec4 textureMatrix[2];
    vec3 entityColor;
	mat3 reflectionTexMatrix;
};

struct DefaultDistortionCB {
    vec4 textureParams;
    vec4 textureMatrix[2];
    float frontPlane;
};

struct DefaultMaterialCB {
    vec4 textureMatrix[2];
    vec3 lightstyleColor[4]
    vec4 deluxLightMapScale;
    float offsetScale;
    vec3 lightDir;
};

// fxaa 
struct DefaultFXAACB {
    mat4 mvp;
    vec4 textureMatrix[2];
};


//struct UBODefaultMaterial {
//    vec4 constColor;
//    vec4 rgbGenFuncArgs;
//};
//
//struct UBODefaultCellShade {
//    mat4 mvp;
//    vec4 entityColor;
//    vec4 textureMatrix[2];
//    vec2 blendMix;
//    vec3 entityDist;
//    mat3 reflectionTextureMat;
//};
//
//
//struct DynamicLight {
//    float4 diffuseAndInvRadius;
//    float3 position;
//    float invRadius;
//};
//
//struct LightConstBuffer {
//
//};
//
//struct UBODynamicLights {
//    int numberLights;
//    DynamicLight lights[NUM_DLIGHTS];
//};
//
//struct FogConstBuffer {
//    vec3 color;
//    float eyeDist;
//    vec4 eyePlane 
//    vec4 plane;
//    float scale;
//};
//
//struct UBOLightMap {
//  int numLightMaps;
//};

