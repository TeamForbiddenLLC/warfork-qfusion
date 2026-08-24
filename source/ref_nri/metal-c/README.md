# metal-c

Thin, typed **C** bindings for Metal / Foundation / QuartzCore.

This is a C port of the Zig binding fabric in `rhi-zig/deps/metal`, which in turn
replicates the curated slice of the [`metal-cpp`](https://developer.apple.com/metal/cpp/)
API surface an RHI backend actually needs.

Like `metal-cpp`, it is **pure C over the Objective-C runtime** — no `.m` files, no
Objective-C compiler, no ARC. Each class is a by-value handle wrapping an
`objc_object *`, and every function forwards to `objc_msgSend` through an
explicitly cast function pointer.

```c
#include "metal-c/metal-c.h"

struct mtlc_device device = mtlc_create_system_default_device();
if (mtlc_device_is_nil(device))
  return 1;

struct mtlc_command_queue queue = mtlc_device_new_command_queue(device);
struct mtlc_buffer buffer =
    mtlc_device_new_buffer(device, 256, MTLC_RESOURCE_STORAGE_MODE_SHARED);

memcpy(mtlc_buffer_contents(buffer), data, 256);

mtlc_buffer_release(buffer);
mtlc_command_queue_release(queue);
mtlc_device_release(device);
```

## Building

macOS only; the build fails at configure time anywhere else.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

| Option | Default | Meaning |
| --- | --- | --- |
| `METALC_BUILD_TESTS` | on when top-level | Build and register the test suite |
| `METALC_INSTALL` | on when top-level | Generate install/export rules |
| `METALC_WERROR` | `OFF` | `-Werror` |
| `BUILD_SHARED_LIBS` | `OFF` | Build a dylib instead of a static library |

## Consuming

```cmake
find_package(metal-c REQUIRED)
target_link_libraries(app PRIVATE metal::c)
```

or vendor it and `add_subdirectory(metal-c)` — the target is `metal::c` either way.

## API shape

Namespaces mirror the frameworks: `mtlc_` for Metal, `ns_` for Foundation, `ca_`
for QuartzCore. Each Objective-C method becomes a free function taking its handle
first, so `-[MTLDevice newCommandQueue]` is `mtlc_device_new_command_queue`.

Every handle type gets four operations, stamped out by `MTLC_HANDLE` in
`types.h`: `_from_id`, `_is_nil`, `_retain`, `_release`.

**nil** is `obj == NULL`. There is no optional type in C, so any function that can
return nil returns a handle you should test with `_is_nil` — the doc comment on
each function says whether it can.

**Ownership follows Cocoa**, exactly as in the Zig original:

- anything from a `*_new_*` function or a descriptor's `_init` is **yours** at
  retain count 1 — release it;
- property getters (`_name`, `_next_drawable`, `_color_attachments`, …) return
  **autoreleased** objects you do not own; `_retain` only if you need them past
  the current autorelease pool.

**Option sets** (`MTLC_RESOURCE_*`, `MTLC_TEXTURE_USAGE_*`) are pre-shifted
constants passed as a plain `mtlc_uinteger`, so they can be ORed from C++ too.
Zero is Metal's default for `mtlc_resource_options`.

## Not covered

Strict parity with the Zig bindings, so the same things are missing:

- compute and blit command encoders (`MTLComputeCommandEncoder`,
  `MTLBlitCommandEncoder`, `MTLComputePipelineState`)
- `MTLSamplerState` / `MTLSamplerDescriptor`, argument buffers
- per-attachment blend state
- instanced / base-vertex / indirect draw variants
- fences, events, timeline semaphores, completion handlers
- cull mode, front-facing winding, fill mode
- `-setFragmentBytes:length:atIndex:` and the other fragment-stage binders
- `mtlc_pixel_format` and `mtlc_vertex_format` are curated subsets

Extending is mechanical: add the declaration to the matching header, the
definition to the matching `.c`, and regenerate `tests/link_all.c`.

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).
