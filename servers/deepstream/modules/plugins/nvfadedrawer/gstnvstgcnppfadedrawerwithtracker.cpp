#include "gstnvstgcnppfadedrawerwithtracker.h"

#include <gst/gst.h>

#include "stgcnpp_pose_fade_engine.hpp"

enum {
  PROP_CLASSIFIER_UNIQUE_ID = 50,
};

#define gst_nvstgcnppfadedrawerwithtracker_parent_class parent_class
G_DEFINE_TYPE(
    GstNvStgcnppFadeDrawerWithTracker,
    gst_nvstgcnppfadedrawerwithtracker,
    GST_TYPE_NVPOSEFADEDRAWERWITHTRACKER);

static nvfadedrawer::StgcnppPoseFadeEngine *engine_of(GstNvStgcnppFadeDrawerWithTracker *self)
{
  return static_cast<nvfadedrawer::StgcnppPoseFadeEngine *>(
      GST_NVPOSEFADEDRAWER(self)->engine);
}

static void gst_nvstgcnppfadedrawerwithtracker_set_property(
    GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvstgcnppfadedrawerwithtracker_get_property(
    GObject *, guint, GValue *, GParamSpec *);

static void
gst_nvstgcnppfadedrawerwithtracker_class_init(GstNvStgcnppFadeDrawerWithTrackerClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  go->set_property = GST_DEBUG_FUNCPTR(gst_nvstgcnppfadedrawerwithtracker_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvstgcnppfadedrawerwithtracker_get_property);
#define RW (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  g_object_class_install_property(
      go, PROP_CLASSIFIER_UNIQUE_ID,
      g_param_spec_int(
          "classifier-unique-id",
          "Classifier unique id",
          "GIE unique-id of ST-GCN++ classifier meta",
          0,
          G_MAXINT,
          4,
          RW));
#undef RW
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

static void
gst_nvstgcnppfadedrawerwithtracker_set_property(
    GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  if (prop_id == PROP_CLASSIFIER_UNIQUE_ID) {
    engine_of(GST_NVSTGCNPPFADEDRAWERWITHTRACKER(object))
        ->set_classifier_unique_id(g_value_get_int(value));
  } else {
    G_OBJECT_CLASS(parent_class)->set_property(object, prop_id, value, pspec);
  }
}

static void
gst_nvstgcnppfadedrawerwithtracker_get_property(
    GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  if (prop_id == PROP_CLASSIFIER_UNIQUE_ID) {
    g_value_set_int(
        value,
        engine_of(GST_NVSTGCNPPFADEDRAWERWITHTRACKER(object))->classifier_unique_id());
  } else {
    G_OBJECT_CLASS(parent_class)->get_property(object, prop_id, value, pspec);
  }
}
