#include "gstnvpresencecoder.h"

#include <unordered_set>

#include "gstnvdsmeta.h"
#include "gstnveventcoder_common.h"
#include "nvds_latency_meta.h"
#include "nvds_presence_event_meta.h"
#include "presence_event_coder.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_nvpresencecoder_debug);
#define GST_CAT_DEFAULT gst_nvpresencecoder_debug

enum {
  PROP_0,
  PROP_CLASS_IDS,
  PROP_EVENT_NAMES,
  PROP_LENGTH,
  PROP_THRESHOLD,
  PROP_MODE,
};

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVEVENTCODER_NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVEVENTCODER_NVMM_CAPS));

#define gst_nvpresencecoder_parent_class parent_class
G_DEFINE_TYPE(GstNvPresenceCoder, gst_nvpresencecoder, GST_TYPE_BASE_TRANSFORM);

static PresenceEventCoder *engine_of(GstNvPresenceCoder *self)
{
  return static_cast<PresenceEventCoder *>(self->engine);
}

static void gst_nvpresencecoder_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvpresencecoder_get_property(GObject *, guint, GValue *, GParamSpec *);
static void gst_nvpresencecoder_finalize(GObject *);
static GstFlowReturn gst_nvpresencecoder_transform_ip(GstBaseTransform *, GstBuffer *);

static void strip_presence_meta(NvDsFrameMeta *frame_meta)
{
  NvDsMetaList *item = frame_meta->frame_user_meta_list;
  while (item != nullptr) {
    NvDsMetaList *next = item->next;
    auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
    if (user_meta != nullptr &&
        user_meta->base_meta.meta_type == NVDS_PRESENCE_EVENT_USER_META) {
      nvds_remove_user_meta_from_frame(frame_meta, user_meta);
    }
    item = next;
  }
}

static void collect_detected(NvDsFrameMeta *frame_meta, std::unordered_set<int> *detected)
{
  detected->clear();
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *object_meta = static_cast<NvDsObjectMeta *>(item->data);
    if (object_meta != nullptr) {
      detected->insert(object_meta->class_id);
    }
  }
}

static void attach_presence_meta(
    PresenceEventCoder *engine, NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  std::unordered_set<int> detected;
  collect_detected(frame_meta, &detected);
  auto *payload = static_cast<NvDsPresenceEventMeta *>(g_malloc0(sizeof(NvDsPresenceEventMeta)));
  engine->fill_meta(
      static_cast<int>(frame_meta->pad_index),
      frame_meta->bInferDone != 0,
      detected,
      payload);
  strip_presence_meta(frame_meta);
  NvDsUserMeta *user_meta = nvds_acquire_user_meta_from_pool(batch_meta);
  if (user_meta != nullptr) {
    user_meta->user_meta_data = payload;
    user_meta->base_meta.meta_type = NVDS_PRESENCE_EVENT_USER_META;
    user_meta->base_meta.copy_func = nvds_presence_event_meta_copy;
    user_meta->base_meta.release_func = nvds_presence_event_meta_release;
    nvds_add_user_meta_to_frame(frame_meta, user_meta);
  } else {
    g_free(payload);
  }
}

static void
gst_nvpresencecoder_class_init(GstNvPresenceCoderClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  go->set_property = GST_DEBUG_FUNCPTR(gst_nvpresencecoder_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvpresencecoder_get_property);
  go->finalize = GST_DEBUG_FUNCPTR(gst_nvpresencecoder_finalize);
  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvpresencecoder_transform_ip);

#define RW (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  g_object_class_install_property(
      go, PROP_CLASS_IDS,
      g_param_spec_string("class-ids", "Class ids",
                          "Semicolon-separated class ids, e.g. 0;1", "", RW));
  g_object_class_install_property(
      go, PROP_EVENT_NAMES,
      g_param_spec_string("event-names", "Event names",
                          "Semicolon-separated names aligned with class-ids", "", RW));
  g_object_class_install_property(
      go, PROP_LENGTH,
      g_param_spec_int("length", "Length", "Presence window length", 1, G_MAXINT, 10, RW));
  g_object_class_install_property(
      go, PROP_THRESHOLD,
      g_param_spec_float("threshold", "Threshold",
                         "Presence ratio threshold", 0.0f, 1.0f, 0.5f, RW));
  g_object_class_install_property(
      go, PROP_MODE,
      g_param_spec_string("mode", "Mode", "slide or fold", "fold", RW));
#undef RW

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge, "NvPresenceCoder", "Filter/Metadata",
      "Encode presence event codes into frame user meta", "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(gst_nvpresencecoder_debug, "nvpresencecoder", 0, "nvpresencecoder");
}

static void
gst_nvpresencecoder_init(GstNvPresenceCoder *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
  self->engine = new PresenceEventCoder();
}

static void
gst_nvpresencecoder_finalize(GObject *object)
{
  GstNvPresenceCoder *self = GST_NVPRESENCECODER(object);
  delete engine_of(self);
  self->engine = nullptr;
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_nvpresencecoder_set_property(GObject *object, guint prop_id, const GValue *value,
                                 GParamSpec *pspec)
{
  PresenceEventCoder *engine = engine_of(GST_NVPRESENCECODER(object));
  switch (prop_id) {
    case PROP_CLASS_IDS:
      engine->set_class_ids(g_value_get_string(value));
      break;
    case PROP_EVENT_NAMES:
      engine->set_event_names(g_value_get_string(value));
      break;
    case PROP_LENGTH:
      engine->set_length(g_value_get_int(value));
      break;
    case PROP_THRESHOLD:
      engine->set_threshold(g_value_get_float(value));
      break;
    case PROP_MODE:
      engine->set_mode(g_value_get_string(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
gst_nvpresencecoder_get_property(GObject *object, guint prop_id, GValue *value,
                                 GParamSpec *pspec)
{
  PresenceEventCoder *engine = engine_of(GST_NVPRESENCECODER(object));
  switch (prop_id) {
    case PROP_CLASS_IDS:
      g_value_set_string(value, engine->class_ids().c_str());
      break;
    case PROP_EVENT_NAMES:
      g_value_set_string(value, engine->event_names().c_str());
      break;
    case PROP_LENGTH:
      g_value_set_int(value, engine->length());
      break;
    case PROP_THRESHOLD:
      g_value_set_float(value, engine->threshold());
      break;
    case PROP_MODE:
      g_value_set_string(value, engine->mode().c_str());
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_nvpresencecoder_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  GstNvPresenceCoder *self = GST_NVPRESENCECODER(btrans);
  GstFlowReturn flow = GST_FLOW_OK;
  nvds_set_input_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(inbuf);
  if (batch_meta != nullptr) {
    nvds_acquire_meta_lock(batch_meta);
    for (NvDsMetaList *item = batch_meta->frame_meta_list; item != nullptr; item = item->next) {
      auto *frame_meta = static_cast<NvDsFrameMeta *>(item->data);
      if (frame_meta != nullptr) {
        attach_presence_meta(engine_of(self), batch_meta, frame_meta);
      }
    }
    nvds_release_meta_lock(batch_meta);
  }
  nvds_set_output_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  return flow;
}
