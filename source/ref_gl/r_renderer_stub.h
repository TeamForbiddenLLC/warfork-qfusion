#ifndef R_RENDERER_STUB_H
#define R_RENDERER_STUB_H

#include "../gameshared/q_shared.h"
#include "r_math.h"

typedef struct image_s image_t;
typedef struct refdef_s refdef_t;

DECLARE_STUB_IMPL( void, R_EndFrame, void );

// stub interface for the renderering logic 
extern void initRendererGL();
#endif
