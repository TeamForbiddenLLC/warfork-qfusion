#include "r_renderer_api.h"

static void R_SetGamma_NRI( float gamma ) {
  // NOOP can't set gamma for vulkan
}
void initRendererNRI() {
  R_SetGamma = R_SetGamma_NRI;
}
