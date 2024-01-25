#ifndef R_RENDERER_API_H
#define R_RENDERER_API_H

#include "../gameshared/q_shared.h"
#include "r_math.h"

typedef struct image_s image_t;
typedef struct refdef_s refdef_t;

DECLARE_STUB_IMPL( void, R_BindFrameBufferObject, int object );
DECLARE_STUB_IMPL( void, R_Set2DMode, bool enable );
DECLARE_STUB_IMPL( void, R_SetGamma, float gamma );
DECLARE_STUB_IMPL( void, R_Scissor, int x, int y, int w, int h );
DECLARE_STUB_IMPL( void, R_GetScissor, int *x, int *y, int *w, int *h );
DECLARE_STUB_IMPL( void, R_ResetScissor, void );
DECLARE_STUB_IMPL( void, R_DrawStretchQuick, int x, int y, int w, int h, float s1, float t1, float s2, float t2, const vec4_t color, int program_type, image_t *image, int blendMask );
DECLARE_STUB_IMPL( void, R_ClearRefInstStack, void );
DECLARE_STUB_IMPL( bool, R_PushRefInst, void );
DECLARE_STUB_IMPL( void, R_PopRefInst, void );
DECLARE_STUB_IMPL( void, R_DeferDataSync, void );
DECLARE_STUB_IMPL( void, R_DataSync, void );
DECLARE_STUB_IMPL( void, R_Finish, void );
DECLARE_STUB_IMPL( void, R_Flush, void );
DECLARE_STUB_IMPL( void, R_EndFrame, void );
DECLARE_STUB_IMPL( void, R_RenderDebugSurface, const refdef_t *fd );
DECLARE_STUB_IMPL( void, R_BeginFrame, float cameraSeparation, bool forceClear, bool forceVsync );
DECLARE_STUB_IMPL( int, R_SetSwapInterval, int swapInterval, int oldSwapInterval );
DECLARE_STUB_IMPL( void, R_RenderView, const refdef_t *fd );
DECLARE_STUB_IMPL( void, R_RenderScene, const refdef_t *fd );

extern void initRendererGL();
extern void initRendererNRI();
#endif
