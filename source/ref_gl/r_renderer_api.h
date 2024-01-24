#ifndef R_RENDERER_API_H
#define R_RENDERER_API_H

#include "../gameshared/q_shared.h"
#include "r_math.h"

typedef struct image_s image_t;

DECLARE_STUB_IMPL( void, R_BindFrameBufferObject, int object );
DECLARE_STUB_IMPL( void, R_Set2DMode, bool enable ); 
DECLARE_STUB_IMPL( void, R_SetGamma, float gamma );
DECLARE_STUB_IMPL( void, R_Scissor, int x, int y, int w, int h );
DECLARE_STUB_IMPL( void, R_GetScissor, int *x, int *y, int *w, int *h );
DECLARE_STUB_IMPL( void, R_ResetScissor, void );
DECLARE_STUB_IMPL( void, R_DrawStretchQuick, int x, int y, int w, int h, float s1, float t1, float s2, float t2, const vec4_t color, int program_type, image_t *image, int blendMask );

extern void initRendererGL();
extern void initRendererNRI();
#endif
