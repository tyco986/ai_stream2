#include "plugins/capturer/gst_bridge/gst_raw_capturer.hpp"

#include "constants/capturer_const.hpp"
#include "plugins/capturer/engine/raw_capturer.hpp"
#include "plugins/capturer/gst_bridge/gst_capturer_utils.hpp"

enum { PROP_CAPTURE_CODES = 1 };

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_raw_capturer_parent_class parent_class
G_DEFINE_TYPE(GstRawCapturer, gst_raw_capturer, GST_TYPE_BASE_TRANSFORM);

static RawCapturer* engineOf(GstRawCapturer* self) {
    return static_cast<RawCapturer*>(self->engine);
}

static void renew(GstRawCapturer* self) {
    delete engineOf(self);
    self->engine = new RawCapturer(self->captureCodes);
}

static void gst_raw_capturer_finalize(GObject* object);
static void gst_raw_capturer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_raw_capturer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
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
    objectClass->set_property = gst_raw_capturer_set_property;
    objectClass->get_property = gst_raw_capturer_get_property;
    objectClass->finalize = gst_raw_capturer_finalize;
    transformClass->transform_ip = gst_raw_capturer_transform_ip;
    g_object_class_install_property(
        objectClass,
        PROP_CAPTURE_CODES,
        g_param_spec_string("capture-codes", "Capture codes", "Event code characters that trigger dump, e.g. 1 or 12", capturer::DEFAULT_CAPTURE_CODES, readWrite));
    capturerAddPads(elementClass);
    gst_element_class_set_details_simple(elementClass, "RawCapturer", "Filter/Video", capturer::ELEMENT_RAW_DETAIL, capturer::PLUGIN_PACKAGE);
}

static void gst_raw_capturer_init(GstRawCapturer* self) {
    capturerPrepare(GST_BASE_TRANSFORM(self));
    self->captureCodes = g_strdup(capturer::DEFAULT_CAPTURE_CODES);
    self->engine = new RawCapturer(self->captureCodes);
}

static void gst_raw_capturer_finalize(GObject* object) {
    GstRawCapturer* self = GST_RAW_CAPTURER(object);
    delete engineOf(self);
    self->engine = nullptr;
    g_free(self->captureCodes);
    self->captureCodes = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_raw_capturer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstRawCapturer* self = GST_RAW_CAPTURER(object);
    const gchar* text = nullptr;
    if (propId == PROP_CAPTURE_CODES) {
        text = g_value_get_string(value);
        g_free(self->captureCodes);
        self->captureCodes = g_strdup(text != nullptr ? text : capturer::DEFAULT_CAPTURE_CODES);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_raw_capturer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstRawCapturer* self = GST_RAW_CAPTURER(object);
    if (propId == PROP_CAPTURE_CODES) {
        g_value_set_string(value, self->captureCodes);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}
