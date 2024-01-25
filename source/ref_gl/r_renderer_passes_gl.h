#ifndef R_RENDERER_GL_PASSES_H
#define R_RENDERER_GL_PASSES_H

typedef struct drawList_s drawList_t;

void R_DrawPortals( void );

void RPass_DrawSurfaces( drawList_t *list );
void RPass_DrawOutlinedSurfaces( drawList_t *list );

#endif
