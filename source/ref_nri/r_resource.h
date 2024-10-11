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
	struct vec3 fogColor;
	float mirrorSide;
	struct mat3 viewAxis;
};

struct DynLight {
    struct vec4 position;
    struct vec4 diffuseAndInvRadius;
};

struct DynamicLightCB {
    int numberLights;
    struct DynLight dynLights[16];
};

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
   struct vec4 texutreMatrix[2];
};

// pass

struct DefaultCellShadeCB {
	struct vec3 entityColor;
	struct mat3 reflectionTexMatrix;
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
    struct vec3 wallColor;
    float softParticleScale;
    struct vec4 textureParam;
    struct vec3 floorColor;
    struct vec2 zRange;
    struct mat4 genTexMatrix;
    struct vec3 lightstyleColor[4];
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


#endif
