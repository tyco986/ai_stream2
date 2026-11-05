#pragma once

#include "gstnvposefadedrawerwithtracker.h"

G_BEGIN_DECLS

typedef struct _GstNvStgcnppFadeDrawerWithTracker GstNvStgcnppFadeDrawerWithTracker;
typedef struct _GstNvStgcnppFadeDrawerWithTrackerClass GstNvStgcnppFadeDrawerWithTrackerClass;

#define GST_TYPE_NVSTGCNPPFADEDRAWERWITHTRACKER (gst_nvstgcnppfadedrawerwithtracker_get_type())
#define GST_NVSTGCNPPFADEDRAWERWITHTRACKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST( \
      (obj), GST_TYPE_NVSTGCNPPFADEDRAWERWITHTRACKER, GstNvStgcnppFadeDrawerWithTracker))

struct _GstNvStgcnppFadeDrawerWithTracker
{
  GstNvPoseFadeDrawerWithTracker parent;
};

struct _GstNvStgcnppFadeDrawerWithTrackerClass
{
  GstNvPoseFadeDrawerWithTrackerClass parent_class;
};

GType gst_nvstgcnppfadedrawerwithtracker_get_type(void);

G_END_DECLS
