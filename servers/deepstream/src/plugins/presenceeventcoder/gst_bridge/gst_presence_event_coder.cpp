#include "plugins/presenceeventcoder/gst_bridge/gst_presence_event_coder.hpp"

#include "constants/presenceeventcoder_const.hpp"
#include "plugins/presenceeventcoder/engine/presence_event_coder.hpp"

#include <gst/video/video.h>

#include "nvds_latency_meta.h"

enum {
    PROP_CLASS_EVENT_MAP = 1,
    PROP_LENGTH,
    PROP_THRESHOLD,
    PROP_MODE,
};

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(PRESENCEEVENTCODER_NVMM_CAPS));
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(PRESENCEEVENTCODER_NVMM_CAPS));

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_presence_event_coder_parent_class parent_class
G_DEFINE_TYPE(GstPresenceEventCoder, gst_presence_event_coder, GST_TYPE_BASE_TRANSFORM);

static PresenceEventCoder* engineOf(GstPresenceEventCoder* self) {
    return static_cast<PresenceEventCoder*>(self->engine);
}

static void renew(GstPresenceEventCoder* self) {
    delete engineOf(self);
    self->engine = new PresenceEventCoder(self->classEventMap, self->length, self->threshold, self->mode);
}

static void gst_presence_event_coder_finalize(GObject* object);
static void gst_presence_event_coder_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_presence_event_coder_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_presence_event_coder_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_presence_event_coder_class_init(GstPresenceEventCoderClass* klass);
static void gst_presence_event_coder_init(GstPresenceEventCoder* self);

static GstFlowReturn gst_presence_event_coder_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    nvds_set_input_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    GstPresenceEventCoder* self = GST_PRESENCE_EVENT_CODER(transform);
    NvDsBatchMeta* batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (batch != nullptr) {
        nvds_acquire_meta_lock(batch);
        for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
            auto* frame = static_cast<NvDsFrameMeta*>(item->data);
            if (frame != nullptr) {
                engineOf(self)->attach(batch, frame);
            }
        }
        nvds_release_meta_lock(batch);
    }
    nvds_set_output_system_timestamp(buffer, GST_ELEMENT_NAME(transform));
    return GST_FLOW_OK;
}

static void gst_presence_event_coder_class_init(GstPresenceEventCoderClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->set_property = gst_presence_event_coder_set_property;
    objectClass->get_property = gst_presence_event_coder_get_property;
    objectClass->finalize = gst_presence_event_coder_finalize;
    transformClass->transform_ip = gst_presence_event_coder_transform_ip;
    g_object_class_install_property(
        objectClass, PROP_CLASS_EVENT_MAP,
        g_param_spec_string(
            "class-event-map", "Class event map",
            "Model class id to event name, e.g. 0:smoke;1:fire", "", readWrite));
    g_object_class_install_property(
        objectClass, PROP_LENGTH,
        g_param_spec_int("length", "Length", "Presence window length", 1, G_MAXINT, presenceeventcoder::DEFAULT_LENGTH, readWrite));
    g_object_class_install_property(
        objectClass, PROP_THRESHOLD,
        g_param_spec_float("threshold", "Threshold", "Presence ratio threshold", 0.0f, 1.0f, presenceeventcoder::DEFAULT_THRESHOLD, readWrite));
    g_object_class_install_property(
        objectClass, PROP_MODE,
        g_param_spec_string("mode", "Mode", "slide or fold", presenceeventcoder::DEFAULT_MODE, readWrite));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_set_details_simple(
        elementClass, "PresenceEventCoder", "Filter/Metadata", presenceeventcoder::ELEMENT_DETAIL, presenceeventcoder::PLUGIN_PACKAGE);
}

static void gst_presence_event_coder_init(GstPresenceEventCoder* self) {
    GstBaseTransform* transform = GST_BASE_TRANSFORM(self);
    gst_base_transform_set_in_place(transform, TRUE);
    gst_base_transform_set_passthrough(transform, FALSE);
    self->classEventMap = g_strdup("");
    self->length = presenceeventcoder::DEFAULT_LENGTH;
    self->threshold = presenceeventcoder::DEFAULT_THRESHOLD;
    self->mode = g_strdup(presenceeventcoder::DEFAULT_MODE);
    self->engine = new PresenceEventCoder(self->classEventMap, self->length, self->threshold, self->mode);
}

static void gst_presence_event_coder_finalize(GObject* object) {
    GstPresenceEventCoder* self = GST_PRESENCE_EVENT_CODER(object);
    delete engineOf(self);
    self->engine = nullptr;
    g_free(self->classEventMap);
    g_free(self->mode);
    self->classEventMap = nullptr;
    self->mode = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_presence_event_coder_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstPresenceEventCoder* self = GST_PRESENCE_EVENT_CODER(object);
    const gchar* text = nullptr;
    if (propId == PROP_CLASS_EVENT_MAP) {
        text = g_value_get_string(value);
        g_free(self->classEventMap);
        self->classEventMap = g_strdup(text != nullptr ? text : "");
        renew(self);
    } else if (propId == PROP_LENGTH) {
        self->length = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_THRESHOLD) {
        self->threshold = g_value_get_float(value);
        renew(self);
    } else if (propId == PROP_MODE) {
        text = g_value_get_string(value);
        g_free(self->mode);
        self->mode = g_strdup(text != nullptr ? text : presenceeventcoder::DEFAULT_MODE);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_presence_event_coder_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstPresenceEventCoder* self = GST_PRESENCE_EVENT_CODER(object);
    if (propId == PROP_CLASS_EVENT_MAP) {
        g_value_set_string(value, self->classEventMap);
    } else if (propId == PROP_LENGTH) {
        g_value_set_int(value, self->length);
    } else if (propId == PROP_THRESHOLD) {
        g_value_set_float(value, self->threshold);
    } else if (propId == PROP_MODE) {
        g_value_set_string(value, self->mode);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}
