// SPDX-License-Identifier: GPL-2.0-or-later
//
// Port of rhi-zig/deps/metal/src/foundation.zig.

#include "metal-c/foundation.h"

#include "internal.h"

__MTLC_HANDLE_IMPL(ns_string);
__MTLC_HANDLE_IMPL(ns_error);
__MTLC_HANDLE_IMPL(ns_array);

// NSUTF8StringEncoding.
#define MTLC_NS_UTF8_STRING_ENCODING ((mtlc_uinteger)4)

// -- NSString ---------------------------------------------------------------

struct ns_string ns_string_from_utf8(const char *text) {
  id cls = (id)__mtlc_class("NSString");
  id str = __MTLC_MSG(id, const char *)(
      cls, __MTLC_SEL("stringWithUTF8String:"), text);
  return ns_string_from_id((void *)str);
}

struct ns_string ns_string_from_utf8_slice(const char *bytes, size_t len) {
  id cls = (id)__mtlc_class("NSString");
  id str = __MTLC_MSG(id, const void *, mtlc_uinteger, mtlc_uinteger)(
      cls, __MTLC_SEL("stringWithBytes:length:encoding:"), (const void *)bytes,
      (mtlc_uinteger)len, MTLC_NS_UTF8_STRING_ENCODING);
  return ns_string_from_id((void *)str);
}

const char *ns_string_utf8(struct ns_string self) {
  return __MTLC_MSG(const char *)((id)self.obj, __MTLC_SEL("UTF8String"));
}

mtlc_uinteger ns_string_length(struct ns_string self) {
  return __MTLC_MSG(mtlc_uinteger)((id)self.obj, __MTLC_SEL("length"));
}

// -- NSError ----------------------------------------------------------------

struct ns_string ns_error_localized_description(struct ns_error self) {
  return __MTLC_GET_OBJ(ns_string, self, "localizedDescription");
}

mtlc_integer ns_error_code(struct ns_error self) {
  return __MTLC_MSG(mtlc_integer)((id)self.obj, __MTLC_SEL("code"));
}

// -- NSArray ----------------------------------------------------------------

mtlc_uinteger ns_array_count(struct ns_array self) {
  return __MTLC_MSG(mtlc_uinteger)((id)self.obj, __MTLC_SEL("count"));
}

void *ns_array_object_at_index(struct ns_array self, mtlc_uinteger index) {
  return (void *)__MTLC_MSG(id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("objectAtIndex:"), index);
}
