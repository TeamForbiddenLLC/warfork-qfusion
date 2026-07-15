#ifndef RI_TYPES_H
#define RI_TYPES_H

// Umbrella header. ri_types.h no longer defines any types itself — it aggregates the per-domain
// ri_*.h component headers so existing consumers can keep including a single header. Prefer including
// the specific component header directly when you want a tighter dependency.

#include "ri_prelude.h"     // base includes, VK loader + macros, queue-flag bits, RIResult_e
#include "ri_resource.h"    // buffers, textures, views, barriers
#include "ri_descriptor.h"  // descriptors, samplers, acceleration structures
#include "ri_command.h"     // queues, pools/cmds, command ring, free list, timeline, viewport/rect
#include "ri_pipeline.h"    // pipeline / draw-state enums
#include "ri_device.h"      // renderer, physical adapter, device
#include "ri_swapchain.h"   // swapchain types + API

#endif
