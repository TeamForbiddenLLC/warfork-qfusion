layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultDistortionCB {
    vec4 textureParams;
    vec4 textureMatrix[2];
    float frontPlane;
} pass;

