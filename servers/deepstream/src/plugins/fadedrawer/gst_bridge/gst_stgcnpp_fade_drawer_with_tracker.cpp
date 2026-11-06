#include "plugins/fadedrawer/gst_bridge/gst_stgcnpp_fade_drawer_with_tracker.hpp"

#include "constants/fadedrawer_const.hpp"
#include "plugins/fadedrawer/engine/stgcnpp_fade_engine_with_tracker.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_fade_drawer_utils.hpp"

enum {
    PROP_INTERVAL = 1,
    PROP_FADE_TIME,
    PROP_SHOW_LABEL,
    PROP_SHOW_POSE,
    PROP_POSE_THRESHOLD,
    PROP_MODE,
    PROP_SHOW_SNAP,
};

static GParamFlags readWrite = static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

#define gst_stgcnpp_fade_drawer_with_tracker_parent_class parent_class
G_DEFINE_TYPE(GstStgcnppFadeDrawerWithTracker, gst_stgcnpp_fade_drawer_with_tracker, GST_TYPE_BASE_TRANSFORM);

static StgcnppFadeEngineWithTracker* engineOf(GstStgcnppFadeDrawerWithTracker* self) {
    return static_cast<StgcnppFadeEngineWithTracker*>(self->engine);
}

static void renew(GstStgcnppFadeDrawerWithTracker* self) {
    delete engineOf(self);
    self->engine = new StgcnppFadeEngineWithTracker(self->interval, self->fadeTime, self->showLabel, self->showPose, self->poseThreshold, self->showSnap);
}

static void gst_stgcnpp_fade_drawer_with_tracker_finalize(GObject* object);
static void gst_stgcnpp_fade_drawer_with_tracker_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec);
static void gst_stgcnpp_fade_drawer_with_tracker_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_stgcnpp_fade_drawer_with_tracker_transform_ip(GstBaseTransform* transform, GstBuffer* buffer);
static void gst_stgcnpp_fade_drawer_with_tracker_class_init(GstStgcnppFadeDrawerWithTrackerClass* klass);
static void gst_stgcnpp_fade_drawer_with_tracker_init(GstStgcnppFadeDrawerWithTracker* self);

static GstFlowReturn gst_stgcnpp_fade_drawer_with_tracker_transform_ip(GstBaseTransform* transform, GstBuffer* buffer) {
    return fadedrawerTransform(transform, buffer, engineOf(GST_STGCNPP_FADE_DRAWER_WITH_TRACKER(transform)));
}

static void gst_stgcnpp_fade_drawer_with_tracker_finalize(GObject* object) {
    GstStgcnppFadeDrawerWithTracker* self = GST_STGCNPP_FADE_DRAWER_WITH_TRACKER(object);
    delete engineOf(self);
    self->engine = nullptr;
    g_free(self->mode);
    self->mode = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_stgcnpp_fade_drawer_with_tracker_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec) {
    GstStgcnppFadeDrawerWithTracker* self = GST_STGCNPP_FADE_DRAWER_WITH_TRACKER(object);
    const gchar* mode = nullptr;
    if (propId == PROP_INTERVAL) {
        self->interval = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_FADE_TIME) {
        self->fadeTime = g_value_get_int(value);
        renew(self);
    } else if (propId == PROP_SHOW_LABEL) {
        self->showLabel = g_value_get_boolean(value);
        renew(self);
    } else if (propId == PROP_SHOW_POSE) {
        self->showPose = g_value_get_boolean(value);
        renew(self);
    } else if (propId == PROP_POSE_THRESHOLD) {
        self->poseThreshold = g_value_get_float(value);
        renew(self);
    } else if (propId == PROP_MODE) {
        mode = g_value_get_string(value);
        g_free(self->mode);
        self->mode = g_strdup(mode != nullptr ? mode : fadedrawer::POSE_MODE);
    } else if (propId == PROP_SHOW_SNAP) {
        self->showSnap = g_value_get_boolean(value);
        renew(self);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_stgcnpp_fade_drawer_with_tracker_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec) {
    GstStgcnppFadeDrawerWithTracker* self = GST_STGCNPP_FADE_DRAWER_WITH_TRACKER(object);
    if (propId == PROP_INTERVAL) {
        g_value_set_int(value, self->interval);
    } else if (propId == PROP_FADE_TIME) {
        g_value_set_int(value, self->fadeTime);
    } else if (propId == PROP_SHOW_LABEL) {
        g_value_set_boolean(value, self->showLabel);
    } else if (propId == PROP_SHOW_POSE) {
        g_value_set_boolean(value, self->showPose);
    } else if (propId == PROP_POSE_THRESHOLD) {
        g_value_set_float(value, self->poseThreshold);
    } else if (propId == PROP_MODE) {
        g_value_set_string(value, self->mode);
    } else if (propId == PROP_SHOW_SNAP) {
        g_value_set_boolean(value, self->showSnap);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void gst_stgcnpp_fade_drawer_with_tracker_class_init(GstStgcnppFadeDrawerWithTrackerClass* klass) {
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transformClass = GST_BASE_TRANSFORM_CLASS(klass);
    objectClass->finalize = gst_stgcnpp_fade_drawer_with_tracker_finalize;
    objectClass->set_property = gst_stgcnpp_fade_drawer_with_tracker_set_property;
    objectClass->get_property = gst_stgcnpp_fade_drawer_with_tracker_get_property;
    transformClass->transform_ip = gst_stgcnpp_fade_drawer_with_tracker_transform_ip;
    g_object_class_install_property(objectClass, PROP_INTERVAL, g_param_spec_int("interval", "Interval", "Inference period", 0, G_MAXINT, 0, readWrite));
    g_object_class_install_property(objectClass, PROP_FADE_TIME, g_param_spec_int("fade-time", "Fade time", "Fade LUT repeats", 0, G_MAXINT, 0, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_LABEL, g_param_spec_boolean("show-label", "Show label", "Show label text", FALSE, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_POSE, g_param_spec_boolean("show-pose", "Show pose", "Draw keypoints and skeleton", TRUE, readWrite));
    g_object_class_install_property(objectClass, PROP_POSE_THRESHOLD, g_param_spec_float("pose-threshold", "Pose threshold", "Keypoint score drawn red below this", 0.0f, 1.0f, 0.0f, readWrite));
    g_object_class_install_property(objectClass, PROP_MODE, g_param_spec_string("mode", "Mode", "Skeleton topology", fadedrawer::POSE_MODE, readWrite));
    g_object_class_install_property(objectClass, PROP_SHOW_SNAP, g_param_spec_boolean("show-snap", "Show snap", "Fade snapshot boxes", TRUE, readWrite));
    fadedrawerAddPads(elementClass);
    gst_element_class_set_details_simple(elementClass, "StgcnppFadeDrawerWithTracker", "Filter/Metadata", "Tracker pose with ST-GCN++ action labels", fadedrawer::PLUGIN_PACKAGE);
}

static void gst_stgcnpp_fade_drawer_with_tracker_init(GstStgcnppFadeDrawerWithTracker* self) {
    self->interval = 0;
    self->fadeTime = 0;
    self->showLabel = FALSE;
    self->showPose = TRUE;
    self->poseThreshold = 0.0f;
    self->mode = g_strdup(fadedrawer::POSE_MODE);
    self->showSnap = TRUE;
    self->engine = new StgcnppFadeEngineWithTracker(self->interval, self->fadeTime, self->showLabel, self->showPose, self->poseThreshold, self->showSnap);
    fadedrawerPrepare(GST_BASE_TRANSFORM(self));
}
