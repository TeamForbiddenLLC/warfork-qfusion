#ifndef R_DEFINES_H
#define R_DEFINES_H

#include "qarch.h"

#define NUMBER_FRAMES_FLIGHT 3
#define NUMBER_SUBFRAMES_FLIGHT 64 
#define NUMBER_RESERVED_BACKBUFFERS 4
#define DESCRIPTOR_MAX_BINDINGS 32
#define MAX_COLOR_ATTACHMENTS 8 
#define MAX_VERTEX_BINDINGS 24
#define MAX_PIPELINE_ATTACHMENTS 5
#define MAX_STREAMS 8 
#define MAX_ATTRIBUTES 32
#define R_DESCRIPTOR_SET_MAX 4 

#define BINDING_SETS_PER_POOL 24

// DirectX 12 requires ubo's be aligned by 256
const static uint32_t UBOBlockerBufferSize = 256 * 128;
const static uint32_t UBOBlockerBufferAlignmentReq = 256;
#endif

