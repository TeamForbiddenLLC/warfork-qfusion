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

struct ObjectCB {
	struct vec4 fogEyePlane;
	struct vec4 fogPlane;
	struct vec3 entityOrigin;
	float entityDist;
	struct mat4 mvp;
	struct mat4 mv;
	float blendMix;
};

// pass

struct DefaultCellShadeCB {
	struct vec4 textureMatrix[2];
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
    struct vec4 textureMatrix[2];
    float frontPlane;
};

struct DefaultQ3ShaderCB {
    struct vec3 wallColor;
    float softParticleScale;
    struct vec4 textureParam;
    struct vec4 textureMatrix[2];
    struct vec3 floorColor;
    struct vec2 zRange;
    struct mat4 genTexMatrix;
    struct vec3 lightstyleColor[4];
};

struct DefaultMaterialCB {
    struct vec4 entityColor;
    struct vec4 textureMatrix[2];
    struct vec3 lightstyleColor[4];
    struct vec4 deluxLightMapScale;
    float offsetScale;
    struct vec3 lightDir;
};


#endif
