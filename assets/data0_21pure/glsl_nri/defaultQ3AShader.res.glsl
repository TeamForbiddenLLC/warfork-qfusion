layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultQ3ShaderCB {
    struct vec3 wallColor;
    float softParticleScale;
    struct vec4 textureParam;
    struct vec4 textureMatrix[2];
    struct vec3 floorColor;
    struct vec2 zRange;
    struct mat4 genTexMatrix;
    struct vec3 lightstyleColor[4];
} pass;

