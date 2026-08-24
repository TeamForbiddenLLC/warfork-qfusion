// SPDX-License-Identifier: GPL-2.0-or-later

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>

Class __mtlc_class(const char *name) {
  Class cls = objc_getClass(name);
  if (cls == NULL) {
    fprintf(stderr, "metal-c: objc class not found: %s\n", name);
    abort();
  }
  return cls;
}

id __mtlc_alloc_init(const char *class_name) {
  Class cls = __mtlc_class(class_name);
  id allocated = __MTLC_MSG(id)((id)cls, __MTLC_SEL("alloc"));
  return __MTLC_MSG(id)(allocated, __MTLC_SEL("init"));
}

id __mtlc_class_msg(const char *class_name, const char *sel_name) {
  Class cls = __mtlc_class(class_name);
  return __MTLC_MSG(id)((id)cls, sel_registerName(sel_name));
}
