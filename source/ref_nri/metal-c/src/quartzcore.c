// SPDX-License-Identifier: GPL-2.0-or-later
//
// Port of rhi-zig/deps/metal/src/quartzcore.zig.

#include "metal-c/quartzcore.h"

#include "internal.h"

__MTLC_HANDLE_IMPL(ca_metal_layer);
__MTLC_HANDLE_IMPL(ca_metal_drawable);

// -- CAMetalLayer -----------------------------------------------------------

struct ca_metal_layer ca_metal_layer_new(void) {
  return ca_metal_layer_from_id(
      (void *)__mtlc_class_msg("CAMetalLayer", "layer"));
}

void ca_metal_layer_set_device(struct ca_metal_layer self,
                               struct mtlc_device device) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setDevice:"),
                       (id)device.obj);
}

void ca_metal_layer_set_pixel_format(struct ca_metal_layer self,
                                     enum mtlc_pixel_format format) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setPixelFormat:"),
                                  (mtlc_uinteger)format);
}

enum mtlc_pixel_format ca_metal_layer_pixel_format(struct ca_metal_layer self) {
  mtlc_uinteger raw =
      __MTLC_MSG(mtlc_uinteger)((id)self.obj, __MTLC_SEL("pixelFormat"));
  return (enum mtlc_pixel_format)raw;
}

void ca_metal_layer_set_drawable_size(struct ca_metal_layer self,
                                      struct cg_size size) {
  __MTLC_MSG(void, struct cg_size)((id)self.obj,
                                   __MTLC_SEL("setDrawableSize:"), size);
}

struct cg_size ca_metal_layer_drawable_size(struct ca_metal_layer self) {
  // CGSize is two doubles, so it comes back in floating-point registers on both
  // arm64 and x86_64 -- plain objc_msgSend, NOT objc_msgSend_stret. See the
  // note on __MTLC_MSG_STRET in internal.h.
  return __MTLC_MSG(struct cg_size)((id)self.obj, __MTLC_SEL("drawableSize"));
}

void ca_metal_layer_set_presents_with_transaction(struct ca_metal_layer self,
                                                   bool value) {
  __MTLC_MSG(void, __mtlc_bool)((id)self.obj,
                                __MTLC_SEL("setPresentsWithTransaction:"),
                                __MTLC_TO_BOOL(value));
}

struct ca_metal_drawable
ca_metal_layer_next_drawable(struct ca_metal_layer self) {
  return __MTLC_GET_OBJ(ca_metal_drawable, self, "nextDrawable");
}

// -- CAMetalDrawable --------------------------------------------------------

struct mtlc_texture ca_metal_drawable_texture(struct ca_metal_drawable self) {
  return __MTLC_GET_OBJ(mtlc_texture, self, "texture");
}

void *ca_metal_drawable_id(struct ca_metal_drawable self) { return self.obj; }

void ca_metal_drawable_present(struct ca_metal_drawable self) {
  __MTLC_MSG(void)((id)self.obj, __MTLC_SEL("present"));
}

// -- CATransaction ----------------------------------------------------------

void mtlc_transaction_begin(void) {
  __MTLC_MSG(void)((id)__mtlc_class("CATransaction"), __MTLC_SEL("begin"));
}

void mtlc_transaction_commit(void) {
  __MTLC_MSG(void)((id)__mtlc_class("CATransaction"), __MTLC_SEL("commit"));
}
