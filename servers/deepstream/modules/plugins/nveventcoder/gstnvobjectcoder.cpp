#include "gstnvobjectcoder.h"

#include "gstnveventcoder_common.h"

GST_DEBUG_CATEGORY_STATIC(gst_nvobjectcoder_debug);

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVEVENTCODER_NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVEVENTCODER_NVMM_CAPS));

#define gst_nvobjectcoder_parent_class parent_class
G_DEFINE_TYPE(GstNvObjectCoder, gst_nvobjectcoder, GST_TYPE_BASE_TRANSFORM);

static GstFlowReturn gst_nvobjectcoder_transform_ip(GstBaseTransform *, GstBuffer *);

static void
gst_nvobjectcoder_class_init(GstNvObjectCoderClass *klass)
{
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvobjectcoder_transform_ip);

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge, "NvObjectCoder", "Filter/Metadata",
      "Placeholder object event coder", "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(gst_nvobjectcoder_debug, "nvobjectcoder", 0, "nvobjectcoder");
}

static void
gst_nvobjectcoder_init(GstNvObjectCoder *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
}

static GstFlowReturn
gst_nvobjectcoder_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  (void)btrans;
  (void)inbuf;
  return GST_FLOW_OK;
}
