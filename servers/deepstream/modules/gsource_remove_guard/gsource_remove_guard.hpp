#ifndef GSOURCE_REMOVE_GUARD_HPP
#define GSOURCE_REMOVE_GUARD_HPP

#include <glib.h>

class GSourceRemoveGuard {
 public:
  static gboolean remove(guint tag);
};

#endif
