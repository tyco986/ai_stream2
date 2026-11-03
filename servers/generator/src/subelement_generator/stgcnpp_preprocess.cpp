#include "subelement_generator/stgcnpp_preprocess.hpp"

#include "constants/subelement_const.hpp"

#include <fstream>
#include <sstream>
#include <utility>

StgcnppPreprocessGenerator::StgcnppPreprocessGenerator(
    int batchSize,
    int clipLen,
    int numJoints,
    int numPerson,
    std::string tensorName,
    int streamCount,
    int patience)
    : batchSize(batchSize),
      clipLen(clipLen),
      numJoints(numJoints),
      numPerson(numPerson),
      tensorName(std::move(tensorName)),
      streamCount(streamCount),
      patience(patience) {
}

std::string StgcnppPreprocessGenerator::render() const {
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
    text << "# nvdspreprocess for PYSKL ST-GCN++: input (N, M, T, V, C)\n"
         << "# Place after nvrtmposepostprocess. Reads Kx3 keypoints from mask_params.\n"
         << "\n"
         << "[property]\n"
         << "enable=1\n"
         << "unique-id=" << subelement::STGCNPP_UNIQUE_ID << "\n"
         << "target-unique-ids=" << subelement::STGCNPP_TARGET_UNIQUE_IDS << "\n"
         << "network-input-order=2\n"
         << "network-input-shape=" << batchSize << ';' << numPerson << ';' << clipLen << ';' << numJoints << ";3\n"
         << "process-on-frame=0\n"
         << "processing-width=" << numJoints << "\n"
         << "processing-height=" << clipLen << "\n"
         << "scaling-buf-pool-size=6\n"
         << "tensor-buf-pool-size=6\n"
         << "network-color-format=0\n"
         << "tensor-data-type=0\n"
         << "tensor-name=" << tensorName << "\n"
         << "scaling-pool-memory-type=0\n"
         << "scaling-pool-compute-hw=1\n"
         << "scaling-filter=0\n"
         << "custom-lib-path=" << subelement::STGCNPP_CUSTOM_LIB_PATH << "\n"
         << "custom-tensor-preparation-function=CustomTensorPreparation\n"
         << "\n"
         << "[user-configs]\n"
         << "frames-sequence-length=" << clipLen << "\n"
         << "num-joints=" << numJoints << "\n"
         << "num-person=" << numPerson << "\n"
         << "patience=" << patience << "\n"
         << "\n"
         << "[group-0]\n"
         << "src-ids=" << srcIds.str() << "\n"
         << "custom-input-transformation-function=CustomTransformation\n"
         << "process-on-roi=1\n"
         << "process-on-all-objects=1\n"
         << roiLines.str() << "\n"
         << "input-object-min-width=32\n"
         << "input-object-min-height=32\n"
         << "draw-roi=0\n";
    return text.str();
}

void StgcnppPreprocessGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << render();
}
