#include "subelement_generator/stgcnpp.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

int StgcnppGenerator::nvinferSkip(int userInterval) {
    if (userInterval < 1) {
        throw std::invalid_argument("interval must be >= 1");
    }
    const int skip = userInterval - 1;
    return skip;
}

StgcnppGenerator::StgcnppGenerator(
    std::string modelEngineFile,
    std::string labelfilePath,
    std::string outputBlobNames,
    int batchSize,
    int networkMode,
    int gpuId,
    int interval,
    std::string tensorName,
    int numPerson,
    int clipLen,
    int numJoints)
    : tensorName(std::move(tensorName)),
      numPerson(numPerson),
      clipLen(clipLen),
      numJoints(numJoints),
      modelEngineFile(std::move(modelEngineFile)),
      labelfilePath(std::move(labelfilePath)),
      outputBlobNames(std::move(outputBlobNames)),
      batchSize(batchSize),
      networkMode(networkMode),
      gpuId(gpuId),
      interval(interval) {
    config["property"] = YAML::Node(YAML::NodeType::Map);
}

void StgcnppGenerator::updateConfig() {
    config["property"]["batch-size"] = batchSize;
    config["property"]["network-mode"] = networkMode;
    config["property"]["gpu-id"] = gpuId;
    config["property"]["interval"] = nvinferSkip(interval);
    config["property"]["model-engine-file"] = modelEngineFile;
    config["property"]["labelfile-path"] = labelfilePath;
    config["property"]["output-blob-names"] = outputBlobNames;
}

void StgcnppGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << YAML::Dump(config);
}
