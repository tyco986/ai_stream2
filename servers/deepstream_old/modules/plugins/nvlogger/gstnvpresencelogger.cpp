#include "gstnvpresencelogger.h"

#include "presence_log_engine.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_nvpresencelogger_debug);
#define GST_CAT_DEFAULT gst_nvpresencelogger_debug

#define gst_nvpresencelogger_parent_class parent_class
G_DEFINE_TYPE(GstNvPresenceLogger, gst_nvpresencelogger, GST_TYPE_NVDETLOGGER);

static void
gst_nvpresencelogger_class_init(GstNvPresenceLoggerClass *klass)
{
  GstElementClass *ge = (GstElementClass *)klass;
  gst_element_class_set_details_simple(
      ge, "NvPresenceLogger", "Filter/Metadata",
      "Write detection boxes and presence eventcode as JSON lines", "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(gst_nvpresencelogger_debug, "nvpresencelogger", 0,
                          "nvpresencelogger");
}

static void
gst_nvpresencelogger_init(GstNvPresenceLogger *self)
{
  GstNvDetLogger *parent = GST_NVDETLOGGER(self);
  delete static_cast<nvlogger::DetLogEngine *>(parent->engine);
  parent->engine = new nvlogger::PresenceLogEngine();
}
