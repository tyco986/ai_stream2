#include "gstnvsegfadedrawerwithtracker.h"

#include <gst/gst.h>

#include "seg_fade_engine.hpp"

enum {
  PROP_SHOW_SNAP = 100,
};

#define gst_nvsegfadedrawerwithtracker_parent_class parent_class
G_DEFINE_TYPE(GstNvSegFadeDrawerWithTracker, gst_nvsegfadedrawerwithtracker,
              GST_TYPE_NVSEGFADEDRAWER);

static nvfadedrawer::SegFadeEngine *engine_of(GstNvSegFadeDrawerWithTracker *self)
{
  return static_cast<nvfadedrawer::SegFadeEngine *>(GST_NVSEGFADEDRAWER(self)->engine);
}

static void gst_nvsegfadedrawerwithtracker_set_property(
    GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvsegfadedrawerwithtracker_get_property(
    GObject *, guint, GValue *, GParamSpec *);

static void
gst_nvsegfadedrawerwithtracker_class_init(GstNvSegFadeDrawerWithTrackerClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  go->set_property = GST_DEBUG_FUNCPTR(gst_nvsegfadedrawerwithtracker_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvsegfadedrawerwithtracker_get_property);
#define RW (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  g_object_class_install_property(
      go, PROP_SHOW_SNAP,
      g_param_spec_boolean("show-snap", "Show snap",
                           "Inject snapshot boxes with fade; false draws tracker boxes only",
                           TRUE, RW));
#undef RW
  gst_element_class_set_details_simple(
      ge, "NvSegFadeDrawerWithTracker", "Filter/Metadata",
      "Tracker seg OSD with optional snapshot fade boxes", "ai_stream2");
}

static void
gst_nvsegfadedrawerwithtracker_init(GstNvSegFadeDrawerWithTracker *self)
{
  GstNvSegFadeDrawer *base = GST_NVSEGFADEDRAWER(self);
  delete static_cast<nvfadedrawer::SegFadeEngine *>(base->engine);
  base->engine = new nvfadedrawer::SegFadeEngineWithTracker();
}

static void
gst_nvsegfadedrawerwithtracker_set_property(GObject *object, guint prop_id, const GValue *value,
                                            GParamSpec *pspec)
{
  if (prop_id == PROP_SHOW_SNAP) {
    engine_of(GST_NVSEGFADEDRAWERWITHTRACKER(object))->set_show_snap(g_value_get_boolean(value));
  } else {
    G_OBJECT_CLASS(parent_class)->set_property(object, prop_id, value, pspec);
  }
}

static void
gst_nvsegfadedrawerwithtracker_get_property(GObject *object, guint prop_id, GValue *value,
                                            GParamSpec *pspec)
{
  if (prop_id == PROP_SHOW_SNAP) {
    g_value_set_boolean(value, engine_of(GST_NVSEGFADEDRAWERWITHTRACKER(object))->show_snap());
  } else {
    G_OBJECT_CLASS(parent_class)->get_property(object, prop_id, value, pspec);
  }
}
