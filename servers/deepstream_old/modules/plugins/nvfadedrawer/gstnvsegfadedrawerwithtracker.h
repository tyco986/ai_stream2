#pragma once

#include "gstnvsegfadedrawer.h"

G_BEGIN_DECLS

typedef struct _GstNvSegFadeDrawerWithTracker GstNvSegFadeDrawerWithTracker;
typedef struct _GstNvSegFadeDrawerWithTrackerClass GstNvSegFadeDrawerWithTrackerClass;

#define GST_TYPE_NVSEGFADEDRAWERWITHTRACKER (gst_nvsegfadedrawerwithtracker_get_type())
#define GST_NVSEGFADEDRAWERWITHTRACKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST( \
      (obj), GST_TYPE_NVSEGFADEDRAWERWITHTRACKER, GstNvSegFadeDrawerWithTracker))

struct _GstNvSegFadeDrawerWithTracker
{
  GstNvSegFadeDrawer parent;
};

struct _GstNvSegFadeDrawerWithTrackerClass
{
  GstNvSegFadeDrawerClass parent_class;
};

GType gst_nvsegfadedrawerwithtracker_get_type(void);

G_END_DECLS
