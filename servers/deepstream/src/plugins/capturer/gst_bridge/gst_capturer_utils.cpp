#include "plugins/capturer/gst_bridge/gst_capturer_utils.hpp"

#include "constants/capturer_const.hpp"
#include "plugins/capturer/engine/capturer.hpp"

#include <gst/video/video.h>

#include "gstnvdsmeta.h"
#include "nvbufsurface.h"
#include "nvds_latency_meta.h"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(CAPTURER_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(CAPTURER_NVMM_CAPS));

void capturerAddPads(GstElementClass* elementClass) {
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
}

void capturerPrepare(GstBaseTransform* transform) {
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
}

GstFlowReturn capturerTransform(GstBaseTransform* transform, GstBuffer* buffer, Capturer* engine) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    GstFlowReturn flow = GST_FLOW_OK;
    GstMapInfo map;
    const bool mapped = gst_buffer_map(buffer, &map, GST_MAP_READ) != FALSE;
    if (mapped && engine != nullptr) {
        auto* surface = reinterpret_cast<NvBufSurface*>(map.data);
        NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
        if (batch != nullptr) {
            nvds_acquire_meta_lock(batch);
            for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
                auto* frame = static_cast<NvDsFrameMeta*>(item->data);
                if (frame != nullptr && !engine->capture(surface, frame)) {
                    flow = GST_FLOW_ERROR;
                }
            }
            nvds_release_meta_lock(batch);
        }
        gst_buffer_unmap(buffer, &map);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return flow;
}
