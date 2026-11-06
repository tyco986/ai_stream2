#include "plugins/fadedrawer/gst_bridge/gst_fade_drawer_utils.hpp"

#include "constants/fadedrawer_const.hpp"
#include "plugins/fadedrawer/engine/det_fade_engine.hpp"

#include <gst/video/video.h>

#include "nvds_latency_meta.h"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(FADEDRAWER_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(FADEDRAWER_NVMM_CAPS));

void fadedrawerAddPads(GstElementClass* elementClass) {
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
}

void fadedrawerPrepare(GstBaseTransform* transform) {
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
}

GstFlowReturn fadedrawerTransform(GstBaseTransform* transform, GstBuffer* buffer, DetFadeEngine* engine) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr && engine != nullptr) {
        nvds_acquire_meta_lock(batch);
        for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
            auto* frame = static_cast<NvDsFrameMeta*>(item->data);
            if (frame != nullptr) {
                engine->processFrame(batch, frame);
            }
        }
        nvds_release_meta_lock(batch);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}
