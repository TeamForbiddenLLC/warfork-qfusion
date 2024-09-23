
//struct VertexColoringCB {
//    vec4 rgbGenFuncArgs;
//    vec4 alphaGenFuncArgs;
//    vec4 colorConst;
//    vec4 lightAmbient;
//    vec4 lightDiffuse;
//    vec3 lightDir;
//};


// pass
struct DefaultCellShadeCB {
    vec4 textureMatrix[2];
    vec3 entityColor;
	mat3 reflectionTexMatrix;
};

struct DefaultShadowCB {
    mat4 shadowmapMatrix0;
    mat4 shadowmapMatrix1;
    mat4 shadowmapMatrix2;
    mat4 shadowmapMatrix3;
    vec4 shadowDir[4];
    vec4 shadowParams[4];
    vec4 shadowAlpha[2];
    vec4 shadowEntitydist[4];
};

//struct DefaultDistortionCB {
//    vec4 textureParams;
//    vec4 textureMatrix[2];
//    float frontPlane;
//};

//struct DefaultQ3ShaderCB {
//    vec3 wallColor;
//    float softParticleScale;
//    vec4 textureParam;
//    vec4 textureMatrix[2];
//    vec3 floorColor;
//    vec2 zRange;
//    mat4 genTexMatrix;
//    vec3 lightstyleColor[4];
//};

struct DynLight {
    vec4 position;
    vec4 diffuseAndInvRadius;
};

//struct DynamicLightCB {
//    int numberLights;
//    Light dynLights[16];
//};

//struct DefaultMaterialCB {
//    vec4 entityColor;
//    vec4 textureMatrix[2];
//    vec3 lightstyleColor[4];
//    vec4 deluxLightMapScale;
//    float offsetScale;
//    vec3 lightDir;
//    float glossIntensity;
//  	vec3 floorColor;
//    float glossExponent;
//    vec3 wallColor;
//};

// fxaa 
struct DefaultFXAACB {
    mat4 mvp;
    vec4 textureMatrix[2];
};


