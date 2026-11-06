#include "plugins/boxsnapshot/gst_bridge/gst_boxsnapshot.hpp"

#include "constants/boxsnapshot_const.hpp"
#include "plugins/boxsnapshot/engine/boxsnapshot.hpp"

#include <gst/video/video.h>

#include "nvds_latency_meta.h"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(BOXSNAPSHOT_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(BOXSNAPSHOT_NVMM_CAPS));

#define gst_boxsnapshot_parent_class parent_class
G_DEFINE_TYPE(GstBoxSnapshot, gst_boxsnapshot, GST_TYPE_BASE_TRANSFORM);

static GstFlowReturn gst_boxsnapshot_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_boxsnapshot_class_init(GstBoxSnapshotClass* klass);
static void gst_boxsnapshot_init(GstBoxSnapshot* self);

static GstFlowReturn gst_boxsnapshot_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr) {
        nvds_acquire_meta_lock(batch);
        for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
            auto* frame = static_cast<NvDsFrameMeta*>(item->data);
            if (frame != nullptr && frame->bInferDone) {
                BoxSnapshot::attach(batch, frame);
            }
        }
        nvds_release_meta_lock(batch);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}

static void gst_boxsnapshot_class_init(GstBoxSnapshotClass* klass) {
    GstElementClass* elementClass = reinterpret_cast<GstElementClass*>(klass);
    GstBaseTransformClass* transformClass = reinterpret_cast<GstBaseTransformClass*>(klass);
    transformClass->transform_ip = GST_DEBUG_FUNCPTR(gst_boxsnapshot_transform_ip);
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_set_details_simple(
        elementClass, "BoxSnapshot", "Filter/Metadata", boxsnapshot::ELEMENT_DETAIL, boxsnapshot::PLUGIN_PACKAGE);
}

static void gst_boxsnapshot_init(GstBoxSnapshot* self) {
    GstBaseTransform* transform = GST_BASE_TRANSFORM(self);
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
}
