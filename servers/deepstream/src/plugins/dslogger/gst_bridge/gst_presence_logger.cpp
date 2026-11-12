#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/presence_log_engine.hpp"
#include "plugins/dslogger/gst_bridge/gst_logger_utils.hpp"
#include "plugins/dslogger/gst_bridge/gst_presence_logger.hpp"

enum {
    PROP_INTERVAL = 1,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_presence_logger_parent_class parent_class
G_DEFINE_TYPE(GstPresenceLogger, gst_presence_logger, GST_TYPE_BASE_TRANSFORM);

static PresenceLogEngine* engineOf(GstPresenceLogger* self) {
    return static_cast<PresenceLogEngine*>(self->engine);
}

static void renew(GstPresenceLogger* self) {
    delete engineOf(self);
    self->engine = new PresenceLogEngine(self->interval);
}

static void gst_presence_logger_finalize(GObject* object);
static void gst_presence_logger_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_presence_logger_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_presence_logger_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_presence_logger_class_init(GstPresenceLoggerClass* klass);
static void gst_presence_logger_init(GstPresenceLogger* self);

static GstFlowReturn gst_presence_logger_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return dsloggerTransform(transform, buffer, engineOf(GST_PRESENCE_LOGGER(transform)));
}

static void gst_presence_logger_finalize(GObject* object) {
    GstPresenceLogger* self = GST_PRESENCE_LOGGER(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_presence_logger_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstPresenceLogger* self = GST_PRESENCE_LOGGER(object);
    if (propId == PROP_INTERVAL) {
        self->interval = g_value_get_int(value);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_presence_logger_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstPresenceLogger* self = GST_PRESENCE_LOGGER(object);
    if (propId == PROP_INTERVAL) {
        g_value_set_int(value, self->interval);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_presence_logger_class_init(GstPresenceLoggerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->set_property = gst_presence_logger_set_property;
    objectClass->get_property = gst_presence_logger_get_property;
    objectClass->finalize = gst_presence_logger_finalize;
    transformClass->transform_ip = gst_presence_logger_transform_ip;
    g_object_class_install_property(
        objectClass, PROP_INTERVAL,
        g_param_spec_int("interval", "Interval", "Log period in frames", 1, G_MAXINT, dslogger::DEFAULT_INTERVAL, readWrite));
    dsloggerAddPads(elementClass);
    gst_element_class_set_details_simple(
        elementClass, "PresenceLogger", "Filter/Metadata", dslogger::ELEMENT_PRESENCE_DETAIL, dslogger::PLUGIN_PACKAGE);
}

static void gst_presence_logger_init(GstPresenceLogger* self) {
    self->interval = dslogger::DEFAULT_INTERVAL;
    self->engine = new PresenceLogEngine(self->interval);
    dsloggerPrepare(GST_BASE_TRANSFORM(self));
}
