#include <gst/video/video.h>

#include "constants/dslogger_const.hpp"
#include "gstnvdsmeta.h"
#include "nvds_latency_meta.h"
#include "plugins/dslogger/engine/log_engine.hpp"
#include "plugins/dslogger/gst_bridge/gst_logger_utils.hpp"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(DSLOGGER_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(DSLOGGER_NVMM_CAPS));

static double latencyForFrame(NvDsFrameLatencyInfo* info, guint count, NvDsFrameMeta* frame) {
    double latencyMs = dslogger::LATENCY_NONE;
    if (info != nullptr && frame != nullptr) {
        for (guint index = 0; index < count; index++) {
            if (info[index].source_id == frame->source_id && static_cast<gint>(info[index].frame_num) == frame->frame_num) {
                latencyMs = info[index].latency;
            }
        }
    }
    return latencyMs;
}

void dsloggerAddPads(GstElementClass* elementClass) {
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
}

void dsloggerPrepare(GstBaseTransform* transform) {
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
}

std::string dsloggerPipelineName(GstElement* element) {
    GstObject* object = GST_OBJECT(element);
    while (object != nullptr && !GST_IS_PIPELINE(object)) {
        object = GST_OBJECT_PARENT(object);
    }
    const gchar* name = object != nullptr ? GST_OBJECT_NAME(object) : nullptr;
    return name != nullptr ? std::string(name) : std::string();
}

GstFlowReturn dsloggerTransform(GstBaseTransform* transform, GstBuffer* buffer, LogEngine* engine) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    if (engine != nullptr && !engine->named()) {
        engine->setPipelineName(dsloggerPipelineName(GST_ELEMENT(transform)));
    }
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr && engine != nullptr) {
        const guint frameSlots = batch->max_frames_in_batch;
        auto* latencyInfo = static_cast<NvDsFrameLatencyInfo*>(g_malloc0(sizeof(NvDsFrameLatencyInfo) * frameSlots));
        const guint latencyCount = nvds_measure_buffer_latency(buffer, latencyInfo);
        nvds_acquire_meta_lock(batch);
        for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
            auto* frame = static_cast<NvDsFrameMeta*>(item->data);
            if (frame != nullptr) {
                engine->processFrame(frame, latencyForFrame(latencyInfo, latencyCount, frame));
            }
        }
        nvds_release_meta_lock(batch);
        g_free(latencyInfo);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}
