layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultCellShadeCB {
	 vec4 textureMatrix[2];
	 vec3 entityColor;
	 mat3 reflectionTexMatrix;
} pass;

