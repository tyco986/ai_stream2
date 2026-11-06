#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstRawCapturer GstRawCapturer;
typedef struct _GstRawCapturerClass GstRawCapturerClass;

#define GST_TYPE_RAW_CAPTURER (gst_raw_capturer_get_type())
#define GST_RAW_CAPTURER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RAW_CAPTURER, GstRawCapturer))

struct _GstRawCapturer {
    GstBaseTransform base;
    gpointer engine;
    gchar* captureCodes;
};

struct _GstRawCapturerClass {
    GstBaseTransformClass parent;
};

GType gst_raw_capturer_get_type(void);

G_END_DECLS
