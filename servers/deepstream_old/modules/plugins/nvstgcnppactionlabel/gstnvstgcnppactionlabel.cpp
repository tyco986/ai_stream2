#include "gstnvstgcnppactionlabel.h"

#include <gst/video/video.h>

#include "action_label_engine.hpp"
#include "constants.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_nvstgcnppactionlabel_debug);
#define GST_CAT_DEFAULT gst_nvstgcnppactionlabel_debug

enum {
  PROP_0,
  PROP_CLASSIFIER_UNIQUE_ID,
};

#define NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVMM_CAPS));

#define gst_nvstgcnppactionlabel_parent_class parent_class
G_DEFINE_TYPE(GstNvStgcnppActionLabel, gst_nvstgcnppactionlabel, GST_TYPE_BASE_TRANSFORM);

static nvstgcnppactionlabel::ActionLabelEngine *engine_of(GstNvStgcnppActionLabel *self)
{
  return static_cast<nvstgcnppactionlabel::ActionLabelEngine *>(self->engine);
}

static void gst_nvstgcnppactionlabel_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvstgcnppactionlabel_get_property(GObject *, guint, GValue *, GParamSpec *);
static void gst_nvstgcnppactionlabel_finalize(GObject *);
static GstFlowReturn gst_nvstgcnppactionlabel_transform_ip(GstBaseTransform *, GstBuffer *);

static void
gst_nvstgcnppactionlabel_class_init(GstNvStgcnppActionLabelClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  go->set_property = GST_DEBUG_FUNCPTR(gst_nvstgcnppactionlabel_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvstgcnppactionlabel_get_property);
  go->finalize = GST_DEBUG_FUNCPTR(gst_nvstgcnppactionlabel_finalize);
  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvstgcnppactionlabel_transform_ip);

  g_object_class_install_property(
      go,
      PROP_CLASSIFIER_UNIQUE_ID,
      g_param_spec_int(
          "classifier-unique-id",
          "Classifier unique-id",
          "ST-GCN++ SGIE unique-id whose classifier meta to read",
          0,
          G_MAXINT,
          kDefaultClassifierUniqueId,
          (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge,
      "NvStgcnppActionLabel",
      "Filter/Metadata",
      "Write ST-GCN++ obj_label and action conf (NA/Unknown are 0)",
      "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(
      gst_nvstgcnppactionlabel_debug, "nvstgcnppactionlabel", 0, "nvstgcnppactionlabel");
}

static void
gst_nvstgcnppactionlabel_init(GstNvStgcnppActionLabel *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
  self->engine = new nvstgcnppactionlabel::ActionLabelEngine();
}

static void
gst_nvstgcnppactionlabel_finalize(GObject *object)
{
  GstNvStgcnppActionLabel *self = GST_NVSTGCNPPACTIONLABEL(object);
  delete engine_of(self);
  self->engine = nullptr;
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_nvstgcnppactionlabel_set_property(
    GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  nvstgcnppactionlabel::ActionLabelEngine *engine = engine_of(GST_NVSTGCNPPACTIONLABEL(object));
  switch (prop_id) {
    case PROP_CLASSIFIER_UNIQUE_ID:
      engine->set_classifier_unique_id(g_value_get_int(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
gst_nvstgcnppactionlabel_get_property(
    GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  nvstgcnppactionlabel::ActionLabelEngine *engine = engine_of(GST_NVSTGCNPPACTIONLABEL(object));
  switch (prop_id) {
    case PROP_CLASSIFIER_UNIQUE_ID:
      g_value_set_int(value, engine->classifier_unique_id());
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_nvstgcnppactionlabel_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  GstNvStgcnppActionLabel *self = GST_NVSTGCNPPACTIONLABEL(btrans);
  engine_of(self)->process_buffer(inbuf);
  return GST_FLOW_OK;
}
