#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvDetFadeDrawer GstNvDetFadeDrawer;
typedef struct _GstNvDetFadeDrawerClass GstNvDetFadeDrawerClass;

#define GST_TYPE_NVDETFADEDRAWER (gst_nvdetfadedrawer_get_type())
#define GST_NVDETFADEDRAWER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVDETFADEDRAWER, GstNvDetFadeDrawer))

struct _GstNvDetFadeDrawer
{
  GstBaseTransform base_trans;
  gpointer engine;
};

struct _GstNvDetFadeDrawerClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvdetfadedrawer_get_type(void);

G_END_DECLS
