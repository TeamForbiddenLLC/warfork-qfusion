#ifndef R_NRI_IMP_H
#define R_NRI_IMP_H

#define NRI_STATIC_LIBRARY 1
#include "qarch.h"

#include "r_texture_format.h"
#include "r_vattribs.h"

#include "r_graphics.h"

// DirectX 12 requires ubo's be aligned by 256
const static uint32_t UBOBlockerBufferSize = 256 * 128;
const static uint32_t UBOBlockerBufferAlignmentReq = 256;

#define NUMBER_FRAMES_FLIGHT 3
#define NUMBER_SUBFRAMES_FLIGHT 64 
#define NUMBER_RESERVED_BACKBUFFERS 4
#define DESCRIPTOR_MAX_BINDINGS 32
#define MAX_COLOR_ATTACHMENTS 8 
#define MAX_VERTEX_BINDINGS 24
#define MAX_PIPELINE_ATTACHMENTS 5
#define MAX_STREAMS 8 
#define MAX_ATTRIBUTES 32

#define BINDING_SETS_PER_POOL 24

enum descriptor_set_e { 
	DESCRIPTOR_SET_0, 
	DESCRIPTOR_SET_1, 
	DESCRIPTOR_SET_2, 
	DESCRIPTOR_SET_3, 
	DESCRIPTOR_SET_MAX 
};

#define R_VK_ABORT_ON_FAILURE( result )             \
	if( result != VK_SUCCESS ) {                    \
		Com_Printf( "invalid result: %d", result ); \
		exit( 0 );                                  \
	}

#define NRI_ABORT_ON_FAILURE( result )  \
	if( result != NriResult_SUCCESS ) { \
		exit( 0 );                      \
	}

// a wrapper to hold onto the hash + cookie
struct nri_descriptor_s {
	//NriDescriptor *descriptor;
	uint32_t cookie;
};

#endif
