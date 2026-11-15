#include "plugins/dscollector/gst_bridge/gst_presence_collector.hpp"

#include "constants/collector_const.hpp"
#include "plugins/dscollector/engine/presence_collector_engine.hpp"
#include "plugins/dscollector/gst_bridge/gst_collector_utils.hpp"

#define gst_presence_collector_parent_class parent_class
G_DEFINE_TYPE(GstPresenceCollector, gst_presence_collector, GST_TYPE_BASE_TRANSFORM);

static PresenceCollectorEngine* engineOf(GstPresenceCollector* self) {
    return static_cast<PresenceCollectorEngine*>(self->engine);
}

static void gst_presence_collector_finalize(GObject* object);
static GstFlowReturn gst_presence_collector_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_presence_collector_class_init(GstPresenceCollectorClass* klass);
static void gst_presence_collector_init(GstPresenceCollector* self);

static GstFlowReturn gst_presence_collector_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return collectorTransform(transform, buffer, engineOf(GST_PRESENCE_COLLECTOR(transform)));
}

static void gst_presence_collector_finalize(GObject* object) {
    GstPresenceCollector* self = GST_PRESENCE_COLLECTOR(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_presence_collector_class_init(GstPresenceCollectorClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_presence_collector_finalize;
    transformClass->transform_ip = gst_presence_collector_transform_ip;
    collectorAddPads(elementClass);
    gst_element_class_set_details_simple(
        elementClass, "PresenceCollector", "Filter/Metadata", collector::ELEMENT_PRESENCE_DETAIL, collector::PLUGIN_PACKAGE);
}

static void gst_presence_collector_init(GstPresenceCollector* self) {
    self->engine = new PresenceCollectorEngine();
    collectorPrepare(GST_BASE_TRANSFORM(self));
}
