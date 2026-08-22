#ifndef R_DEFINES_H
#define R_DEFINES_H

#include "qarch.h"

// Matches the swapchain image count requested in RF_SetMode. A frame slot stays live until the
// present thread has finished blitting out of its backbuffer, so fewer slots than images just makes
// RF_BeginFrame decline ticks the presentation engine could already service.
#define NUMBER_FRAMES_FLIGHT 3
#define NUMBER_SUBFRAMES_FLIGHT 16

// RF_BeginFrame probes instead of blocking, so a driver or compositor that never releases a slot
// would leave the client rendering nothing forever. After this many consecutive declined ticks the
// frame blocks instead.
#define R_MAX_SKIPPED_FRAMES 100

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

