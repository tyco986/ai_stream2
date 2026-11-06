#include <gst/gst.h>

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/gst_bridge/gst_det_logger.hpp"
#include "plugins/dslogger/gst_bridge/gst_presence_logger.hpp"
#include "plugins/dslogger/gst_bridge/gst_stgcnpp_logger.hpp"

static gboolean dsloggerPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(plugin, dslogger::ELEMENT_DET, GST_RANK_PRIMARY, GST_TYPE_DET_LOGGER)
        && gst_element_register(plugin, dslogger::ELEMENT_PRESENCE, GST_RANK_PRIMARY, GST_TYPE_PRESENCE_LOGGER)
        && gst_element_register(plugin, dslogger::ELEMENT_STGCNPP, GST_RANK_PRIMARY, GST_TYPE_STGCNPP_LOGGER);
    return registered;
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
