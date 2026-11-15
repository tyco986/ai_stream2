#include <gst/gst.h>

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/gst_bridge/gst_ds_logger.hpp"

static gboolean dsloggerPluginInit(GstPlugin* plugin) {
    return gst_element_register(plugin, dslogger::ELEMENT_NAME, GST_RANK_PRIMARY, GST_TYPE_DS_LOGGER);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  dslogger,
                  dslogger::PLUGIN_DESCRIPTION,
                  dsloggerPluginInit,
                  dslogger::VERSION,
                  dslogger::LICENSE,
                  dslogger::PLUGIN_PACKAGE,
                  dslogger::ORIGIN)
