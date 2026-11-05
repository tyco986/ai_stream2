#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvSegFadeDrawer GstNvSegFadeDrawer;
typedef struct _GstNvSegFadeDrawerClass GstNvSegFadeDrawerClass;

#define GST_TYPE_NVSEGFADEDRAWER (gst_nvsegfadedrawer_get_type())
#define GST_NVSEGFADEDRAWER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVSEGFADEDRAWER, GstNvSegFadeDrawer))

struct _GstNvSegFadeDrawer
{
  GstBaseTransform base_trans;
  gpointer engine;
};

struct _GstNvSegFadeDrawerClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvsegfadedrawer_get_type(void);

G_END_DECLS
