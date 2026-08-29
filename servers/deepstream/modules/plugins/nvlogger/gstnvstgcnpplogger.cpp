#include "gstnvstgcnpplogger.h"

#include "stgcnpp_log_engine.hpp"

#define gst_nvstgcnpplogger_parent_class parent_class
G_DEFINE_TYPE(GstNvStgcnppLogger, gst_nvstgcnpplogger, GST_TYPE_NVDETLOGGER);

static void
gst_nvstgcnpplogger_class_init(GstNvStgcnppLoggerClass *klass)
{
  GstElementClass *ge = (GstElementClass *)klass;
  gst_element_class_set_details_simple(
      ge, "NvStgcnppLogger", "Filter/Metadata",
      "Write detection and ST-GCN++ action fields as JSON lines", "ai_stream2");
}

static void
gst_nvstgcnpplogger_init(GstNvStgcnppLogger *self)
{
  GstNvDetLogger *base = GST_NVDETLOGGER(self);
  delete static_cast<nvlogger::DetLogEngine *>(base->engine);
  base->engine = new nvlogger::StgcnppLogEngine();
}
