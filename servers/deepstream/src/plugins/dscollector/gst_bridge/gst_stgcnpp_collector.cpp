#include "plugins/dscollector/gst_bridge/gst_stgcnpp_collector.hpp"

#include "constants/collector_const.hpp"
#include "plugins/dscollector/engine/stgcnpp_collector_engine.hpp"
#include "plugins/dscollector/gst_bridge/gst_collector_utils.hpp"

#define gst_stgcnpp_collector_parent_class parent_class
G_DEFINE_TYPE(GstStgcnppCollector, gst_stgcnpp_collector, GST_TYPE_BASE_TRANSFORM);

static StgcnppCollectorEngine* engineOf(GstStgcnppCollector* self) {
    return static_cast<StgcnppCollectorEngine*>(self->engine);
}

static void gst_stgcnpp_collector_finalize(GObject* object);
static GstFlowReturn gst_stgcnpp_collector_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_stgcnpp_collector_class_init(GstStgcnppCollectorClass* klass);
static void gst_stgcnpp_collector_init(GstStgcnppCollector* self);

static GstFlowReturn gst_stgcnpp_collector_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return collectorTransform(transform, buffer, engineOf(GST_STGCNPP_COLLECTOR(transform)));
}

static void gst_stgcnpp_collector_finalize(GObject* object) {
    GstStgcnppCollector* self = GST_STGCNPP_COLLECTOR(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_stgcnpp_collector_class_init(GstStgcnppCollectorClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_stgcnpp_collector_finalize;
    transformClass->transform_ip = gst_stgcnpp_collector_transform_ip;
    collectorAddPads(elementClass);
    gst_element_class_set_details_simple(
        elementClass, "StgcnppCollector", "Filter/Metadata", collector::ELEMENT_STGCNPP_DETAIL, collector::PLUGIN_PACKAGE);
}

static void gst_stgcnpp_collector_init(GstStgcnppCollector* self) {
    self->engine = new StgcnppCollectorEngine();
    collectorPrepare(GST_BASE_TRANSFORM(self));
}
