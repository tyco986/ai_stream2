#include "plugins/dscollector/gst_bridge/gst_collector_utils.hpp"

#include <gst/video/video.h>

#include "constants/collector_const.hpp"
#include "gstnvdsmeta.h"
#include "nvds_latency_meta.h"
#include "plugins/dscollector/engine/collector_engine.hpp"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(COLLECTOR_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(COLLECTOR_NVMM_CAPS));
struct CollectorInstance {
    GstBaseTransform base;
    gpointer engine;
};

static CollectorEngine* engineFrom(gpointer object) {
    return static_cast<CollectorEngine*>(reinterpret_cast<CollectorInstance*>(object)->engine);
}

static void collectorSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    if (propId == 1) {
        engineFrom(object)->setInterval(g_value_get_int(value));
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void collectorGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    if (propId == 1) {
        g_value_set_int(value, engineFrom(object)->getInterval());
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static GstFlowReturn collectorOnTransform(GstBaseTransform* transform, GstBuffer* buffer) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    CollectorEngine* engine = engineFrom(transform);
    if (batch != nullptr && engine != nullptr) {
        const guint frameSlots = batch->max_frames_in_batch;
        auto* latencyInfo = static_cast<NvDsFrameLatencyInfo*>(g_malloc0(sizeof(NvDsFrameLatencyInfo) * frameSlots));
        const guint latencyCount = nvds_measure_buffer_latency(buffer, latencyInfo);
        nvds_acquire_meta_lock(batch);
        engine->collect(batch, latencyInfo, latencyCount);
        nvds_release_meta_lock(batch);
        g_free(latencyInfo);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}

void collectorAddPads(GstElementClass* elementClass) {
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
}

void collectorConfigure(GObjectClass* objectClass, GstBaseTransformClass* transformClass) {
    objectClass->set_property = collectorSetProperty;
    objectClass->get_property = collectorGetProperty;
    transformClass->transform_ip = collectorOnTransform;
    g_object_class_install_property(
        objectClass, 1,
        g_param_spec_int("interval", "Interval", "Collect period in batches", 1, G_MAXINT, collector::DEFAULT_INTERVAL, static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

void collectorPrepare(GstBaseTransform* transform) {
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
}
