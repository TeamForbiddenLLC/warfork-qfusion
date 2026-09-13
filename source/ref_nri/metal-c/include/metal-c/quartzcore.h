// SPDX-License-Identifier: GPL-2.0-or-later
//
// QuartzCore (CA) bindings -- CAMetalLayer and CAMetalDrawable, the bridge
// between a window surface and Metal. Mirrors metal-cpp/QuartzCore.
//
// Port of rhi-zig/deps/metal/src/quartzcore.zig.

#ifndef MTLC_QUARTZCORE_H
#define MTLC_QUARTZCORE_H

#include "metal-c/metal.h"
#include "metal-c/types.h"

// CAMetalLayer.
MTLC_HANDLE(ca_metal_layer);
// CAMetalDrawable (conforms to MTLDrawable).
MTLC_HANDLE(ca_metal_drawable);

// -- CAMetalLayer -----------------------------------------------------------

// +[CAMetalLayer layer] -- a fresh autoreleased layer. Never nil.
MTLC_EXTERN_C struct ca_metal_layer ca_metal_layer_new(void);

// -[CAMetalLayer setDevice:] -- the Metal device the layer draws with.
MTLC_EXTERN_C void ca_metal_layer_set_device(struct ca_metal_layer self,
                                             struct mtlc_device device);

// -[CAMetalLayer setPixelFormat:].
MTLC_EXTERN_C void
ca_metal_layer_set_pixel_format(struct ca_metal_layer self,
                                enum mtlc_pixel_format format);

// -[CAMetalLayer pixelFormat].
MTLC_EXTERN_C enum mtlc_pixel_format
ca_metal_layer_pixel_format(struct ca_metal_layer self);

// -[CAMetalLayer setDrawableSize:].
MTLC_EXTERN_C void ca_metal_layer_set_drawable_size(struct ca_metal_layer self,
                                                    struct cg_size size);

// -[CAMetalLayer drawableSize].
MTLC_EXTERN_C struct cg_size
ca_metal_layer_drawable_size(struct ca_metal_layer self);

// -[CAMetalLayer setPresentsWithTransaction:] -- when true, presenting a
// drawable is committed synchronously within a CATransaction on the calling
// thread rather than scheduled by the render server. Required to recycle
// drawables in an app that does not drive presentation from the Cocoa run loop.
MTLC_EXTERN_C void
ca_metal_layer_set_presents_with_transaction(struct ca_metal_layer self,
                                             bool value);

// -[CAMetalLayer nextDrawable] (autoreleased). Nil if no drawable became
// available before the layer's internal timeout.
MTLC_EXTERN_C struct ca_metal_drawable
ca_metal_layer_next_drawable(struct ca_metal_layer self);

// -- CAMetalDrawable --------------------------------------------------------

// -[CAMetalDrawable texture] -- the renderable texture backing this drawable.
// Never nil.
MTLC_EXTERN_C struct mtlc_texture
ca_metal_drawable_texture(struct ca_metal_drawable self);

// The underlying id, to hand to mtlc_command_buffer_present_drawable.
MTLC_EXTERN_C void *ca_metal_drawable_id(struct ca_metal_drawable self);

// -[CAMetalDrawable present] -- present the drawable on the calling thread.
// With setPresentsWithTransaction:YES this must be paired with a surrounding
// CATransaction (see mtlc_transaction_begin/commit) after the command buffer
// that renders into it has been committed and scheduled.
MTLC_EXTERN_C void ca_metal_drawable_present(struct ca_metal_drawable self);

// -- CATransaction ----------------------------------------------------------

// +[CATransaction begin] / +[CATransaction commit] -- an explicit transaction
// scope. Needed to flush a presentsWithTransaction present without relying on
// the Cocoa run loop's implicit transaction.
MTLC_EXTERN_C void mtlc_transaction_begin(void);
MTLC_EXTERN_C void mtlc_transaction_commit(void);

#endif // MTLC_QUARTZCORE_H
