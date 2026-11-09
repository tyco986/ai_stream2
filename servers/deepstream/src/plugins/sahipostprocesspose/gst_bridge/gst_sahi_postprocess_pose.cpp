#include "plugins/sahipostprocesspose/gst_bridge/gst_sahi_postprocess_pose.hpp"

#include <gst/video/video.h>

#include "constants/sahipostprocesspose_const.hpp"
#include "nvds_latency_meta.h"
#include "plugins/sahipostprocesspose/engine/sahi_pose_nms.hpp"

enum {
    PROP_GIE_IDS = 1,
    PROP_CLASS_AGNOSTIC,
    PROP_MATCH_METRIC,
    PROP_MATCH_THRESHOLD,
    PROP_PREFER_FULL_FRAME,
    PROP_MAX_DETECTIONS,
    PROP_OKS_THRESHOLD,
    PROP_VIS_THRESHOLD,
    PROP_NUM_KEYPOINTS,
};

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(SAHIPOSES_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(SAHIPOSES_NVMM_CAPS));

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_sahi_postprocess_pose_parent_class parent_class
G_DEFINE_TYPE(GstSahiPostProcessPose, gst_sahi_postprocess_pose, GST_TYPE_BASE_TRANSFORM);

static SahiPoseNms* engineOf(GstSahiPostProcessPose* self) {
    return static_cast<SahiPoseNms*>(self->engine);
}

static void renew(GstSahiPostProcessPose* self) {
    delete engineOf(self);
    self->engine = new SahiPoseNms(self->gieIds, self->classAgnostic, self->matchMetric, self->matchThreshold, self->preferFullFrame, self->maxDetections, self->oksThreshold, self->visThreshold, self->numKeypoints);
}

static void gst_sahi_postprocess_pose_finalize(GObject* object);
static void gst_sahi_postprocess_pose_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_sahi_postprocess_pose_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_sahi_postprocess_pose_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_sahi_postprocess_pose_class_init(GstSahiPostProcessPoseClass* klass);
static void gst_sahi_postprocess_pose_init(GstSahiPostProcessPose* self);

static GstFlowReturn gst_sahi_postprocess_pose_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    GstSahiPostProcessPose* self = GST_SAHI_POSTPROCESS_POSE(transform);
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr) {
        nvds_acquire_meta_lock(batch);
        for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
            auto* frame = static_cast<NvDsFrameMeta*>(item->data);
            if (frame != nullptr) {
                engineOf(self)->suppress(frame);
            }
        }
        nvds_release_meta_lock(batch);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}

static void gst_sahi_postprocess_pose_class_init(GstSahiPostProcessPoseClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->set_property = gst_sahi_postprocess_pose_set_property;
    objectClass->get_property = gst_sahi_postprocess_pose_get_property;
    objectClass->finalize = gst_sahi_postprocess_pose_finalize;
    transformClass->transform_ip = gst_sahi_postprocess_pose_transform_ip;
    g_object_class_install_property(
        objectClass, PROP_GIE_IDS,
        g_param_spec_string("gie-ids", "GIE IDs", "\"-1\"=all, or semicolon-separated ids", sahipostprocesspose::DEFAULT_GIE_IDS, readWrite));
    g_object_class_install_property(
        objectClass, PROP_CLASS_AGNOSTIC,
        g_param_spec_boolean("class-agnostic", "Class agnostic", "Match across different class IDs", sahipostprocesspose::DEFAULT_CLASS_AGNOSTIC, readWrite));
    g_object_class_install_property(
        objectClass, PROP_MATCH_METRIC,
        g_param_spec_int("match-metric", "Match metric", "0=IoU, 1=IoS", sahipostprocesspose::MATCH_METRIC_IOU, sahipostprocesspose::MATCH_METRIC_IOS, sahipostprocesspose::DEFAULT_MATCH_METRIC, readWrite));
    g_object_class_install_property(
        objectClass, PROP_MATCH_THRESHOLD,
        g_param_spec_float("match-threshold", "Match threshold", "Box overlap at or above this suppresses the lower-score pose", 0.0f, 1.0f, sahipostprocesspose::DEFAULT_MATCH_THRESHOLD, readWrite));
    g_object_class_install_property(
        objectClass, PROP_PREFER_FULL_FRAME,
        g_param_spec_boolean("prefer-full-frame", "Prefer full frame", "Keep full-frame poses and drop overlapping slice poses before OKS", sahipostprocesspose::DEFAULT_PREFER_FULL_FRAME, readWrite));
    g_object_class_install_property(
        objectClass, PROP_MAX_DETECTIONS,
        g_param_spec_int("max-detections", "Max detections", "Max surviving poses per frame (-1=unlimited)", -1, G_MAXINT, sahipostprocesspose::DEFAULT_MAX_DETECTIONS, readWrite));
    g_object_class_install_property(
        objectClass, PROP_OKS_THRESHOLD,
        g_param_spec_float("oks-threshold", "OKS threshold", "OKS above this suppresses the lower-score pose", 0.0f, 1.0f, sahipostprocesspose::DEFAULT_OKS_THRESHOLD, readWrite));
    g_object_class_install_property(
        objectClass, PROP_VIS_THRESHOLD,
        g_param_spec_float("vis-threshold", "Visibility threshold", "Keypoint score must exceed this to count in OKS", 0.0f, 1.0f, sahipostprocesspose::DEFAULT_VIS_THRESHOLD, readWrite));
    g_object_class_install_property(
        objectClass, PROP_NUM_KEYPOINTS,
        g_param_spec_int("num-keypoints", "Num keypoints", "K for Kx3 keypoints (0=use mask height)", 0, G_MAXINT, sahipostprocesspose::DEFAULT_NUM_KEYPOINTS, readWrite));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_set_details_simple(
        elementClass, "SahiPostProcessPose", "Filter/Metadata", sahipostprocesspose::ELEMENT_DETAIL, sahipostprocesspose::PLUGIN_PACKAGE);
}

static void gst_sahi_postprocess_pose_init(GstSahiPostProcessPose* self) {
    GstBaseTransform* transform = GST_BASE_TRANSFORM(self);
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
    self->gieIds = g_strdup(sahipostprocesspose::DEFAULT_GIE_IDS);
    self->classAgnostic = sahipostprocesspose::DEFAULT_CLASS_AGNOSTIC;
    self->matchMetric = sahipostprocesspose::DEFAULT_MATCH_METRIC;
    self->matchThreshold = sahipostprocesspose::DEFAULT_MATCH_THRESHOLD;
    self->preferFullFrame = sahipostprocesspose::DEFAULT_PREFER_FULL_FRAME;
    self->maxDetections = sahipostprocesspose::DEFAULT_MAX_DETECTIONS;
    self->oksThreshold = sahipostprocesspose::DEFAULT_OKS_THRESHOLD;
    self->visThreshold = sahipostprocesspose::DEFAULT_VIS_THRESHOLD;
    self->numKeypoints = sahipostprocesspose::DEFAULT_NUM_KEYPOINTS;
    self->engine = new SahiPoseNms(self->gieIds, self->classAgnostic, self->matchMetric, self->matchThreshold, self->preferFullFrame, self->maxDetections, self->oksThreshold, self->visThreshold, self->numKeypoints);
}

static void gst_sahi_postprocess_pose_finalize(GObject* object) {
    GstSahiPostProcessPose* self = GST_SAHI_POSTPROCESS_POSE(object);
    delete engineOf(self);
    self->engine = nullptr;
    g_free(self->gieIds);
    self->gieIds = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_sahi_postprocess_pose_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstSahiPostProcessPose* self = GST_SAHI_POSTPROCESS_POSE(object);
    const gchar* text = nullptr;
    if (propId == PROP_GIE_IDS) {
        text = g_value_get_string(value);
        g_free(self->gieIds);
        self->gieIds = g_strdup(text != nullptr ? text : sahipostprocesspose::DEFAULT_GIE_IDS);
        renew(self);
    } else if (propId == PROP_CLASS_AGNOSTIC) {
        self->classAgnostic = g_value_get_boolean(value);
        renew(self);
    } else if (propId == PROP_MATCH_METRIC) {
        self->matchMetric = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_MATCH_THRESHOLD) {
        self->matchThreshold = g_value_get_float(value);
        renew(self);
    } else if (propId == PROP_PREFER_FULL_FRAME) {
        self->preferFullFrame = g_value_get_boolean(value);
        renew(self);
    } else if (propId == PROP_MAX_DETECTIONS) {
        self->maxDetections = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_OKS_THRESHOLD) {
        self->oksThreshold = g_value_get_float(value);
        renew(self);
    } else if (propId == PROP_VIS_THRESHOLD) {
        self->visThreshold = g_value_get_float(value);
        renew(self);
    } else if (propId == PROP_NUM_KEYPOINTS) {
        self->numKeypoints = g_value_get_int(value);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_sahi_postprocess_pose_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstSahiPostProcessPose* self = GST_SAHI_POSTPROCESS_POSE(object);
    if (propId == PROP_GIE_IDS) {
        g_value_set_string(value, self->gieIds);
    } else if (propId == PROP_CLASS_AGNOSTIC) {
        g_value_set_boolean(value, self->classAgnostic);
    } else if (propId == PROP_MATCH_METRIC) {
        g_value_set_int(value, self->matchMetric);
    } else if (propId == PROP_MATCH_THRESHOLD) {
        g_value_set_float(value, self->matchThreshold);
    } else if (propId == PROP_PREFER_FULL_FRAME) {
        g_value_set_boolean(value, self->preferFullFrame);
    } else if (propId == PROP_MAX_DETECTIONS) {
        g_value_set_int(value, self->maxDetections);
    } else if (propId == PROP_OKS_THRESHOLD) {
        g_value_set_float(value, self->oksThreshold);
    } else if (propId == PROP_VIS_THRESHOLD) {
        g_value_set_float(value, self->visThreshold);
    } else if (propId == PROP_NUM_KEYPOINTS) {
        g_value_set_int(value, self->numKeypoints);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}
