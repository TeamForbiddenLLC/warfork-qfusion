#include "r_renderer.h"
#include "r_local.h"

#include <assert.h>

renderer_t renderer;

extern rserr_t initNRIRenderer( const renderer_desc_t *desc, renderer_t *rend );
//extern void printGLExtensions(renderer_t* rend);

//rserr_t RE_initRenderer(const renderer_desc_t* desc, renderer_t* renderer) {
//  assert(desc);
//  assert(renderer);
//  renderer->backend = desc->backend;
//  switch(desc->backend) {
//	  case BACKEND_OPENGL_LEGACY:
////		  return initGLRenderer( desc, renderer );
//	  case BACKEND_NRI_VULKAN:
//	  case BACKEND_NRI_METAL:
//	  case BACKEND_NRI_DX12:
//		  return initNRIRenderer( desc, renderer );
//  }
//  return rserr_unknown;
//}


//void RE_printDetails(renderer_t* r) {
//  switch(r->backend) {
//    case BACKEND_OPENGL_LEGACY:
// //     printGLExtensions(r);
//      break;
//    default:
//      break;
//  }
//
//}

//bool RE_exitRenderer(renderer_t* renderer) {
//  switch(renderer->backend) {
//    case BACKEND_OPENGL_LEGACY:
//		  QGL_Shutdown();
//      return true;
//    case BACKEND_NRI_VULKAN:
//	  case BACKEND_NRI_METAL: 
//	  case BACKEND_NRI_DX12:
//      return true;
//  }
//  return true;
//}
