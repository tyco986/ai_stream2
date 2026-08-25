#include "gstnvsegfadedrawer.h"

#include "gstnvfadedrawer_common.h"
#include "gstnvdsmeta.h"
#include "nvds_latency_meta.h"
#include "seg_fade_engine.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_nvsegfadedrawer_debug);
#define GST_CAT_DEFAULT gst_nvsegfadedrawer_debug

enum {
  PROP_0,
  PROP_INTERVAL,
  PROP_FADE_TIME,
  PROP_SHOW_LABEL,
  PROP_SHOW_MASK,
};

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVFADEDRAWER_NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVFADEDRAWER_NVMM_CAPS));

#define gst_nvsegfadedrawer_parent_class parent_class
G_DEFINE_TYPE(GstNvSegFadeDrawer, gst_nvsegfadedrawer, GST_TYPE_BASE_TRANSFORM);

static nvfadedrawer::SegFadeEngine *engine_of(GstNvSegFadeDrawer *self)
{
  return static_cast<nvfadedrawer::SegFadeEngine *>(self->engine);
}

static void gst_nvsegfadedrawer_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvsegfadedrawer_get_property(GObject *, guint, GValue *, GParamSpec *);
static void gst_nvsegfadedrawer_finalize(GObject *);
static GstFlowReturn gst_nvsegfadedrawer_transform_ip(GstBaseTransform *, GstBuffer *);

static void
gst_nvsegfadedrawer_class_init(GstNvSegFadeDrawerClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  go->set_property = GST_DEBUG_FUNCPTR(gst_nvsegfadedrawer_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvsegfadedrawer_get_property);
  go->finalize = GST_DEBUG_FUNCPTR(gst_nvsegfadedrawer_finalize);
  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvsegfadedrawer_transform_ip);

#define RW (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  g_object_class_install_property(
      go, PROP_INTERVAL,
      g_param_spec_int("interval", "Interval",
                       "Inference period; 0 means every frame", 0, G_MAXINT, 0, RW));
  g_object_class_install_property(
      go, PROP_FADE_TIME,
      g_param_spec_int("fade-time", "Fade time",
                       "Fade LUT repeats; 0 disables fade", 0, G_MAXINT, 0, RW));
  g_object_class_install_property(
      go, PROP_SHOW_LABEL,
      g_param_spec_boolean("show-label", "Show label",
                           "Show {label}|{conf}|{id} text", FALSE, RW));
  g_object_class_install_property(
      go, PROP_SHOW_MASK,
      g_param_spec_boolean("show-mask", "Show mask",
                           "Draw instance masks", TRUE, RW));
#undef RW

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge, "NvSegFadeDrawer", "Filter/Metadata",
      "Fade detection boxes and optional instance masks", "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(gst_nvsegfadedrawer_debug, "nvsegfadedrawer", 0,
                          "nvsegfadedrawer");
}

static void
gst_nvsegfadedrawer_init(GstNvSegFadeDrawer *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
  self->engine = new nvfadedrawer::SegFadeEngine();
}

static void
gst_nvsegfadedrawer_finalize(GObject *object)
{
  GstNvSegFadeDrawer *self = GST_NVSEGFADEDRAWER(object);
  delete engine_of(self);
  self->engine = nullptr;
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_nvsegfadedrawer_set_property(GObject *object, guint prop_id, const GValue *value,
                                 GParamSpec *pspec)
{
  nvfadedrawer::SegFadeEngine *engine = engine_of(GST_NVSEGFADEDRAWER(object));
  switch (prop_id) {
    case PROP_INTERVAL:
      engine->set_interval(g_value_get_int(value));
      break;
    case PROP_FADE_TIME:
      engine->set_fade_time(g_value_get_int(value));
      break;
    case PROP_SHOW_LABEL:
      engine->set_show_label(g_value_get_boolean(value));
      break;
    case PROP_SHOW_MASK:
      engine->set_show_mask(g_value_get_boolean(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
gst_nvsegfadedrawer_get_property(GObject *object, guint prop_id, GValue *value,
                                 GParamSpec *pspec)
{
  nvfadedrawer::SegFadeEngine *engine = engine_of(GST_NVSEGFADEDRAWER(object));
  switch (prop_id) {
    case PROP_INTERVAL:
      g_value_set_int(value, engine->interval());
      break;
    case PROP_FADE_TIME:
      g_value_set_int(value, engine->fade_time());
      break;
    case PROP_SHOW_LABEL:
      g_value_set_boolean(value, engine->show_label());
      break;
    case PROP_SHOW_MASK:
      g_value_set_boolean(value, engine->show_mask());
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_nvsegfadedrawer_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  GstNvSegFadeDrawer *self = GST_NVSEGFADEDRAWER(btrans);
  GstFlowReturn flow = GST_FLOW_OK;
  nvds_set_input_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(inbuf);
  if (batch_meta) {
    nvds_acquire_meta_lock(batch_meta);
    for (NvDsMetaList *item = batch_meta->frame_meta_list; item; item = item->next) {
      auto *frame_meta = static_cast<NvDsFrameMeta *>(item->data);
      if (frame_meta) {
        engine_of(self)->process_frame(batch_meta, frame_meta);
      }
    }
    nvds_release_meta_lock(batch_meta);
  }
  nvds_set_output_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  return flow;
}
