#include "plugins/stgcnppactionlabel/gst_bridge/gst_stgcnpp_action_label.hpp"

#include "constants/stgcnppactionlabel_const.hpp"
#include "plugins/stgcnppactionlabel/engine/stgcnpp_action_label.hpp"

#include <gst/video/video.h>

#include "nvds_latency_meta.h"

enum {
    PROP_CLASSIFIER_UNIQUE_ID = 1,
};

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(STGCNPPACTIONLABEL_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(STGCNPPACTIONLABEL_NVMM_CAPS));

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_stgcnpp_action_label_parent_class parent_class
G_DEFINE_TYPE(GstStgcnppActionLabel, gst_stgcnpp_action_label, GST_TYPE_BASE_TRANSFORM);

static StgcnppActionLabel* engineOf(GstStgcnppActionLabel* self) {
    return static_cast<StgcnppActionLabel*>(self->engine);
}

static void gst_stgcnpp_action_label_finalize(GObject* object);
static void gst_stgcnpp_action_label_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_stgcnpp_action_label_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_stgcnpp_action_label_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_stgcnpp_action_label_class_init(GstStgcnppActionLabelClass* klass);
static void gst_stgcnpp_action_label_init(GstStgcnppActionLabel* self);

static GstFlowReturn gst_stgcnpp_action_label_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    GstStgcnppActionLabel* self = GST_STGCNPP_ACTION_LABEL(transform);
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr) {
        nvds_acquire_meta_lock(batch);
        engineOf(self)->attach(batch);
        nvds_release_meta_lock(batch);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}

static void gst_stgcnpp_action_label_class_init(GstStgcnppActionLabelClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->set_property = gst_stgcnpp_action_label_set_property;
    objectClass->get_property = gst_stgcnpp_action_label_get_property;
    objectClass->finalize = gst_stgcnpp_action_label_finalize;
    transformClass->transform_ip = gst_stgcnpp_action_label_transform_ip;
    g_object_class_install_property(
        objectClass, PROP_CLASSIFIER_UNIQUE_ID,
        g_param_spec_int(
            "classifier-unique-id", "Classifier unique id",
            "ST-GCN++ SGIE unique-id whose classifier meta to read",
            0, G_MAXINT, stgcnppactionlabel::DEFAULT_CLASSIFIER_UNIQUE_ID, readWrite));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_set_details_simple(
        elementClass, "StgcnppActionLabel", "Filter/Metadata", stgcnppactionlabel::ELEMENT_DETAIL, stgcnppactionlabel::PLUGIN_PACKAGE);
}

static void gst_stgcnpp_action_label_init(GstStgcnppActionLabel* self) {
    GstBaseTransform* transform = GST_BASE_TRANSFORM(self);
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
    self->engine = new StgcnppActionLabel(stgcnppactionlabel::DEFAULT_CLASSIFIER_UNIQUE_ID);
}

static void gst_stgcnpp_action_label_finalize(GObject* object) {
    GstStgcnppActionLabel* self = GST_STGCNPP_ACTION_LABEL(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_stgcnpp_action_label_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstStgcnppActionLabel* self = GST_STGCNPP_ACTION_LABEL(object);
    if (propId == PROP_CLASSIFIER_UNIQUE_ID) {
        engineOf(self)->setClassifierUniqueId(g_value_get_int(value));
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_stgcnpp_action_label_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstStgcnppActionLabel* self = GST_STGCNPP_ACTION_LABEL(object);
    if (propId == PROP_CLASSIFIER_UNIQUE_ID) {
        g_value_set_int(value, engineOf(self)->classifierUniqueId());
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}
