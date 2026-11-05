#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "gsource_remove_guard.hpp"

#include <dlfcn.h>

gboolean GSourceRemoveGuard::remove(guint tag) {
  using Remove = gboolean (*)(guint);
  static Remove real_remove = reinterpret_cast<Remove>(dlsym(RTLD_NEXT, "g_source_remove"));
  gboolean removed = FALSE;
  if (g_main_context_find_source_by_id(nullptr, tag) != nullptr) {
    removed = real_remove(tag);
  }
  return removed;
}

extern "C" gboolean g_source_remove(guint tag) { return GSourceRemoveGuard::remove(tag); }
