#include "constants/fadedrawer_const.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_det_fade_drawer.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_det_fade_drawer_with_tracker.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_pose_fade_drawer.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_pose_fade_drawer_with_tracker.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_seg_fade_drawer.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_seg_fade_drawer_with_tracker.hpp"
#include "plugins/fadedrawer/gst_bridge/gst_stgcnpp_fade_drawer_with_tracker.hpp"

#include <gst/gst.h>

static gboolean fadedrawerPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(plugin, fadedrawer::ELEMENT_DET, GST_RANK_PRIMARY, GST_TYPE_DET_FADE_DRAWER)
        && gst_element_register(plugin, fadedrawer::ELEMENT_DET_TRACKER, GST_RANK_PRIMARY, GST_TYPE_DET_FADE_DRAWER_WITH_TRACKER)
        && gst_element_register(plugin, fadedrawer::ELEMENT_SEG, GST_RANK_PRIMARY, GST_TYPE_SEG_FADE_DRAWER)
        && gst_element_register(plugin, fadedrawer::ELEMENT_SEG_TRACKER, GST_RANK_PRIMARY, GST_TYPE_SEG_FADE_DRAWER_WITH_TRACKER)
        && gst_element_register(plugin, fadedrawer::ELEMENT_POSE, GST_RANK_PRIMARY, GST_TYPE_POSE_FADE_DRAWER)
        && gst_element_register(plugin, fadedrawer::ELEMENT_POSE_TRACKER, GST_RANK_PRIMARY, GST_TYPE_POSE_FADE_DRAWER_WITH_TRACKER)
        && gst_element_register(plugin, fadedrawer::ELEMENT_STGCNPP_TRACKER, GST_RANK_PRIMARY, GST_TYPE_STGCNPP_FADE_DRAWER_WITH_TRACKER);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  fadedrawer,
                  fadedrawer::PLUGIN_DESCRIPTION,
                  fadedrawerPluginInit,
                  fadedrawer::VERSION,
                  fadedrawer::LICENSE,
                  fadedrawer::PLUGIN_PACKAGE,
                  fadedrawer::ORIGIN)
