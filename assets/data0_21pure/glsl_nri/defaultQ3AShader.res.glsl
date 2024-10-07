layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultQ3ShaderCB {
    vec3 wallColor;
    float softParticleScale;
    vec4 textureParam;
    vec4 textureMatrix[2];
    vec3 floorColor;
    vec2 zRange;
    mat4 genTexMatrix;
    vec3 lightstyleColor[4];
} pass;

