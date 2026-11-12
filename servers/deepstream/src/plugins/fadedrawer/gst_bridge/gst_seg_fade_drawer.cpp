#include "plugins/fadedrawer/gst_bridge/gst_seg_fade_drawer.hpp"

#include "constants/fadedrawer_const.hpp"
#include "plugins/fadedrawer/engine/seg_fade_engine.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_fade_drawer_utils.hpp"

enum {
    PROP_INTERVAL = 1,
    PROP_FADE_TIME,
    PROP_SHOW_LABEL,
    PROP_SHOW_MASK,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_seg_fade_drawer_parent_class parent_class
G_DEFINE_TYPE(GstSegFadeDrawer, gst_seg_fade_drawer, GST_TYPE_BASE_TRANSFORM);

static SegFadeEngine* engineOf(GstSegFadeDrawer* self) {
    return static_cast<SegFadeEngine*>(self->engine);
}

static void renew(GstSegFadeDrawer* self) {
    delete engineOf(self);
    self->engine = new SegFadeEngine(self->interval, self->fadeTime, self->showLabel, self->showMask);
}

static void gst_seg_fade_drawer_finalize(GObject* object);
static void gst_seg_fade_drawer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_seg_fade_drawer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_seg_fade_drawer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_seg_fade_drawer_class_init(GstSegFadeDrawerClass* klass);
static void gst_seg_fade_drawer_init(GstSegFadeDrawer* self);

static GstFlowReturn gst_seg_fade_drawer_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return fadedrawerTransform(transform, buffer, engineOf(GST_SEG_FADE_DRAWER(transform)));
}

static void gst_seg_fade_drawer_finalize(GObject* object) {
    GstSegFadeDrawer* self = GST_SEG_FADE_DRAWER(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_seg_fade_drawer_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstSegFadeDrawer* self = GST_SEG_FADE_DRAWER(object);
    if (propId == PROP_INTERVAL) {
        self->interval = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_FADE_TIME) {
        self->fadeTime = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_SHOW_LABEL) {
        self->showLabel = g_value_get_boolean(value);
        renew(self);
    } else if (propId == PROP_SHOW_MASK) {
        self->showMask = g_value_get_boolean(value);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_seg_fade_drawer_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstSegFadeDrawer* self = GST_SEG_FADE_DRAWER(object);
    if (propId == PROP_INTERVAL) {
        g_value_set_int(value, self->interval);
    } else if (propId == PROP_FADE_TIME) {
        g_value_set_int(value, self->fadeTime);
    } else if (propId == PROP_SHOW_LABEL) {
        g_value_set_boolean(value, self->showLabel);
    } else if (propId == PROP_SHOW_MASK) {
        g_value_set_boolean(value, self->showMask);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_seg_fade_drawer_class_init(GstSegFadeDrawerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_seg_fade_drawer_finalize;
    objectClass->set_property = gst_seg_fade_drawer_set_property;
    objectClass->get_property = gst_seg_fade_drawer_get_property;
    transformClass->transform_ip = gst_seg_fade_drawer_transform_ip;
    g_object_class_install_property(objectClass, PROP_INTERVAL, g_param_spec_int("interval", "Interval", "Inference period", 1, G_MAXINT, 1, readWrite));
    g_object_class_install_property(objectClass, PROP_FADE_TIME, g_param_spec_int("fade-time", "Fade time", "Fade LUT repeats", 0, G_MAXINT, 0, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_LABEL, g_param_spec_boolean("show-label", "Show label", "Show label text", FALSE, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_MASK, g_param_spec_boolean("show-mask", "Show mask", "Draw instance masks", TRUE, readWrite));
    fadedrawerAddPads(elementClass);
    gst_element_class_set_details_simple(elementClass, "SegFadeDrawer", "Filter/Metadata", "Fade detection boxes and instance masks", fadedrawer::PLUGIN_PACKAGE);
}

static void gst_seg_fade_drawer_init(GstSegFadeDrawer* self) {
    self->interval = 1;
    self->fadeTime = 0;
    self->showLabel = FALSE;
    self->showMask = TRUE;
    self->engine = new SegFadeEngine(self->interval, self->fadeTime, self->showLabel, self->showMask);
    fadedrawerPrepare(GST_BASE_TRANSFORM(self));
}
