#pragma once

#include <map>
#include <string>

namespace generator_pipeline {

inline const std::map<std::string, std::string> GENERATOR_PIPELINE_MAP = {
    {"DetImageGenerator", "DetImagePipeline"},
    {"DetSahiImageGenerator", "DetSahiImagePipeline"},
    {"SegSahiImageGenerator", "SegSahiImagePipeline"},
    {"SegSahiVideoGenerator", "SegSahiVideoPipeline"},
    {"SegSahiVisVideoGenerator", "SegSahiVisVideoPipeline"},
    {"DetVideoGenerator", "DetVideoPipeline"},
    {"DetVisVideoGenerator", "DetVisVideoPipeline"},
    {"DetVisVideoPresenceGenerator", "PresenceVisVideoPipeline"},
    {"DetSahiVisVideoPresenceGenerator", "PresenceSahiVisVideoPipeline"},
    {"DetImagePresenceGenerator", "PresenceImagePipeline"},
    {"DetVideoPresenceGenerator", "PresenceVideoPipeline"},
    {"DetRTSPPresenceGenerator", "PresenceRTSPPipeline"},
    {"DetSahiRTSPPresenceGenerator", "PresenceSahiRTSPPipeline"},
    {"DetVisRTSPPresenceGenerator", "PresenceVisRTSPPipeline"},
    {"DetSahiVisRTSPPresenceGenerator", "PresenceSahiVisRTSPPipeline"},
    {"SegVideoGenerator", "SegVideoPipeline"},
    {"SegVisVideoGenerator", "SegVisVideoPipeline"},
    {"DetSahiVideoGenerator", "DetSahiVideoPipeline"},
    {"DetSahiVisVideoGenerator", "DetSahiVisVideoPipeline"},
    {"SegImageGenerator", "SegImagePipeline"},
    {"DetRTSPGenerator", "DetRTSPPipeline"},
    {"DetVisRTSPGenerator", "DetVisRTSPPipeline"},
    {"SegRTSPGenerator", "SegRTSPPipeline"},
    {"SegVisRTSPGenerator", "SegVisRTSPPipeline"},
    {"DetSahiRTSPGenerator", "DetSahiRTSPPipeline"},
    {"DetSahiVisRTSPGenerator", "DetSahiVisRTSPPipeline"},
    {"SegSahiRTSPGenerator", "SegSahiRTSPPipeline"},
    {"SegSahiVisRTSPGenerator", "SegSahiVisRTSPPipeline"},
    {"PoseImageGenerator", "PoseImagePipeline"},
    {"PoseVideoGenerator", "PoseVideoPipeline"},
    {"PoseVisVideoGenerator", "PoseVisVideoPipeline"},
    {"PoseRTSPGenerator", "PoseRTSPPipeline"},
    {"PoseVisRTSPGenerator", "PoseVisRTSPPipeline"},
    {"PoseSahiImageGenerator", "PoseSahiImagePipeline"},
    {"PoseSahiVideoGenerator", "PoseSahiVideoPipeline"},
    {"PoseSahiVisVideoGenerator", "PoseSahiVisVideoPipeline"},
    {"PoseSahiRTSPGenerator", "PoseSahiRTSPPipeline"},
    {"PoseSahiVisRTSPGenerator", "PoseSahiVisRTSPPipeline"},
    {"TopdownPoseImageGenerator", "TopdownPoseImagePipeline"},
    {"TopdownPoseVideoGenerator", "TopdownPoseVideoPipeline"},
    {"TopdownPoseVisVideoGenerator", "TopdownPoseVisVideoPipeline"},
    {"TopdownPoseRTSPGenerator", "TopdownPoseRTSPPipeline"},
    {"TopdownPoseVisRTSPGenerator", "TopdownPoseVisRTSPPipeline"},
    {"TopdownPoseSahiImageGenerator", "TopdownPoseSahiImagePipeline"},
    {"TopdownPoseSahiVideoGenerator", "TopdownPoseSahiVideoPipeline"},
    {"TopdownPoseSahiVisVideoGenerator", "TopdownPoseSahiVisVideoPipeline"},
    {"TopdownPoseSahiRTSPGenerator", "TopdownPoseSahiRTSPPipeline"},
    {"TopdownPoseSahiVisRTSPGenerator", "TopdownPoseSahiVisRTSPPipeline"},
    {"StgcnppRtmposeVideoGenerator", "StgcnppRtmposeVideoPipeline"},
    {"StgcnppRtmposeVisVideoGenerator", "StgcnppRtmposeVisVideoPipeline"},
    {"StgcnppRtmposeRTSPGenerator", "StgcnppRtmposeRTSPPipeline"},
    {"StgcnppRtmposeVisRTSPGenerator", "StgcnppRtmposeVisRTSPPipeline"},
    {"StgcnppRtmposeSahiVideoGenerator", "StgcnppRtmposeSahiVideoPipeline"},
    {"StgcnppRtmposeSahiVisVideoGenerator", "StgcnppRtmposeSahiVisVideoPipeline"},
    {"StgcnppRtmposeSahiRTSPGenerator", "StgcnppRtmposeSahiRTSPPipeline"},
    {"StgcnppRtmposeSahiVisRTSPGenerator", "StgcnppRtmposeSahiVisRTSPPipeline"},
    {"StgcnppYoloPoseVideoGenerator", "StgcnppYoloPoseVideoPipeline"},
    {"StgcnppYoloPoseVisVideoGenerator", "StgcnppYoloPoseVisVideoPipeline"},
    {"StgcnppYoloPoseRTSPGenerator", "StgcnppYoloPoseRTSPPipeline"},
    {"StgcnppYoloPoseVisRTSPGenerator", "StgcnppYoloPoseVisRTSPPipeline"},
    {"StgcnppYoloPoseSahiVideoGenerator", "StgcnppYoloPoseSahiVideoPipeline"},
    {"StgcnppYoloPoseSahiVisVideoGenerator", "StgcnppYoloPoseSahiVisVideoPipeline"},
    {"StgcnppYoloPoseSahiRTSPGenerator", "StgcnppYoloPoseSahiRTSPPipeline"},
    {"StgcnppYoloPoseSahiVisRTSPGenerator", "StgcnppYoloPoseSahiVisRTSPPipeline"},
};

inline std::map<std::string, std::string> invertMap(
    const std::map<std::string, std::string>& source) {
    std::map<std::string, std::string> inverted;
    for (const auto& item : source) {
        inverted.emplace(item.second, item.first);
    }
    return inverted;
}

inline const std::map<std::string, std::string> PIPELINE_GENERATOR_MAP =
    invertMap(GENERATOR_PIPELINE_MAP);

}
