// SPDX-License-Identifier: GPL-2.0-or-later
//
// Shared plumbing for the typed Objective-C wrappers. Private to the library --
// this header is not installed, so no public header has to pull in the
// Objective-C runtime.
//
// Port of rhi-zig/deps/metal/src/wrapper.zig. Where the Zig version leans on
// zig_objc's msgSend, which synthesises the right call signature at comptime,
// C has to spell each signature out by hand. That cast IS the ABI contract; get
// it wrong and the call silently reads the wrong registers.

#ifndef MTLC_INTERNAL_H
#define MTLC_INTERNAL_H

#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>

#include "metal-c/types.h"

// -- Sending messages -------------------------------------------------------

// Build a correctly-typed pointer to objc_msgSend and call through it:
//
//   __MTLC_MSG(mtlc_uinteger)((id)h.obj, __MTLC_SEL("length"))
//   __MTLC_MSG(void, id, mtlc_uinteger)((id)h.obj, sel, arg0, arg1)
//
// Never call objc_msgSend through its own declaration. Current SDKs declare it
// as `void objc_msgSend(void)` precisely to force this cast, and on arm64
// variadic arguments use a different ABI from fixed ones, so a call made
// through a variadic prototype passes them in the wrong place.
#define __MTLC_MSG(RET, ...) ((RET (*)(id, SEL, ##__VA_ARGS__))objc_msgSend)

// Same, for a struct return that the ABI passes in memory.
//
// Which one you need is target- and size-dependent. On arm64 everything,
// structs included, goes through plain objc_msgSend and this macro is an alias.
// On x86_64 a struct returned in memory -- larger than 16 bytes, or 16 bytes
// containing anything that is not two SSE-class fields -- needs
// objc_msgSend_stret instead.
//
// Nothing in the current surface needs it: the only struct-returning selector
// is -drawableSize, and CGSize is two doubles, so it comes back in xmm0/xmm1
// even on x86_64 and must use plain __MTLC_MSG. This exists so that the next
// struct return (an MTLSize at 24 bytes, say) is written correctly.
#if defined(__x86_64__)
#define __MTLC_MSG_STRET(RET, ...)                                             \
  ((RET (*)(id, SEL, ##__VA_ARGS__))objc_msgSend_stret)
#else
#define __MTLC_MSG_STRET(RET, ...)                                             \
  ((RET (*)(id, SEL, ##__VA_ARGS__))objc_msgSend)
#endif

// -- Selectors --------------------------------------------------------------

// sel_registerName is a hash lookup, so cache it per call site. The race on
// first use is benign: every racing writer stores the same interned pointer.
// Define MTLC_NO_SEL_CACHE to fall back to a plain lookup.
#ifdef MTLC_NO_SEL_CACHE
#define __MTLC_SEL(NAME) sel_registerName(NAME)
#else
#define __MTLC_SEL(NAME)                                                       \
  __extension__({                                                              \
    static SEL __mtlc_cached_sel = NULL;                                       \
    if (__builtin_expect(__mtlc_cached_sel == NULL, 0))                        \
      __mtlc_cached_sel = sel_registerName(NAME);                              \
    __mtlc_cached_sel;                                                         \
  })
#endif

// -- Classes ----------------------------------------------------------------

// Look up a registered Objective-C class by name. Aborts if it is missing --
// the system frameworks are linked, so a miss means a typo in the class name.
Class __mtlc_class(const char *name);

// alloc + init a fresh instance of `class_name`. Used for the descriptor
// objects (MTLRenderPipelineDescriptor, ...). Caller owns the result.
id __mtlc_alloc_init(const char *class_name);

// Send a zero-argument class (static) message, e.g. +[CAMetalLayer layer].
id __mtlc_class_msg(const char *class_name, const char *sel_name);

// -- BOOL -------------------------------------------------------------------

// Objective-C BOOL is a single byte, but which byte type differs by target:
// `signed char` on x86_64 macOS, `_Bool` on arm64. Both are passed and returned
// in the low byte of an integer register, so signed char is ABI-correct for
// either -- as long as we normalise to 0/1 on the way in, which _Bool requires.
typedef signed char __mtlc_bool;
#define __MTLC_TO_BOOL(v) ((__mtlc_bool)((v) ? 1 : 0))
#define __MTLC_FROM_BOOL(v) ((v) != 0)

// -- Handle boilerplate -----------------------------------------------------

// Definitions for the retain/release pair MTLC_HANDLE declares. Sending to a
// nil receiver is a defined no-op in the Objective-C runtime, so neither needs
// a NULL guard -- same as the Zig original.
#define __MTLC_HANDLE_IMPL(T)                                                  \
  void T##_retain(struct T h) {                                                \
    (void)__MTLC_MSG(id)((id)h.obj, __MTLC_SEL("retain"));                     \
  }                                                                            \
  void T##_release(struct T h) {                                               \
    __MTLC_MSG(void)((id)h.obj, __MTLC_SEL("release"));                        \
  }                                                                            \
  MTLC_STATIC_ASSERT(sizeof(struct T) == sizeof(void *),                       \
                     "handle must be a bare objc_object* view")

// Common shape: send a zero-argument selector and wrap the id result as
// `struct T`.
#define __MTLC_GET_OBJ(T, RECV, SELNAME)                                       \
  T##_from_id((void *)__MTLC_MSG(id)((id)(RECV).obj, __MTLC_SEL(SELNAME)))

// Common shape: -objectAtIndexedSubscript: on a descriptor array.
#define __MTLC_GET_INDEXED(T, RECV, INDEX)                                     \
  T##_from_id((void *)__MTLC_MSG(id, mtlc_uinteger)(                           \
      (id)(RECV).obj, __MTLC_SEL("objectAtIndexedSubscript:"), (INDEX)))

#endif // MTLC_INTERNAL_H
