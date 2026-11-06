#include "constants/capturer_const.hpp"
#include "plugins/capturer/gst_bridge/gst_raw_capturer.hpp"
#include "plugins/capturer/gst_bridge/gst_vis_capturer.hpp"

#include <gst/gst.h>

static gboolean capturerPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(plugin, capturer::ELEMENT_RAW, GST_RANK_PRIMARY, GST_TYPE_RAW_CAPTURER)
        && gst_element_register(plugin, capturer::ELEMENT_VIS, GST_RANK_PRIMARY, GST_TYPE_VIS_CAPTURER);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  capturer,
                  capturer::PLUGIN_DESCRIPTION,
                  capturerPluginInit,
                  capturer::VERSION,
                  capturer::LICENSE,
                  capturer::PLUGIN_PACKAGE,
                  capturer::ORIGIN)
