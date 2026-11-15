#include "plugins/dslogger/gst_bridge/gst_ds_logger.hpp"

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/ds_logger.hpp"
#include "plugins/dslogger/gst_bridge/gst_logger_utils.hpp"

enum {
    PROP_INTERVAL = 1,
    PROP_PAGE,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_ds_logger_parent_class parent_class
G_DEFINE_TYPE(GstDsLogger, gst_ds_logger, GST_TYPE_BASE_TRANSFORM);

static DsLogger* engineOf(GstDsLogger* self) {
    return static_cast<DsLogger*>(self->engine);
}

static void renew(GstDsLogger* self) {
    delete engineOf(self);
    self->engine = new DsLogger(self->interval, self->page != nullptr ? self->page : "");
}

static void gst_ds_logger_finalize(GObject* object);
static void gst_ds_logger_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_ds_logger_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_ds_logger_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_ds_logger_class_init(GstDsLoggerClass* klass);
static void gst_ds_logger_init(GstDsLogger* self);

static GstFlowReturn gst_ds_logger_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return dsloggerTransform(transform, buffer, engineOf(GST_DS_LOGGER(transform)));
}

static void gst_ds_logger_finalize(GObject* object) {
    GstDsLogger* self = GST_DS_LOGGER(object);
    delete engineOf(self);
    self->engine = nullptr;
    g_free(self->page);
    self->page = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_ds_logger_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstDsLogger* self = GST_DS_LOGGER(object);
    const gchar* text = nullptr;
    if (propId == PROP_INTERVAL) {
        self->interval = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_PAGE) {
        text = g_value_get_string(value);
        g_free(self->page);
        self->page = g_strdup(text != nullptr ? text : "");
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_ds_logger_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstDsLogger* self = GST_DS_LOGGER(object);
    if (propId == PROP_INTERVAL) {
        g_value_set_int(value, self->interval);
    } else if (propId == PROP_PAGE) {
        g_value_set_string(value, self->page);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_ds_logger_class_init(GstDsLoggerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->set_property = gst_ds_logger_set_property;
    objectClass->get_property = gst_ds_logger_get_property;
    objectClass->finalize = gst_ds_logger_finalize;
    transformClass->transform_ip = gst_ds_logger_transform_ip;
    g_object_class_install_property(
        objectClass, PROP_INTERVAL,
        g_param_spec_int("interval", "Interval", "Log period in frames", 1, G_MAXINT, dslogger::DEFAULT_INTERVAL, readWrite));
    g_object_class_install_property(
        objectClass, PROP_PAGE,
        g_param_spec_string("page", "Page", "JSONL file stem", dslogger::PAGE_DET, readWrite));
    dsloggerAddPads(elementClass);
    gst_element_class_set_details_simple(
        elementClass, "DsLogger", "Filter/Metadata", dslogger::ELEMENT_DETAIL, dslogger::PLUGIN_PACKAGE);
}

static void gst_ds_logger_init(GstDsLogger* self) {
    self->interval = dslogger::DEFAULT_INTERVAL;
    self->page = g_strdup(dslogger::PAGE_DET);
    self->engine = new DsLogger(self->interval, self->page);
    dsloggerPrepare(GST_BASE_TRANSFORM(self));
}
