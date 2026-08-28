#include "gstnvrawcapturer.h"

#include "capture_core.hpp"
#include "gstnvdsmeta.h"
#include "gstnvcapturer_common.h"
#include "nvbufsurface.h"
#include "nvds_latency_meta.h"

GST_DEBUG_CATEGORY_STATIC(gst_nvrawcapturer_debug);
#define GST_CAT_DEFAULT gst_nvrawcapturer_debug

enum {
  PROP_0,
  PROP_OUTPUT_DIR,
  PROP_CAPTURE_CODES,
  PROP_INTERVAL,
};

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVCAPTURER_NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVCAPTURER_NVMM_CAPS));

#define gst_nvrawcapturer_parent_class parent_class
G_DEFINE_TYPE(GstNvRawCapturer, gst_nvrawcapturer, GST_TYPE_BASE_TRANSFORM);

static CaptureCore *engine_of(GstNvRawCapturer *self)
{
  return static_cast<CaptureCore *>(self->engine);
}

static void gst_nvrawcapturer_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvrawcapturer_get_property(GObject *, guint, GValue *, GParamSpec *);
static void gst_nvrawcapturer_finalize(GObject *);
static GstFlowReturn gst_nvrawcapturer_transform_ip(GstBaseTransform *, GstBuffer *);

static void
gst_nvrawcapturer_class_init(GstNvRawCapturerClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  go->set_property = GST_DEBUG_FUNCPTR(gst_nvrawcapturer_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvrawcapturer_get_property);
  go->finalize = GST_DEBUG_FUNCPTR(gst_nvrawcapturer_finalize);
  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvrawcapturer_transform_ip);

#define RW (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  g_object_class_install_property(
      go, PROP_OUTPUT_DIR,
      g_param_spec_string("output-dir", "Output dir",
                          "Root directory for images/", NVCAPTURER_DEFAULT_OUTPUT_DIR, RW));
  g_object_class_install_property(
      go, PROP_CAPTURE_CODES,
      g_param_spec_string("capture-codes", "Capture codes",
                          "Event code characters that trigger dump, e.g. 1",
                          NVCAPTURER_DEFAULT_CAPTURE_CODES, RW));
  g_object_class_install_property(
      go, PROP_INTERVAL,
      g_param_spec_int("interval", "Interval",
                       "Inference period; 0 means every frame", 0, G_MAXINT, 0, RW));
#undef RW

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge, "NvRawCapturer", "Filter/Video",
      "Dump raw RGB frames as PNG on presence event codes", "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(gst_nvrawcapturer_debug, "nvrawcapturer", 0, "nvrawcapturer");
}

static void
gst_nvrawcapturer_init(GstNvRawCapturer *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
  self->engine = new CaptureCore();
}

static void
gst_nvrawcapturer_finalize(GObject *object)
{
  GstNvRawCapturer *self = GST_NVRAWCAPTURER(object);
  delete engine_of(self);
  self->engine = nullptr;
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_nvrawcapturer_set_property(GObject *object, guint prop_id, const GValue *value,
                              GParamSpec *pspec)
{
  CaptureCore *engine = engine_of(GST_NVRAWCAPTURER(object));
  switch (prop_id) {
    case PROP_OUTPUT_DIR:
      engine->set_output_dir(g_value_get_string(value));
      break;
    case PROP_CAPTURE_CODES:
      engine->set_capture_codes(g_value_get_string(value));
      break;
    case PROP_INTERVAL:
      engine->set_interval(g_value_get_int(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
gst_nvrawcapturer_get_property(GObject *object, guint prop_id, GValue *value,
                              GParamSpec *pspec)
{
  CaptureCore *engine = engine_of(GST_NVRAWCAPTURER(object));
  switch (prop_id) {
    case PROP_OUTPUT_DIR:
      g_value_set_string(value, engine->output_dir().c_str());
      break;
    case PROP_CAPTURE_CODES:
      g_value_set_string(value, engine->capture_codes().c_str());
      break;
    case PROP_INTERVAL:
      g_value_set_int(value, engine->interval());
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_nvrawcapturer_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  GstNvRawCapturer *self = GST_NVRAWCAPTURER(btrans);
  GstFlowReturn flow = GST_FLOW_OK;
  GstMapInfo map;
  nvds_set_input_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  if (gst_buffer_map(inbuf, &map, GST_MAP_READ)) {
    auto *surface = reinterpret_cast<NvBufSurface *>(map.data);
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(inbuf);
    if (batch_meta != nullptr) {
      nvds_acquire_meta_lock(batch_meta);
      for (NvDsMetaList *item = batch_meta->frame_meta_list; item != nullptr; item = item->next) {
        auto *frame_meta = static_cast<NvDsFrameMeta *>(item->data);
        if (frame_meta != nullptr && !engine_of(self)->dump_raw(surface, frame_meta)) {
          GST_ELEMENT_ERROR(self, RESOURCE, WRITE, ("nvrawcapturer write failed"), (NULL));
          flow = GST_FLOW_ERROR;
          break;
        }
      }
      nvds_release_meta_lock(batch_meta);
    }
    gst_buffer_unmap(inbuf, &map);
  }
  nvds_set_output_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  return flow;
}
