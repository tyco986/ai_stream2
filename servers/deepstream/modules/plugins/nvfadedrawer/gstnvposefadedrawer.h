#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvPoseFadeDrawer GstNvPoseFadeDrawer;
typedef struct _GstNvPoseFadeDrawerClass GstNvPoseFadeDrawerClass;

#define GST_TYPE_NVPOSEFADEDRAWER (gst_nvposefadedrawer_get_type())
#define GST_NVPOSEFADEDRAWER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVPOSEFADEDRAWER, GstNvPoseFadeDrawer))

struct _GstNvPoseFadeDrawer
{
  GstBaseTransform base_trans;
  gpointer engine;
  gchar *mode;
};

struct _GstNvPoseFadeDrawerClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvposefadedrawer_get_type(void);

G_END_DECLS
