#include "plugins/capturer/gst_bridge/gst_raw_capturer.hpp"

#include "constants/capturer_const.hpp"
#include "plugins/capturer/engine/raw_capturer.hpp"
#include "plugins/capturer/gst_bridge/gst_capturer_utils.hpp"

#define gst_raw_capturer_parent_class parent_class
G_DEFINE_TYPE(GstRawCapturer, gst_raw_capturer, GST_TYPE_BASE_TRANSFORM);

static RawCapturer* engineOf(GstRawCapturer* self) {
    return static_cast<RawCapturer*>(self->engine);
}

static void gst_raw_capturer_finalize(GObject* object);
static GstFlowReturn gst_raw_capturer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_raw_capturer_class_init(GstRawCapturerClass* klass);
static void gst_raw_capturer_init(GstRawCapturer* self);

static GstFlowReturn gst_raw_capturer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return capturerTransform(transform, buffer, engineOf(GST_RAW_CAPTURER(transform)));
}

static void gst_raw_capturer_class_init(GstRawCapturerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_raw_capturer_finalize;
    transformClass->transform_ip = gst_raw_capturer_transform_ip;
    capturerAddPads(elementClass);
    gst_element_class_set_details_simple(elementClass, "RawCapturer", "Filter/Video", capturer::ELEMENT_RAW_DETAIL, capturer::PLUGIN_PACKAGE);
}

static void gst_raw_capturer_init(GstRawCapturer* self) {
    capturerPrepare(GST_BASE_TRANSFORM(self));
    self->engine = new RawCapturer();
}

static void gst_raw_capturer_finalize(GObject* object) {
    GstRawCapturer* self = GST_RAW_CAPTURER(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}
