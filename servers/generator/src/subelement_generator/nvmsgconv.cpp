#include "subelement_generator/nvmsgconv.hpp"

#include <fstream>
#include <utility>

NvmsgconvGenerator::NvmsgconvGenerator(std::vector<std::string> streamNames)
    : streamNames(std::move(streamNames)) {
    for (std::size_t index = 0; index < this->streamNames.size(); ++index) {
        const std::string section = "sensor" + std::to_string(index);
        config[section]["enable"] = 1;
        config[section]["type"] = "Camera";
        config[section]["id"] = "stream_" + std::to_string(index);
        config[section]["description"] = this->streamNames[index];
    }
}

void NvmsgconvGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << YAML::Dump(config);
}
