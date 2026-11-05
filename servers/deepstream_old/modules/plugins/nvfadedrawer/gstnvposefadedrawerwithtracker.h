#pragma once

#include "gstnvposefadedrawer.h"

G_BEGIN_DECLS

typedef struct _GstNvPoseFadeDrawerWithTracker GstNvPoseFadeDrawerWithTracker;
typedef struct _GstNvPoseFadeDrawerWithTrackerClass GstNvPoseFadeDrawerWithTrackerClass;

#define GST_TYPE_NVPOSEFADEDRAWERWITHTRACKER (gst_nvposefadedrawerwithtracker_get_type())
#define GST_NVPOSEFADEDRAWERWITHTRACKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST( \
      (obj), GST_TYPE_NVPOSEFADEDRAWERWITHTRACKER, GstNvPoseFadeDrawerWithTracker))

struct _GstNvPoseFadeDrawerWithTracker
{
  GstNvPoseFadeDrawer parent;
};

struct _GstNvPoseFadeDrawerWithTrackerClass
{
  GstNvPoseFadeDrawerClass parent_class;
};

GType gst_nvposefadedrawerwithtracker_get_type(void);

G_END_DECLS
