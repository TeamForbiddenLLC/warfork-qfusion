// SPDX-License-Identifier: GPL-2.0-or-later
//
// metal-c -- thin, typed C wrappers over the Metal / Foundation / QuartzCore
// Objective-C APIs, built directly on the Objective-C runtime (no Objective-C
// compiler required). This is a C port of the Zig binding fabric in
// rhi-zig/deps/metal, which itself replicates the curated slice of the
// metal-cpp API surface an RHI backend needs.
//
// Namespaces mirror the frameworks:
//   mtlc_ -- Metal (MTL): device, queues, buffers, textures, pipelines, ...
//   ns_   -- Foundation (NS): strings, errors, arrays.
//   ca_   -- QuartzCore (CA): CAMetalLayer, CAMetalDrawable.
// with the POD structs and enums (sizes, clear colours, pixel formats, ...) in
// types.h.
//
// Include this header for everything, or the individual headers if you prefer.

#ifndef MTLC_METAL_C_H
#define MTLC_METAL_C_H

#include "metal-c/foundation.h"
#include "metal-c/metal.h"
#include "metal-c/quartzcore.h"
#include "metal-c/types.h"

#endif // MTLC_METAL_C_H
