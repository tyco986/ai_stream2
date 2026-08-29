#include "gstnvstgcnppfadedrawerwithtracker.h"

#include <gst/gst.h>

#include "stgcnpp_pose_fade_engine.hpp"

#define gst_nvstgcnppfadedrawerwithtracker_parent_class parent_class
G_DEFINE_TYPE(
    GstNvStgcnppFadeDrawerWithTracker,
    gst_nvstgcnppfadedrawerwithtracker,
    GST_TYPE_NVPOSEFADEDRAWERWITHTRACKER);

static void
gst_nvstgcnppfadedrawerwithtracker_class_init(GstNvStgcnppFadeDrawerWithTrackerClass *klass)
{
  GstElementClass *ge = (GstElementClass *)klass;
  gst_element_class_set_details_simple(
      ge, "NvStgcnppFadeDrawerWithTracker", "Filter/Metadata",
      "Tracker pose OSD with ST-GCN++ action labels", "ai_stream2");
}

static void
gst_nvstgcnppfadedrawerwithtracker_init(GstNvStgcnppFadeDrawerWithTracker *self)
{
  GstNvPoseFadeDrawer *base = GST_NVPOSEFADEDRAWER(self);
  delete static_cast<nvfadedrawer::PoseFadeEngine *>(base->engine);
  base->engine = new nvfadedrawer::StgcnppPoseFadeEngine();
}
