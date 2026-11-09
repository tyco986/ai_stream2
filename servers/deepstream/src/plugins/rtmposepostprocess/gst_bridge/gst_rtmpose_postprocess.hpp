#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstRtmposePostprocess GstRtmposePostprocess;
typedef struct _GstRtmposePostprocessClass GstRtmposePostprocessClass;

#define GST_TYPE_RTMPOSE_POSTPROCESS (gst_rtmpose_postprocess_get_type())
#define GST_RTMPOSE_POSTPROCESS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTMPOSE_POSTPROCESS, GstRtmposePostprocess))

struct _GstRtmposePostprocess {
    GstBaseTransform base;
    gpointer engine;
};

struct _GstRtmposePostprocessClass {
    GstBaseTransformClass parent;
};

GType gst_rtmpose_postprocess_get_type(void);

G_END_DECLS
