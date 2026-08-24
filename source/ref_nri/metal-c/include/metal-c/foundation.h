// SPDX-License-Identifier: GPL-2.0-or-later
//
// Foundation (NS) bindings -- the minimal slice the Metal layer needs: strings
// (for shader source, labels, device names), error out-parameters, and arrays.
// Mirrors the relevant parts of metal-cpp/Foundation.
//
// Port of rhi-zig/deps/metal/src/foundation.zig.

#ifndef MTLC_FOUNDATION_H
#define MTLC_FOUNDATION_H

#include "metal-c/types.h"

// NSString.
MTLC_HANDLE(ns_string);
// NSError. Typically received via an out-parameter from a failing
// *_with_error call.
MTLC_HANDLE(ns_error);
// NSArray.
MTLC_HANDLE(ns_array);

// -- NSString ---------------------------------------------------------------

// +[NSString stringWithUTF8String:] -- an autoreleased NSString from a
// NUL-terminated UTF-8 string. Never nil for a valid `text`.
MTLC_EXTERN_C struct ns_string ns_string_from_utf8(const char *text);

// +[NSString stringWithBytes:length:encoding:] with NSUTF8StringEncoding --
// an autoreleased NSString from a byte range that need not be NUL-terminated.
MTLC_EXTERN_C struct ns_string ns_string_from_utf8_slice(const char *bytes,
                                                         size_t len);

// -[NSString UTF8String] -- the contents as a NUL-terminated UTF-8 string. The
// returned pointer is owned by the NSString; copy it if it must outlive the
// receiver (or the enclosing autorelease pool).
MTLC_EXTERN_C const char *ns_string_utf8(struct ns_string self);

// -[NSString length] -- number of UTF-16 code units, NOT bytes. For a byte
// count, call strlen on ns_string_utf8.
MTLC_EXTERN_C mtlc_uinteger ns_string_length(struct ns_string self);

// -- NSError ----------------------------------------------------------------

// -[NSError localizedDescription] -- a human-readable message. Never nil.
MTLC_EXTERN_C struct ns_string
ns_error_localized_description(struct ns_error self);

// -[NSError code].
MTLC_EXTERN_C mtlc_integer ns_error_code(struct ns_error self);

// -- NSArray ----------------------------------------------------------------

// -[NSArray count].
MTLC_EXTERN_C mtlc_uinteger ns_array_count(struct ns_array self);

// -[NSArray objectAtIndex:] -- returns the raw id, since the element type is
// not known to the array. Wrap it with the matching <type>_from_id.
MTLC_EXTERN_C void *ns_array_object_at_index(struct ns_array self,
                                             mtlc_uinteger index);

#endif // MTLC_FOUNDATION_H
