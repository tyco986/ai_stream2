#include "plugins/dscollector/gst_bridge/gst_presence_collector.hpp"

#include "constants/collector_const.hpp"
#include "plugins/dscollector/engine/presence_collector_engine.hpp"
#include "plugins/dscollector/gst_bridge/gst_collector_utils.hpp"

#define gst_presence_collector_parent_class parent_class
G_DEFINE_TYPE(GstPresenceCollector, gst_presence_collector, GST_TYPE_BASE_TRANSFORM);

enum {
    PROP_DUMP_CODES = 2,
    PROP_CAPTURER_OUTPUT = 3,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

static void gst_presence_collector_finalize(GObject* object);
static void gst_presence_collector_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_presence_collector_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static void gst_presence_collector_class_init(GstPresenceCollectorClass* klass);
static void gst_presence_collector_init(GstPresenceCollector* self);

static PresenceCollectorEngine* engineOf(GObject* object) {
    return static_cast<PresenceCollectorEngine*>(GST_PRESENCE_COLLECTOR(object)->engine);
}

static void gst_presence_collector_finalize(GObject* object) {
    GstPresenceCollector* self = GST_PRESENCE_COLLECTOR(object);
    delete static_cast<PresenceCollectorEngine*>(self->engine);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_presence_collector_class_init(GstPresenceCollectorClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_presence_collector_finalize;
    collectorConfigure(objectClass, transformClass);
    objectClass->set_property = gst_presence_collector_set_property;
    objectClass->get_property = gst_presence_collector_get_property;
    g_object_class_install_property(
        objectClass,
        PROP_DUMP_CODES,
        g_param_spec_string(collector::PROP_DUMP_CODES, "Dump codes", "Event code characters that trigger a path, e.g. 1 or 12", collector::DEFAULT_DUMP_CODES, readWrite));
    g_object_class_install_property(
        objectClass,
        PROP_CAPTURER_OUTPUT,
        g_param_spec_string(collector::PROP_CAPTURER_OUTPUT, "Capturer output", "Dump directory prefixed onto raw and visualization paths", "", readWrite));
    collectorAddPads(elementClass);
    gst_element_class_set_details_simple(
        elementClass, "PresenceCollector", "Filter/Metadata", collector::ELEMENT_PRESENCE_DETAIL, collector::PLUGIN_PACKAGE);
}

static void gst_presence_collector_init(GstPresenceCollector* self) {
    self->engine = new PresenceCollectorEngine(collector::DEFAULT_INTERVAL);
    collectorPrepare(GST_BASE_TRANSFORM(self));
}

static void gst_presence_collector_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    PresenceCollectorEngine* engine = engineOf(object);
    if (propId == 1) {
        engine->setInterval(g_value_get_int(value));
    } else if (propId == PROP_DUMP_CODES) {
        engine->setDumpCodes(g_value_get_string(value));
    } else if (propId == PROP_CAPTURER_OUTPUT) {
        engine->setCapturerOutput(g_value_get_string(value));
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_presence_collector_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    PresenceCollectorEngine* engine = engineOf(object);
    if (propId == 1) {
        g_value_set_int(value, engine->getInterval());
    } else if (propId == PROP_DUMP_CODES) {
        g_value_set_string(value, engine->getDumpCodes().c_str());
    } else if (propId == PROP_CAPTURER_OUTPUT) {
        g_value_set_string(value, engine->getCapturerOutput().c_str());
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}
