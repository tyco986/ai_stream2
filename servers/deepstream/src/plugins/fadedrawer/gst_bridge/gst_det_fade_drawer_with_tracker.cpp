#include "plugins/fadedrawer/gst_bridge/gst_det_fade_drawer_with_tracker.hpp"

#include "constants/fadedrawer_const.hpp"
#include "plugins/fadedrawer/engine/det_fade_engine_with_tracker.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_fade_drawer_utils.hpp"

enum {
    PROP_INTERVAL = 1,
    PROP_FADE_TIME,
    PROP_SHOW_LABEL,
    PROP_SHOW_SNAP,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_det_fade_drawer_with_tracker_parent_class parent_class
G_DEFINE_TYPE(GstDetFadeDrawerWithTracker, gst_det_fade_drawer_with_tracker, GST_TYPE_BASE_TRANSFORM);

static DetFadeEngineWithTracker* engineOf(GstDetFadeDrawerWithTracker* self) {
    return static_cast<DetFadeEngineWithTracker*>(self->engine);
}

static void renew(GstDetFadeDrawerWithTracker* self) {
    delete engineOf(self);
    self->engine = new DetFadeEngineWithTracker(self->interval, self->fadeTime, self->showLabel, self->showSnap);
}

static void gst_det_fade_drawer_with_tracker_finalize(GObject* object);
static void gst_det_fade_drawer_with_tracker_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_det_fade_drawer_with_tracker_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_det_fade_drawer_with_tracker_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_det_fade_drawer_with_tracker_class_init(GstDetFadeDrawerWithTrackerClass* klass);
static void gst_det_fade_drawer_with_tracker_init(GstDetFadeDrawerWithTracker* self);

static GstFlowReturn gst_det_fade_drawer_with_tracker_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return fadedrawerTransform(transform, buffer, engineOf(GST_DET_FADE_DRAWER_WITH_TRACKER(transform)));
}

static void gst_det_fade_drawer_with_tracker_finalize(GObject* object) {
    GstDetFadeDrawerWithTracker* self = GST_DET_FADE_DRAWER_WITH_TRACKER(object);
    delete engineOf(self);
    self->engine = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_det_fade_drawer_with_tracker_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstDetFadeDrawerWithTracker* self = GST_DET_FADE_DRAWER_WITH_TRACKER(object);
    if (propId == PROP_INTERVAL) {
        self->interval = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_FADE_TIME) {
        self->fadeTime = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_SHOW_LABEL) {
        self->showLabel = g_value_get_boolean(value);
        renew(self);
    } else if (propId == PROP_SHOW_SNAP) {
        self->showSnap = g_value_get_boolean(value);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_det_fade_drawer_with_tracker_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstDetFadeDrawerWithTracker* self = GST_DET_FADE_DRAWER_WITH_TRACKER(object);
    if (propId == PROP_INTERVAL) {
        g_value_set_int(value, self->interval);
    } else if (propId == PROP_FADE_TIME) {
        g_value_set_int(value, self->fadeTime);
    } else if (propId == PROP_SHOW_LABEL) {
        g_value_set_boolean(value, self->showLabel);
    } else if (propId == PROP_SHOW_SNAP) {
        g_value_set_boolean(value, self->showSnap);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_det_fade_drawer_with_tracker_class_init(GstDetFadeDrawerWithTrackerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_det_fade_drawer_with_tracker_finalize;
    objectClass->set_property = gst_det_fade_drawer_with_tracker_set_property;
    objectClass->get_property = gst_det_fade_drawer_with_tracker_get_property;
    transformClass->transform_ip = gst_det_fade_drawer_with_tracker_transform_ip;
    g_object_class_install_property(objectClass, PROP_INTERVAL, g_param_spec_int("interval", "Interval", "Inference period", 1, G_MAXINT, 1, readWrite));
    g_object_class_install_property(objectClass, PROP_FADE_TIME, g_param_spec_int("fade-time", "Fade time", "Fade LUT repeats", 0, G_MAXINT, 0, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_LABEL, g_param_spec_boolean("show-label", "Show label", "Show label text", FALSE, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_SNAP, g_param_spec_boolean("show-snap", "Show snap", "Fade snapshot boxes", TRUE, readWrite));
    fadedrawerAddPads(elementClass);
    gst_element_class_set_details_simple(elementClass, "DetFadeDrawerWithTracker", "Filter/Metadata", "Tracker boxes with snapshot fade", fadedrawer::PLUGIN_PACKAGE);
}

static void gst_det_fade_drawer_with_tracker_init(GstDetFadeDrawerWithTracker* self) {
    self->interval = 1;
    self->fadeTime = 0;
    self->showLabel = FALSE;
    self->showSnap = TRUE;
    self->engine = new DetFadeEngineWithTracker(self->interval, self->fadeTime, self->showLabel, self->showSnap);
    fadedrawerPrepare(GST_BASE_TRANSFORM(self));
}
