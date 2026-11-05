#pragma once

#include "gstnvdetfadedrawer.h"

G_BEGIN_DECLS

typedef struct _GstNvDetFadeDrawerWithTracker GstNvDetFadeDrawerWithTracker;
typedef struct _GstNvDetFadeDrawerWithTrackerClass GstNvDetFadeDrawerWithTrackerClass;

#define GST_TYPE_NVDETFADEDRAWERWITHTRACKER (gst_nvdetfadedrawerwithtracker_get_type())
#define GST_NVDETFADEDRAWERWITHTRACKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST( \
      (obj), GST_TYPE_NVDETFADEDRAWERWITHTRACKER, GstNvDetFadeDrawerWithTracker))

struct _GstNvDetFadeDrawerWithTracker
{
  GstNvDetFadeDrawer parent;
};

struct _GstNvDetFadeDrawerWithTrackerClass
{
  GstNvDetFadeDrawerClass parent_class;
};

GType gst_nvdetfadedrawerwithtracker_get_type(void);

G_END_DECLS
