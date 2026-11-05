#include "gstnvdetlogger.h"

#include "det_log_engine.hpp"
#include "gstnvdsmeta.h"
#include "gstnvlogger_common.h"
#include "nvds_latency_meta.h"

GST_DEBUG_CATEGORY_STATIC(gst_nvdetlogger_debug);
#define GST_CAT_DEFAULT gst_nvdetlogger_debug

enum {
  PROP_0,
  PROP_ROOT,
  PROP_INTERVAL,
};

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVLOGGER_NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVLOGGER_NVMM_CAPS));

#define gst_nvdetlogger_parent_class parent_class
G_DEFINE_TYPE(GstNvDetLogger, gst_nvdetlogger, GST_TYPE_BASE_TRANSFORM);

static nvlogger::DetLogEngine *engine_of(GstNvDetLogger *self)
{
  return static_cast<nvlogger::DetLogEngine *>(self->engine);
}

static void gst_nvdetlogger_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvdetlogger_get_property(GObject *, guint, GValue *, GParamSpec *);
static void gst_nvdetlogger_finalize(GObject *);
static GstFlowReturn gst_nvdetlogger_transform_ip(GstBaseTransform *, GstBuffer *);
static double latency_for_frame(NvDsFrameLatencyInfo *info, guint count,
                                NvDsFrameMeta *frame_meta);

static void
gst_nvdetlogger_class_init(GstNvDetLoggerClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  go->set_property = GST_DEBUG_FUNCPTR(gst_nvdetlogger_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvdetlogger_get_property);
  go->finalize = GST_DEBUG_FUNCPTR(gst_nvdetlogger_finalize);
  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvdetlogger_transform_ip);

#define RW (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  g_object_class_install_property(
      go, PROP_ROOT,
      g_param_spec_string("root", "Root", "Directory for probe_{pad}.log",
                          NVLOGGER_PLACEHOLDER_ROOT, RW));
  g_object_class_install_property(
      go, PROP_INTERVAL,
      g_param_spec_int("interval", "Interval",
                       "Log period in frames; 0 means every frame", 0, G_MAXINT, 0, RW));
#undef RW

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge, "NvDetLogger", "Filter/Metadata",
      "Write detection boxes as JSON lines", "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(gst_nvdetlogger_debug, "nvdetlogger", 0, "nvdetlogger");
}

static void
gst_nvdetlogger_init(GstNvDetLogger *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
  self->engine = new nvlogger::DetLogEngine();
}

static void
gst_nvdetlogger_finalize(GObject *object)
{
  GstNvDetLogger *self = GST_NVDETLOGGER(object);
  delete engine_of(self);
  self->engine = nullptr;
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_nvdetlogger_set_property(GObject *object, guint prop_id, const GValue *value,
                             GParamSpec *pspec)
{
  nvlogger::DetLogEngine *engine = engine_of(GST_NVDETLOGGER(object));
  switch (prop_id) {
    case PROP_ROOT:
      engine->set_root(g_value_get_string(value));
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
gst_nvdetlogger_get_property(GObject *object, guint prop_id, GValue *value,
                             GParamSpec *pspec)
{
  nvlogger::DetLogEngine *engine = engine_of(GST_NVDETLOGGER(object));
  switch (prop_id) {
    case PROP_ROOT:
      g_value_set_string(value, engine->root());
      break;
    case PROP_INTERVAL:
      g_value_set_int(value, engine->interval());
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static double
latency_for_frame(NvDsFrameLatencyInfo *info, guint count, NvDsFrameMeta *frame_meta)
{
  double latency_ms = -1.0;
  if (info != nullptr && frame_meta != nullptr) {
    for (guint index = 0; index < count; index++) {
      if (info[index].source_id == frame_meta->source_id &&
          static_cast<gint>(info[index].frame_num) == frame_meta->frame_num) {
        latency_ms = info[index].latency;
      }
    }
  }
  return latency_ms;
}

static GstFlowReturn
gst_nvdetlogger_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  GstNvDetLogger *self = GST_NVDETLOGGER(btrans);
  GstFlowReturn flow = GST_FLOW_OK;
  nvds_set_input_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  guint latency_count = 0;
  NvDsFrameLatencyInfo *latency_info = nullptr;
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(inbuf);
  if (batch_meta) {
    latency_info = static_cast<NvDsFrameLatencyInfo *>(
        g_malloc0(sizeof(NvDsFrameLatencyInfo) * batch_meta->num_frames_in_batch));
    latency_count = nvds_measure_buffer_latency(inbuf, latency_info);
    nvds_acquire_meta_lock(batch_meta);
    for (NvDsMetaList *item = batch_meta->frame_meta_list; item; item = item->next) {
      auto *frame_meta = static_cast<NvDsFrameMeta *>(item->data);
      double latency_ms = latency_for_frame(latency_info, latency_count, frame_meta);
      if (frame_meta && !engine_of(self)->process_frame(frame_meta, latency_ms)) {
        GST_ELEMENT_ERROR(self, RESOURCE, WRITE, ("nvdetlogger write failed"), (NULL));
        flow = GST_FLOW_ERROR;
        break;
      }
    }
    nvds_release_meta_lock(batch_meta);
  }
  g_free(latency_info);
  nvds_set_output_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  return flow;
}
