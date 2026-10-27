#include "services/exporter_service.hpp"

#include "constants/exporter_const.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <stdexcept>

#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>
#include <dlfcn.h>
#include <zip.h>

Exporter::Exporter() : trtLogger() {}

ExportResult Exporter::exportModel(const std::string& fileName, const std::string& fileContent, const nlohmann::json& config) {
    const std::filesystem::path file(std::filesystem::path(fileName).filename());
    const ExportInput input = exportStart(fileContent);
    const std::string fileNameOut = file.stem().string() + exporter::ZIP_SUFFIX;
    nlohmann::json result = exporter::RESULT;
    const nlohmann::json meta = input.meta;
    
    result[exporter::KEY_TYPE] = config[exporter::KEY_TYPE];
    result[exporter::KEY_BATCH_SIZE] = config[exporter::KEY_BATCH_SIZE];
    result[exporter::KEY_PRECISION] = config[exporter::KEY_PRECISION];
    result[exporter::KEY_OPT_LEVEL] = config[exporter::KEY_OPT_LEVEL];
    result[exporter::KEY_DEVICE] = config[exporter::KEY_DEVICE];

    result[exporter::KEY_BATCH_MODE] = meta[exporter::KEY_BATCH_MODE];
    result[exporter::KEY_INPUT_TENSORS] = meta[exporter::KEY_INPUT_TENSORS];
    result[exporter::KEY_OUTPUT_TENSORS] = meta[exporter::KEY_OUTPUT_TENSORS];
    result[exporter::KEY_MAX_DET] = meta[exporter::KEY_MAX_DET];
    result[exporter::KEY_CONF] = meta[exporter::KEY_CONF];
    result[exporter::KEY_IOU] = meta[exporter::KEY_IOU];
    result[exporter::KEY_NUM_CLASS] = meta[exporter::KEY_NUM_CLASS];
    result[exporter::KEY_CLASSES] = meta[exporter::KEY_CLASSES];
    const std::filesystem::path source(meta["source_path"].get<std::string>());
    const std::string engineName = source.stem().string() + exporter::ENGINE_SUFFIX;
    result[exporter::KEY_STATIC_PLUGIN] = exporter::STATIC_PLUGIN[result[exporter::KEY_TYPE].get<std::string>()];
    const std::string built = exportBuild(input, result);
    const std::chrono::zoned_time built_at{std::chrono::current_zone(), std::chrono::system_clock::now()};
    result[exporter::KEY_STATUS] = exporter::STATUS_BUILT;
    result[exporter::KEY_BUILT_AT] = std::format("{:%FT%T%z}", built_at);
    result[exporter::KEY_TRT_VERSION] = getInferLibVersion();
    const std::string packed = exportEnd(engineName, built, input.labels, result.dump());
    ExportResult output;
    output.data = result;
    output.fileName = fileNameOut;
    output.file = packed;
    return output;
}

ExportInput Exporter::exportStart(const std::string& fileContent) {
    zip_error_t zipError;
    zip_error_init(&zipError);
    zip_source_t* source = zip_source_buffer_create(fileContent.data(), fileContent.size(), 0, &zipError);
    zip_t* zip = source ? zip_open_from_source(source, ZIP_RDONLY, &zipError) : nullptr;
    std::string onnxBytes;
    std::string labelsBytes;
    std::string metaBytes;
    const zip_int64_t count = zip ? zip_get_num_entries(zip, 0) : 0;
    for (zip_int64_t index = 0; index < count; ++index) {
        const std::string name = zip_get_name(zip, index, 0);
        const bool directoryEntry = !name.empty() && name.back() == '/';
        zip_stat_t stat;
        const int stated = directoryEntry ? -1 : zip_stat_index(zip, index, 0, &stat);
        const bool sized = stated == 0 && (stat.valid & ZIP_STAT_SIZE) != 0;
        zip_file_t* entry = sized ? zip_fopen_index(zip, index, 0) : nullptr;
        std::string bytes(sized ? static_cast<std::size_t>(stat.size) : 0, '\0');
        const zip_int64_t read = entry ? zip_fread(entry, bytes.data(), bytes.size()) : -1;
        if (entry) {
            zip_fclose(entry);
        }
        const std::string fileName = std::filesystem::path(name).filename().string();
        const bool onnx = read == static_cast<zip_int64_t>(bytes.size()) && fileName.ends_with(exporter::ONNX_SUFFIX);
        const bool labels = read == static_cast<zip_int64_t>(bytes.size()) && fileName == exporter::LABELS_TXT;
        const bool meta = read == static_cast<zip_int64_t>(bytes.size()) && fileName == exporter::META_JSON;
        onnxBytes = onnx ? bytes : onnxBytes;
        labelsBytes = labels ? bytes : labelsBytes;
        metaBytes = meta ? bytes : metaBytes;
    }
    if (zip) {
        zip_close(zip);
    }
    zip_error_fini(&zipError);
    const bool accepted = zip && !onnxBytes.empty() && !labelsBytes.empty() && !metaBytes.empty();
    if (!accepted) {
        throw std::runtime_error("unzip failed");
    }
    ExportInput input;
    input.onnx = onnxBytes;
    input.labels = labelsBytes;
    input.meta = nlohmann::json::parse(metaBytes);
    return input;
}

std::string Exporter::exportEnd(const std::string& engineName, const std::string& engine, const std::string& labels, const std::string& meta) {
    zip_error_t zipError;
    zip_error_init(&zipError);
    zip_source_t* source = zip_source_buffer_create(nullptr, 0, 1, &zipError);
    if (source) {
        zip_source_keep(source);
    }
    zip_t* zip = source ? zip_open_from_source(source, ZIP_CREATE, &zipError) : nullptr;
    zip_source_t* engineSource = zip ? zip_source_buffer(zip, engine.data(), engine.size(), 0) : nullptr;
    zip_source_t* labelsSource = zip ? zip_source_buffer(zip, labels.data(), labels.size(), 0) : nullptr;
    zip_source_t* metaSource = zip ? zip_source_buffer(zip, meta.data(), meta.size(), 0) : nullptr;
    const zip_int64_t engineAdded = engineSource ? zip_file_add(zip, engineName.c_str(), engineSource, ZIP_FL_ENC_UTF_8) : -1;
    const zip_int64_t labelsAdded = labelsSource ? zip_file_add(zip, exporter::LABELS_TXT, labelsSource, ZIP_FL_ENC_UTF_8) : -1;
    const zip_int64_t metaAdded = metaSource ? zip_file_add(zip, exporter::META_JSON, metaSource, ZIP_FL_ENC_UTF_8) : -1;
    const int closed = zip && engineAdded >= 0 && labelsAdded >= 0 && metaAdded >= 0 ? zip_close(zip) : -1;
    if (closed != 0 && zip) {
        zip_discard(zip);
    }
    zip_stat_t stat;
    const int stated = closed == 0 ? zip_source_stat(source, &stat) : -1;
    const bool sized = stated == 0 && (stat.valid & ZIP_STAT_SIZE) != 0;
    std::string bytes(sized ? static_cast<std::size_t>(stat.size) : 0, '\0');
    const int opened = sized ? zip_source_open(source) : -1;
    const zip_int64_t read = opened == 0 ? zip_source_read(source, bytes.data(), bytes.size()) : -1;
    if (opened == 0) {
        zip_source_close(source);
    }
    if (source) {
        zip_source_free(source);
    }
    zip_error_fini(&zipError);
    const bool packed = read == static_cast<zip_int64_t>(bytes.size()) && !bytes.empty();
    if (!packed) {
        throw std::runtime_error("zip failed");
    }
    return bytes;
}

std::string Exporter::exportBuild(const ExportInput& input, const nlohmann::json& result) {
    const cudaError_t device = cudaSetDevice(result[exporter::KEY_DEVICE].get<int>());
    const bool pluginRequested = !result[exporter::KEY_STATIC_PLUGIN].is_null();
    void* plugin = pluginRequested ? dlopen(result[exporter::KEY_STATIC_PLUGIN].get<std::string>().c_str(), RTLD_NOW | RTLD_GLOBAL) : nullptr;
    nvinfer1::IBuilder* builder = device == cudaSuccess ? nvinfer1::createInferBuilder(trtLogger) : nullptr;
    nvinfer1::INetworkDefinition* network = builder ? builder->createNetworkV2(0) : nullptr;
    nvonnxparser::IParser* parser = network ? nvonnxparser::createParser(*network, trtLogger) : nullptr;
    const bool parsed = parser && parser->parse(input.onnx.data(), input.onnx.size());
    nvinfer1::IBuilderConfig* config = parsed ? builder->createBuilderConfig() : nullptr;
    if (config) {
        config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, static_cast<std::size_t>(result[exporter::KEY_WORKSPACE].get<int>()) << 20);
    }
    const bool fp16 = result[exporter::KEY_PRECISION].get<std::string>() == "fp16";
    if (config && fp16) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
    }
    if (config && !result[exporter::KEY_OPT_LEVEL].is_null()) {
        config->setBuilderOptimizationLevel(result[exporter::KEY_OPT_LEVEL].get<int>());
    }
    const bool dynamic = result[exporter::KEY_BATCH_MODE] == exporter::BATCH_MODE_DYNAMIC;
    const nlohmann::json inputs = dynamic ? result[exporter::KEY_INPUT_TENSORS] : nlohmann::json::object();
    nvinfer1::IOptimizationProfile* profile = dynamic && builder ? builder->createOptimizationProfile() : nullptr;
    bool profileSet = profile && inputs.is_object() && !inputs.empty();
    for (const auto& inputTensor : inputs.items()) {
        const std::string name = inputTensor.key();
        const nlohmann::json& shape = inputTensor.value();
        nvinfer1::ITensor* tensor = nullptr;
        for (int index = 0; network && index < network->getNbInputs(); ++index) {
            nvinfer1::ITensor* item = network->getInput(index);
            tensor = item->getName() == name ? item : tensor;
        }
        nvinfer1::Dims minDims{};
        nvinfer1::Dims optDims{};
        minDims.nbDims = static_cast<int32_t>(shape.size());
        optDims.nbDims = minDims.nbDims;
        for (std::size_t index = 0; tensor && index < shape.size(); ++index) {
            const int dim = shape[index].get<int>();
            minDims.d[index] = index == 0 ? exporter::MIN_SHAPE : dim;
            optDims.d[index] = index == 0 ? result[exporter::KEY_BATCH_SIZE].get<int>() : dim;
        }
        if (tensor) {
            nvinfer1::Dims networkDims = optDims;
            networkDims.d[0] = -1;
            tensor->setDimensions(networkDims);
        }
        const bool set = tensor && profile &&
            profile->setDimensions(tensor->getName(), nvinfer1::OptProfileSelector::kMIN, minDims) &&
            profile->setDimensions(tensor->getName(), nvinfer1::OptProfileSelector::kOPT, optDims) &&
            profile->setDimensions(tensor->getName(), nvinfer1::OptProfileSelector::kMAX, optDims);
        profileSet = profileSet && set;
    }
    if (config && profileSet) {
        config->addOptimizationProfile(profile);
    }
    nvinfer1::IHostMemory* serialized = config && (!dynamic || profileSet) && (!pluginRequested || plugin) ? builder->buildSerializedNetwork(*network, *config) : nullptr;
    const std::string engine = serialized ? std::string(static_cast<const char*>(serialized->data()), serialized->size()) : "";
    delete serialized;
    delete config;
    delete parser;
    delete network;
    delete builder;
    if (engine.empty()) {
        throw std::runtime_error("trtexec failed");
    }
    return engine;
}

std::string Exporter::trtexecCommand(const nlohmann::json& result, const std::filesystem::path& source, const std::filesystem::path& engine) {
    std::string command = std::string(exporter::TRTEXEC) + " --onnx=" + source.string() + " --saveEngine=" + engine.string() + " --device=" + std::to_string(result[exporter::KEY_DEVICE].get<int>()) + " --memPoolSize=workspace:" + std::to_string(result[exporter::KEY_WORKSPACE].get<int>()) + "M";
    if (!result[exporter::KEY_OPT_LEVEL].is_null()) {
        command += " --builderOptimizationLevel=" + std::to_string(result[exporter::KEY_OPT_LEVEL].get<int>());
    }
    const std::string precision = result[exporter::KEY_PRECISION].get<std::string>();
    if (precision != "fp32") {
        command += " --" + precision;
    }
    if (!result[exporter::KEY_STATIC_PLUGIN].is_null()) {
        command += " --staticPlugins=" + result[exporter::KEY_STATIC_PLUGIN].get<std::string>();
    }
    if (result[exporter::KEY_BATCH_MODE] == exporter::BATCH_MODE_DYNAMIC) {
        std::string minShapes;
        std::string optShapes;
        std::string maxShapes;
        for (const auto& inputTensor : result[exporter::KEY_INPUT_TENSORS].items()) {
            const std::string name = inputTensor.key();
            const nlohmann::json& shape = inputTensor.value();
            std::string minDims = std::to_string(exporter::MIN_SHAPE);
            std::string optDims = std::to_string(result[exporter::KEY_BATCH_SIZE].get<int>());
            for (std::size_t index = 1; index < shape.size(); ++index) {
                const std::string dim = "x" + std::to_string(shape[index].get<int>());
                minDims += dim;
                optDims += dim;
            }
            const std::string separator = minShapes.empty() ? "" : ",";
            minShapes += separator + name + ":" + minDims;
            optShapes += separator + name + ":" + optDims;
            maxShapes += separator + name + ":" + optDims;
        }
        command += " --minShapes=" + minShapes + " --optShapes=" + optShapes + " --maxShapes=" + maxShapes;
    }
    return command;
}
