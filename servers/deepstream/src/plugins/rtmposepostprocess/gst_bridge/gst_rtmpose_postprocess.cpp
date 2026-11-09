#include "plugins/rtmposepostprocess/gst_bridge/gst_rtmpose_postprocess.hpp"

#include "constants/rtmpose_const.hpp"
#include "constants/rtmposepostprocess_const.hpp"
#include "plugins/rtmposepostprocess/engine/rtmpose_postprocess.hpp"

#include <gst/video/video.h>

#include "nvds_latency_meta.h"

enum {
    PROP_SGIE_UNIQUE_ID = 1,
    PROP_INFER_WIDTH,
    PROP_INFER_HEIGHT,
    PROP_PADDING,
};

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(RTMPOSEPOSTPROCESS_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(RTMPOSEPOSTPROCESS_NVMM_CAPS));

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_rtmpose_postprocess_parent_class parent_class
G_DEFINE_TYPE(GstRtmposePostprocess, gst_rtmpose_postprocess, GST_TYPE_BASE_TRANSFORM);

static RtmposePostprocess* engineOf(GstRtmposePostprocess* self) {
    return static_cast<RtmposePostprocess*>(self->engine);
}

static void gst_rtmpose_postprocess_finalize(GObject* object);
static void gst_rtmpose_postprocess_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_rtmpose_postprocess_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_rtmpose_postprocess_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);

static GstFlowReturn gst_rtmpose_postprocess_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    GstRtmposePostprocess* self = GST_RTMPOSE_POSTPROCESS(transform);
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr) {
        nvds_acquire_meta_lock(batch);
        engineOf(self)->apply(batch);
        nvds_release_meta_lock(batch);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}

static void gst_rtmpose_postprocess_class_init(GstRtmposePostprocessClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->set_property = gst_rtmpose_postprocess_set_property;
    objectClass->get_property = gst_rtmpose_postprocess_get_property;
    objectClass->finalize = gst_rtmpose_postprocess_finalize;
    transformClass->transform_ip = gst_rtmpose_postprocess_transform_ip;
    g_object_class_install_property(
        objectClass, PROP_SGIE_UNIQUE_ID,
        g_param_spec_int(
            "sgie-unique-id", "SGIE unique id",
            "Pose SGIE unique-id whose keypoints tensor to map",
            0, G_MAXINT, rtmposepostprocess::DEFAULT_SGIE_UNIQUE_ID, readWrite));
    g_object_class_install_property(
        objectClass, PROP_INFER_WIDTH,
        g_param_spec_int(
            "infer-width", "Infer width",
            "RTMPose network width used to rebuild the letterbox",
            1, G_MAXINT, rtmpose::INFER_WIDTH, readWrite));
    g_object_class_install_property(
        objectClass, PROP_INFER_HEIGHT,
        g_param_spec_int(
            "infer-height", "Infer height",
            "RTMPose network height used to rebuild the letterbox",
            1, G_MAXINT, rtmpose::INFER_HEIGHT, readWrite));
    g_object_class_install_property(
        objectClass, PROP_PADDING,
        g_param_spec_float(
            "padding", "Padding",
            "Crop expand ratio shared with rtmpose preprocess",
            0.0f, G_MAXFLOAT, rtmpose::PADDING, readWrite));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_set_details_simple(
        elementClass, "RtmposePostprocess", "Filter/Metadata", rtmposepostprocess::ELEMENT_DETAIL, rtmposepostprocess::PLUGIN_PACKAGE);
}

static void gst_rtmpose_postprocess_init(GstRtmposePostprocess* self) {
    GstBaseTransform* transform = GST_BASE_TRANSFORM(self);
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
    self->engine = new RtmposePostprocess(
        rtmposepostprocess::DEFAULT_SGIE_UNIQUE_ID, rtmpose::INFER_WIDTH, rtmpose::INFER_HEIGHT, rtmpose::PADDING);
}

static void gst_rtmpose_postprocess_finalize(GObject* object) {
    GstRtmposePostprocess* self = GST_RTMPOSE_POSTPROCESS(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_rtmpose_postprocess_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstRtmposePostprocess* self = GST_RTMPOSE_POSTPROCESS(object);
    if (propId == PROP_SGIE_UNIQUE_ID) {
        engineOf(self)->setSgieUniqueId(g_value_get_int(value));
    } else if (propId == PROP_INFER_WIDTH) {
        engineOf(self)->setGeometry(g_value_get_int(value), engineOf(self)->inferHeight(), engineOf(self)->padding());
    } else if (propId == PROP_INFER_HEIGHT) {
        engineOf(self)->setGeometry(engineOf(self)->inferWidth(), g_value_get_int(value), engineOf(self)->padding());
    } else if (propId == PROP_PADDING) {
        engineOf(self)->setGeometry(engineOf(self)->inferWidth(), engineOf(self)->inferHeight(), g_value_get_float(value));
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_rtmpose_postprocess_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstRtmposePostprocess* self = GST_RTMPOSE_POSTPROCESS(object);
    if (propId == PROP_SGIE_UNIQUE_ID) {
        g_value_set_int(value, engineOf(self)->sgieUniqueId());
    } else if (propId == PROP_INFER_WIDTH) {
        g_value_set_int(value, engineOf(self)->inferWidth());
    } else if (propId == PROP_INFER_HEIGHT) {
        g_value_set_int(value, engineOf(self)->inferHeight());
    } else if (propId == PROP_PADDING) {
        g_value_set_float(value, engineOf(self)->padding());
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}
