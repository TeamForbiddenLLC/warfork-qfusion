layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultShadowCB {
    mat4 shadowmapMatrix0;
    mat4 shadowmapMatrix1;
    mat4 shadowmapMatrix2;
    mat4 shadowmapMatrix3;
    vec4 shadowDir[4];
    vec4 shadowParams[4];
    vec4 shadowAlpha[2];
    vec4 shadowEntitydist[4];
} pass;

