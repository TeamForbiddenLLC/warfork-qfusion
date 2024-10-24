/*
Copyright (C) 2011 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef R_RESOURCE_H
#define R_RESOURCE_H

#include "../gameshared/q_arch.h"

#define MAX_GLSL_BONE_UNIFORMS 128

struct mat4 {
  union {
    float v[16];
    struct {
      float col0[4];
      float col1[4];
      float col2[4];
      float col3[4];
    };
  };
};

struct mat3 {
	union {
		float v[9];
		struct {
			float col0[3];
			float col1[3];
			float col2[3];
		};
	};
};


struct dualQuat8 {
	union {
		float v[8];
		struct {
			float x1;
			float y1;
			float z1;
			float w1;
			float x2;
			float y2;
			float z2;
			float w2;
		};
	};
};

struct vec4 {
	union {
		float v[4];
		struct {
			float x;
			float y;
			float z;
			float w;
		};
	};
};

struct vec2 {
	union {
		float v[2];
		struct {
			float x;
			float y;
		};
	};
};

struct vec3 {
	union {
		float v[3];
		struct {
			float x;
			float y;
			float z;
		};
	};
};

struct FrameCB {
	struct vec3 viewOrigin;
	float shaderTime;
	struct vec2 blendMix;
	float zNear;
	float zFar;
	float fogScale;
	float eyeDist;
  int pad[2];
	struct vec3 fogColor;
	float mirrorSide;
  union {
	  struct mat3 viewAxis;
    struct mat4 viewAxisPadding0;
  };
};

struct DynLight {
    struct vec4 position;
    struct vec4 diffuseAndInvRadius;
};

struct DynamicLightCB {
    int numberLights;
    int pad3[3];
    struct DynLight dynLights[16];
};

static inline size_t DynamicLightCB_Size(int numLights) {
    return sizeof(int) + numLights * sizeof(struct DynLight);
}

struct ObjectCB {
   struct vec4 fogEyePlane;
   struct vec4 fogPlane;
   struct mat4 mvp;
   struct mat4 mv;
   struct vec4 rgbGenFuncArgs;
   struct vec4 alphaGenFuncArgs;
   struct vec4 colorConst;
   struct vec4 lightAmbient;
   struct vec4 lightDiffuse;
   struct vec3 entityOrigin;
   float isAlphaBlending;
   struct vec3 lightDir;
   float pad0;
   struct vec3 entityDist;
   float pad1;
   float texutreMatrix[8];
};

static inline void ObjectCB_SetTextureMatrix(struct ObjectCB *cb, float* matrix) {
	  cb->texutreMatrix[0] = matrix[0];
    cb->texutreMatrix[1] = matrix[4];
    cb->texutreMatrix[2] = matrix[1];
    cb->texutreMatrix[3] = matrix[5];
    cb->texutreMatrix[4] = matrix[12];
    cb->texutreMatrix[5] = matrix[13];
}

struct DualQuatCB {
    struct dualQuat8 bones[128];
};
// pass

struct DefaultCellShadeCB {
	struct vec4 entityColor;
  union {
  	struct mat3 reflectionTexMatrix;
    struct mat4 padMat0; // padding mat4
  };
};

struct DefaultShadowCB {
    struct mat4 shadowmapMatrix0;
    struct mat4 shadowmapMatrix1;
    struct mat4 shadowmapMatrix2;
    struct mat4 shadowmapMatrix3;
    struct vec4 shadowDir[4];
    struct vec4 shadowParams[4];
    struct vec4 shadowAlpha[2];
    struct vec4 shadowEntitydist[4];
};


struct DefaultDistortionCB {
    struct vec4 textureParams;
    float frontPlane;
};

struct DefaultQ3ShaderCB {
    struct mat4 genTexMatrix;
    struct vec4 textureParam;
    struct vec4 lightstyleColor[4];
    struct vec3 wallColor;
    float softParticlesScale;
    struct vec4 zRange;
    struct vec4 floorColor;
};

struct DefaultMaterialCB {
  struct vec4 entityColor;
  struct vec4 lightstyleColor[4];
  struct vec4 deluxLightMapScale;
  struct vec3 lightDir;
  float glossIntensity;
  struct vec3 floorColor;
  float glossExponent;
  struct vec3 wallColor;
  float offsetScale;
};

struct DefaultOutlinePushConstant {
  float outlineHeight;
  float outlineCutoff;
};

#endif
