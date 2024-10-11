/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2002-2013 Victor Luchits

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
#ifndef R_RESOURCE_UPLOAD_H
#define R_RESOURCE_UPLOAD_H

#include "../gameshared/q_shared.h"
#include "r_nri.h"


static const uint32_t SizeOfStageBufferByte = 8 * MB_TO_BYTE;
static const NriAccessLayoutStage ResourceUploadTexturePostAccess = { 
	.layout = NriLayout_COPY_DESTINATION, 
	.access = NriAccessBits_COPY_DESTINATION, 
	.stages = NriStageBits_COPY 
};
static const NriAccessStage ResourceUploadBufferPostAccess = { 
	.access = NriLayout_COPY_DESTINATION, 
	.stages = NriStageBits_COPY 
};

typedef struct {
	NriBuffer *target;
	NriAccessStage before;
	NriAccessStage after; 

	size_t numBytes;
	size_t byteOffset;

	// begin mapping
	void *data;

	struct {
		NriBuffer *backing;
		size_t byteOffset;
	} internal;
} buffer_upload_desc_t;

typedef struct {
	NriTexture *target;
	NriAccessLayoutStage before;
	NriAccessLayoutStage after;

	// https://github.com/microsoft/DirectXTex/wiki/Image
	uint32_t sliceNum;
	uint32_t rowPitch;

	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint32_t width;
	uint32_t height;

	uint32_t arrayOffset;
	uint32_t mipOffset;

	// begin mapping
	void *data;
	uint32_t alignRowPitch;
	uint32_t alignSlicePitch;

	struct {
		NriBuffer *backing;
		uint64_t byteOffset;
	} internal;

} texture_upload_desc_t;

void R_InitResourceUpload();
void R_ExitResourceUpload();

// buffer upload
// NriAccessStage R_ResourceTransitionBuffer( NriBuffer *buffer, NriAccessStage currentAccessAndLayout );
void R_ResourceBeginCopyBuffer( buffer_upload_desc_t *action );
void R_ResourceEndCopyBuffer( buffer_upload_desc_t *action );

// texture upload
// void R_ResourceTransitionTexture( NriTexture *texture, NriAccessLayoutStage currentAccessAndLayout );
void R_ResourceBeginCopyTexture( texture_upload_desc_t *desc );
void R_ResourceEndCopyTexture( texture_upload_desc_t *desc );
void R_ResourceSubmit();

void R_FlushTransitionBarrier( NriCommandBuffer *cmd );

#endif
