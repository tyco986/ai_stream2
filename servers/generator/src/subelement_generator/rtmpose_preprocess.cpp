#include "subelement_generator/rtmpose_preprocess.hpp"

#include "constants/subelement_const.hpp"

#include <fstream>
#include <sstream>

NvdspreprocessRtmposeGenerator::NvdspreprocessRtmposeGenerator(
    int batchSize,
    int inferWidth,
    int inferHeight,
    int channels,
    int streamCount,
    std::optional<std::string> tensorName)
    : batchSize(batchSize),
      inferWidth(inferWidth),
      inferHeight(inferHeight),
      channels(channels),
      streamCount(streamCount),
      tensorName(tensorName.value_or(subelement::RTMPOSE_TENSOR_NAME)) {
}

std::string NvdspreprocessRtmposeGenerator::render() const {
    std::ostringstream srcIds;
    std::ostringstream roiLines;
    for (int index = 0; index < streamCount; ++index) {
        if (index > 0) {
            srcIds << ';';
            roiLines << '\n';
        }
        srcIds << index;
        roiLines << "roi-params-src-" << index << "=0;0;100;100";
    }
    std::ostringstream text;
    text << "# nvdspreprocess for RTMPose: expand object crop, letterbox, NCHW RGB.\n"
         << "\n"
         << "[property]\n"
         << "enable=1\n"
         << "unique-id=" << subelement::RTMPOSE_UNIQUE_ID << "\n"
         << "target-unique-ids=" << subelement::RTMPOSE_TARGET_UNIQUE_IDS << "\n"
         << "operate-on-gie-id=" << subelement::RTMPOSE_OPERATE_ON_GIE_ID << "\n"
         << "network-input-order=0\n"
         << "network-input-shape=" << batchSize << ';' << channels << ';' << inferHeight << ';' << inferWidth << "\n"
         << "process-on-frame=0\n"
         << "processing-width=" << inferWidth << "\n"
         << "processing-height=" << inferHeight << "\n"
         << "scaling-buf-pool-size=6\n"
         << "tensor-buf-pool-size=6\n"
         << "network-color-format=0\n"
         << "tensor-data-type=0\n"
         << "tensor-name=" << tensorName << "\n"
         << "scaling-pool-memory-type=0\n"
         << "scaling-pool-compute-hw=1\n"
         << "scaling-filter=0\n"
         << "maintain-aspect-ratio=0\n"
         << "symmetric-padding=0\n"
         << "custom-lib-path=" << subelement::RTMPOSE_CUSTOM_LIB_PATH << "\n"
         << "custom-tensor-preparation-function=CustomTensorPreparation\n"
         << "\n"
         << "[user-configs]\n"
         << "padding=" << subelement::RTMPOSE_PADDING << "\n"
         << "infer-width=" << inferWidth << "\n"
         << "infer-height=" << inferHeight << "\n"
         << "pixel-normalization-factor=" << subelement::RTMPOSE_PIXEL_NORMALIZATION_FACTOR << "\n"
         << "offsets=" << subelement::RTMPOSE_OFFSETS << "\n"
         << "\n"
         << "[group-0]\n"
         << "src-ids=" << srcIds.str() << "\n"
         << "custom-input-transformation-function=CustomTransformation\n"
         << "process-on-roi=1\n"
         << "process-on-all-objects=1\n"
         << roiLines.str() << "\n"
         << "input-object-min-width=32\n"
         << "input-object-min-height=32\n"
         << "operate-on-class-ids=" << subelement::RTMPOSE_OPERATE_ON_CLASS_IDS << "\n"
         << "draw-roi=0\n";
    return text.str();
}

void NvdspreprocessRtmposeGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << render();
}
