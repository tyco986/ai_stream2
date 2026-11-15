#include "plugins/dscollector/gst_bridge/gst_det_collector.hpp"

#include "constants/collector_const.hpp"
#include "plugins/dscollector/engine/collector_engine.hpp"
#include "plugins/dscollector/gst_bridge/gst_collector_utils.hpp"

#define gst_det_collector_parent_class parent_class
G_DEFINE_TYPE(GstDetCollector, gst_det_collector, GST_TYPE_BASE_TRANSFORM);

static CollectorEngine* engineOf(GstDetCollector* self) {
    return static_cast<CollectorEngine*>(self->engine);
}

static void gst_det_collector_finalize(GObject* object);
static GstFlowReturn gst_det_collector_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_det_collector_class_init(GstDetCollectorClass* klass);
static void gst_det_collector_init(GstDetCollector* self);

static GstFlowReturn gst_det_collector_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return collectorTransform(transform, buffer, engineOf(GST_DET_COLLECTOR(transform)));
}

static void gst_det_collector_finalize(GObject* object) {
    GstDetCollector* self = GST_DET_COLLECTOR(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_det_collector_class_init(GstDetCollectorClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_det_collector_finalize;
    transformClass->transform_ip = gst_det_collector_transform_ip;
    collectorAddPads(elementClass);
    gst_element_class_set_details_simple(
        elementClass, "DetCollector", "Filter/Metadata", collector::ELEMENT_DET_DETAIL, collector::PLUGIN_PACKAGE);
}

static void gst_det_collector_init(GstDetCollector* self) {
    self->engine = new CollectorEngine();
    collectorPrepare(GST_BASE_TRANSFORM(self));
}
