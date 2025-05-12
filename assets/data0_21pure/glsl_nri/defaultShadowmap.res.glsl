layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform DefaultShadowCB {
    mat4 shadowmapMatrix[4];
    vec4 shadowDir[4];
    vec4 shadowParams[4];
    vec4 shadowAlpha[2];
    vec4 shadowEntitydist[4];
} pass;

