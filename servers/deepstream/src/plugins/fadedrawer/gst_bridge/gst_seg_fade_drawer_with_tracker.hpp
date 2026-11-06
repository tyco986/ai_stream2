#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstSegFadeDrawerWithTracker GstSegFadeDrawerWithTracker;
typedef struct _GstSegFadeDrawerWithTrackerClass GstSegFadeDrawerWithTrackerClass;

#define GST_TYPE_SEG_FADE_DRAWER_WITH_TRACKER (gst_seg_fade_drawer_with_tracker_get_type())
#define GST_SEG_FADE_DRAWER_WITH_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SEG_FADE_DRAWER_WITH_TRACKER, GstSegFadeDrawerWithTracker))

struct _GstSegFadeDrawerWithTracker {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
    gint fadeTime;
    gboolean showLabel;
    gboolean showMask;
    gboolean showSnap;
};

struct _GstSegFadeDrawerWithTrackerClass {
    GstBaseTransformClass parent;
};

GType gst_seg_fade_drawer_with_tracker_get_type(void);

G_END_DECLS
