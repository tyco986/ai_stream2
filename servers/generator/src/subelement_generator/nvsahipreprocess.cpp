#include "subelement_generator/nvsahipreprocess.hpp"

#include "constants/subelement_const.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

NvsahipreprocessGenerator::NvsahipreprocessGenerator(
    std::string networkInputShape,
    int targetUniqueIds,
    int tensorDataType,
    std::string tensorName)
    : networkInputShape(std::move(networkInputShape)),
      targetUniqueIds(targetUniqueIds),
      tensorDataType(tensorDataType),
      tensorName(std::move(tensorName)) {
    if (this->tensorDataType < 0 || this->tensorDataType > 5) {
        throw std::invalid_argument("unsupported tensor_data_type");
    }
    std::vector<int> shapeParts;
    std::stringstream shapeStream(this->networkInputShape);
    std::string part;
    while (std::getline(shapeStream, part, ';')) {
        shapeParts.push_back(std::stoi(part));
    }
    if (shapeParts.size() != 4) {
        throw std::invalid_argument("network_input_shape must be B;C;H;W");
    }
    config = {
        "[property]",
        "enable=1",
        "target-unique-ids=" + std::to_string(this->targetUniqueIds),
        "# 0=NCHW, 1=NHWC",
        "network-input-order=0",
        "maintain-aspect-ratio=1",
        "symmetric-padding=1",
        "processing-width=" + std::to_string(shapeParts[3]),
        "processing-height=" + std::to_string(shapeParts[2]),
        "scaling-buf-pool-size=6",
        "tensor-buf-pool-size=6",
        "network-input-shape=" + this->networkInputShape,
        "# 0=RGB, 1=BGR, 2=GRAY",
        "network-color-format=0",
        "# 0=FP32, 1=UINT8, 2=INT8, 3=UINT32, 4=INT32, 5=FP16",
        "tensor-data-type=" + std::to_string(this->tensorDataType),
        "tensor-name=" + this->tensorName,
        "# 0=DEFAULT, 1=CUDA_PINNED, 2=CUDA_DEVICE, 3=CUDA_UNIFIED, 4=SURFACE_ARRAY",
        "scaling-pool-memory-type=0",
        "# 0=Default, 1=GPU, 2=VIC(Jetson)",
        "scaling-pool-compute-hw=0",
        "# 0=Nearest, 1=Bilinear",
        "scaling-filter=0",
        std::string("custom-lib-path=") + subelement::SAHI_CUSTOM_LIB_PATH,
        "custom-tensor-preparation-function=CustomTensorPreparation",
        "",
        "[user-configs]",
        "pixel-normalization-factor=0.003921568",
    };
}

void NvsahipreprocessGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    for (std::size_t index = 0; index < config.size(); ++index) {
        if (index > 0) {
            handle << '\n';
        }
        handle << config[index];
    }
}
