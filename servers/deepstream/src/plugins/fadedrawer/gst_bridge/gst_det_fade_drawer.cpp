#include "plugins/fadedrawer/gst_bridge/gst_det_fade_drawer.hpp"

#include "constants/fadedrawer_const.hpp"
#include "plugins/fadedrawer/engine/det_fade_engine.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_fade_drawer_utils.hpp"

enum {
    PROP_INTERVAL = 1,
    PROP_FADE_TIME,
    PROP_SHOW_LABEL,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_det_fade_drawer_parent_class parent_class
G_DEFINE_TYPE(GstDetFadeDrawer, gst_det_fade_drawer, GST_TYPE_BASE_TRANSFORM);

static DetFadeEngine* engineOf(GstDetFadeDrawer* self) {
    return static_cast<DetFadeEngine*>(self->engine);
}

static void renew(GstDetFadeDrawer* self) {
    delete engineOf(self);
    self->engine = new DetFadeEngine(self->interval, self->fadeTime, self->showLabel);
}

static void gst_det_fade_drawer_finalize(GObject* object);
static void gst_det_fade_drawer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_det_fade_drawer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_det_fade_drawer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_det_fade_drawer_class_init(GstDetFadeDrawerClass* klass);
static void gst_det_fade_drawer_init(GstDetFadeDrawer* self);

static GstFlowReturn gst_det_fade_drawer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return fadedrawerTransform(transform, buffer, engineOf(GST_DET_FADE_DRAWER(transform)));
}

static void gst_det_fade_drawer_finalize(GObject* object) {
    GstDetFadeDrawer* self = GST_DET_FADE_DRAWER(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_det_fade_drawer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstDetFadeDrawer* self = GST_DET_FADE_DRAWER(object);
    if (propId == PROP_INTERVAL) {
        self->interval = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_FADE_TIME) {
        self->fadeTime = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_SHOW_LABEL) {
        self->showLabel = g_value_get_boolean(value);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_det_fade_drawer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstDetFadeDrawer* self = GST_DET_FADE_DRAWER(object);
    if (propId == PROP_INTERVAL) {
        g_value_set_int(value, self->interval);
    } else if (propId == PROP_FADE_TIME) {
        g_value_set_int(value, self->fadeTime);
    } else if (propId == PROP_SHOW_LABEL) {
        g_value_set_boolean(value, self->showLabel);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_det_fade_drawer_class_init(GstDetFadeDrawerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_det_fade_drawer_finalize;
    objectClass->set_property = gst_det_fade_drawer_set_property;
    objectClass->get_property = gst_det_fade_drawer_get_property;
    transformClass->transform_ip = gst_det_fade_drawer_transform_ip;
    g_object_class_install_property(objectClass, PROP_INTERVAL, g_param_spec_int("interval", "Interval", "Inference period", 0, G_MAXINT, 0, readWrite));
    g_object_class_install_property(objectClass, PROP_FADE_TIME, g_param_spec_int("fade-time", "Fade time", "Fade LUT repeats", 0, G_MAXINT, 0, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_LABEL, g_param_spec_boolean("show-label", "Show label", "Show label text", FALSE, readWrite));
    fadedrawerAddPads(elementClass);
    gst_element_class_set_details_simple(elementClass, "DetFadeDrawer", "Filter/Metadata", "Fade detection boxes", fadedrawer::PLUGIN_PACKAGE);
}

static void gst_det_fade_drawer_init(GstDetFadeDrawer* self) {
    self->interval = 0;
    self->fadeTime = 0;
    self->showLabel = FALSE;
    self->engine = new DetFadeEngine(self->interval, self->fadeTime, self->showLabel);
    fadedrawerPrepare(GST_BASE_TRANSFORM(self));
}

#undef gst_det_fade_drawer_parent_class
