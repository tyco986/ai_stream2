#include "plugins/dslogger/gst_bridge/gst_logger_utils.hpp"

#include <gst/video/video.h>

#include "constants/dslogger_const.hpp"
#include "gstnvdsmeta.h"
#include "plugins/dslogger/engine/ds_logger.hpp"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(DSLOGGER_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(DSLOGGER_NVMM_CAPS));

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

GstFlowReturn dsloggerTransform(GstBaseTransform* transform, GstBuffer* buffer, DsLogger* logger) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    if (logger != nullptr && !logger->named()) {
        logger->setPipelineName(dsloggerPipelineName(GST_ELEMENT(transform)));
    }
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr && logger != nullptr) {
        nvds_acquire_meta_lock(batch);
        for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
            auto* frame = static_cast<NvDsFrameMeta*>(item->data);
            if (frame != nullptr) {
                logger->writeFrame(frame);
            }
        }
        nvds_release_meta_lock(batch);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}
