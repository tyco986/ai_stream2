#include "plugins/capturer/gst_bridge/gst_vis_capturer.hpp"

#include "constants/capturer_const.hpp"
#include "plugins/capturer/engine/vis_capturer.hpp"
#include "plugins/capturer/gst_bridge/gst_capturer_utils.hpp"

enum {
    PROP_CAPTURE_CODES = 1,
    PROP_LABEL_TASK,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_vis_capturer_parent_class parent_class
G_DEFINE_TYPE(GstVisCapturer, gst_vis_capturer, GST_TYPE_BASE_TRANSFORM);

static VisCapturer* engineOf(GstVisCapturer* self) {
    return static_cast<VisCapturer*>(self->engine);
}

static void renew(GstVisCapturer* self) {
    delete engineOf(self);
    self->engine = new VisCapturer(self->captureCodes, self->labelTask);
}

static void gst_vis_capturer_finalize(GObject* object);
static void gst_vis_capturer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_vis_capturer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_vis_capturer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_vis_capturer_class_init(GstVisCapturerClass* klass);
static void gst_vis_capturer_init(GstVisCapturer* self);

static GstFlowReturn gst_vis_capturer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return capturerTransform(transform, buffer, engineOf(GST_VIS_CAPTURER(transform)));
}

static void gst_vis_capturer_class_init(GstVisCapturerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->set_property = gst_vis_capturer_set_property;
    objectClass->get_property = gst_vis_capturer_get_property;
    objectClass->finalize = gst_vis_capturer_finalize;
    transformClass->transform_ip = gst_vis_capturer_transform_ip;
    g_object_class_install_property(
        objectClass,
        PROP_CAPTURE_CODES,
        g_param_spec_string("capture-codes", "Capture codes", "Event code characters that trigger dump, e.g. 1 or 12", capturer::DEFAULT_CAPTURE_CODES, readWrite));
    g_object_class_install_property(
        objectClass,
        PROP_LABEL_TASK,
        g_param_spec_string("label-task", "Label task", "det or seg; both write bbox labels", capturer::DEFAULT_LABEL_TASK, readWrite));
    capturerAddPads(elementClass);
    gst_element_class_set_details_simple(elementClass, "VisCapturer", "Filter/Video", capturer::ELEMENT_VIS_DETAIL, capturer::PLUGIN_PACKAGE);
}

static void gst_vis_capturer_init(GstVisCapturer* self) {
    capturerPrepare(GST_BASE_TRANSFORM(self));
    self->captureCodes = g_strdup(capturer::DEFAULT_CAPTURE_CODES);
    self->labelTask = g_strdup(capturer::DEFAULT_LABEL_TASK);
    self->engine = new VisCapturer(self->captureCodes, self->labelTask);
}

static void gst_vis_capturer_finalize(GObject* object) {
    GstVisCapturer* self = GST_VIS_CAPTURER(object);
    delete engineOf(self);
    self->engine = nullptr;
    g_free(self->captureCodes);
    g_free(self->labelTask);
    self->captureCodes = nullptr;
    self->labelTask = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_vis_capturer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstVisCapturer* self = GST_VIS_CAPTURER(object);
    const gchar* text = nullptr;
    if (propId == PROP_CAPTURE_CODES) {
        text = g_value_get_string(value);
        g_free(self->captureCodes);
        self->captureCodes = g_strdup(text != nullptr ? text : capturer::DEFAULT_CAPTURE_CODES);
        renew(self);
    } else if (propId == PROP_LABEL_TASK) {
        text = g_value_get_string(value);
        g_free(self->labelTask);
        self->labelTask = g_strdup(text != nullptr ? text : capturer::DEFAULT_LABEL_TASK);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_vis_capturer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstVisCapturer* self = GST_VIS_CAPTURER(object);
    if (propId == PROP_CAPTURE_CODES) {
        g_value_set_string(value, self->captureCodes);
    } else if (propId == PROP_LABEL_TASK) {
        g_value_set_string(value, self->labelTask);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}
