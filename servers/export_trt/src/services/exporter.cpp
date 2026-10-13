#include "services/exporter.hpp"

#include "constants/exporter.hpp"
#include "constants/onnx_bundle.hpp"

#include <chrono>
#include <cstdlib>
#include <format>
#include <fstream>
#include <stdexcept>

#include <NvInfer.h>
#include <sys/wait.h>

ExportResult Exporter::exportModel(const std::string& fileName, const std::string& fileContent, const nlohmann::json& config) {
    const std::filesystem::path file(std::filesystem::path(fileName).filename());
    const std::filesystem::path directory = exportStart(fileName, fileContent);
    const std::filesystem::path archive = std::filesystem::path(exporter::TMP_DIR) / (file.stem().string() + exporter::ZIP_SUFFIX);
    nlohmann::json result = exporter::RESULT;
    std::ifstream stream(directory / exporter::META_JSON);
    const nlohmann::json meta = nlohmann::json::parse(stream);
    
    result["type"] = config["type"];

    result["precision"] = config["precision"];
    result["opt_level"] = config["opt_level"];

    result["batch_mode"] = meta["batch_mode"];
    result["input_tensor_name"] = meta["input_tensor_name"];
    result["input_tensor_shape"] = meta["input_tensor_shape"];
    result["max_det"] = meta["max_det"];
    result["conf"] = meta["conf"];
    result["iou"] = meta["iou"];
    result["num_class"] = meta["num_class"];
    result["classes"] = meta["classes"];
    const std::filesystem::path source(meta["source_path"].get<std::string>());
    const std::filesystem::path engine = directory / (source.stem().string() + exporter::ENGINE_SUFFIX);
    result["static_plugin"] = exporter::STATIC_PLUGIN[result["type"].get<std::string>()];
    const int status = std::system(trtexecCommand(result, source, engine).c_str());
    const bool failed = status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0;
    const bool missing_engine = !std::filesystem::is_regular_file(engine);
    if (failed || missing_engine) {
        throw std::runtime_error("trtexec failed");
    }
    const std::chrono::zoned_time built_at{std::chrono::current_zone(), std::chrono::system_clock::now()};
    result["status"] = exporter::STATUS_BUILT;
    result["built_at"] = std::format("{:%FT%T%z}", built_at);
    result["trt_version"] = nvinfer1::getInferLibVersion();
    std::ofstream meta_file(directory / exporter::META_JSON);
    meta_file << result.dump();
    meta_file.close();
    if (!meta_file) {
        throw std::runtime_error("meta.json not saved");
    }
    exportEnd(directory, archive);
    ExportResult output;
    output.data = result;
    output.archive = archive;
    return output;
}

std::filesystem::path Exporter::exportStart(const std::string& fileName, const std::string& fileContent) {
    const std::filesystem::path file(std::filesystem::path(fileName).filename());
    const std::filesystem::path directory = std::filesystem::path(exporter::TMP_DIR) / file.stem();
    const std::filesystem::path archive = directory / file;
    std::filesystem::create_directories(directory);
    std::ofstream output(archive, std::ios::binary);
    output.write(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
    output.close();
    const std::string command = "unzip -o '" + archive.string() + "' -d '" + directory.string() + "'";
    const int status = std::system(command.c_str());
    const bool failed = !output || status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0;
    if (failed) {
        throw std::runtime_error("unzip failed");
    }
    return directory;
}

void Exporter::exportEnd(const std::filesystem::path& directory, const std::filesystem::path& archive) {
    const std::string command = "cd '" + directory.string() + "' && zip -j -q '" + archive.string() + "' *" + std::string(exporter::ENGINE_SUFFIX) + " " + exporter::LABELS_TXT + " " + exporter::META_JSON;
    const int status = std::system(command.c_str());
    const bool failed = status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0;
    const bool missing = !std::filesystem::is_regular_file(archive);
    if (failed || missing) {
        throw std::runtime_error("zip failed");
    }
    const std::string remove = "rm -rf '" + directory.string() + "'";
    const int removed = std::system(remove.c_str());
    const bool remove_failed = removed == -1 || !WIFEXITED(removed) || WEXITSTATUS(removed) != 0;
    if (remove_failed) {
        throw std::runtime_error("remove failed");
    }
}

std::string Exporter::trtexecCommand(const nlohmann::json& result, const std::filesystem::path& source, const std::filesystem::path& engine) {
    std::string command = std::string(onnx_bundle::TRTEXEC) + " --onnx=" + source.string() + " --saveEngine=" + engine.string() + " --device=" + std::to_string(result["device"].get<int>()) + " --memPoolSize=workspace:" + std::to_string(result["workspace"].get<int>()) + "M";
    if (!result["opt_level"].is_null()) {
        command += " --builderOptimizationLevel=" + std::to_string(result["opt_level"].get<int>());
    }
    const std::string precision = result["precision"].get<std::string>();
    if (precision != "fp32") {
        command += " --" + precision;
    }
    if (!result["static_plugin"].is_null()) {
        command += " --staticPlugins=" + result["static_plugin"].get<std::string>();
    }
    if (result["batch_mode"] == exporter::BATCH_MODE_DYNAMIC) {
        const std::string name = result["input_tensor_name"].get<std::string>();
        const nlohmann::json& shape = result["input_tensor_shape"];
        std::string min_dims = std::to_string(exporter::MIN_SHAPE);
        std::string opt_dims = std::to_string(result["batch_size"].get<int>());
        for (std::size_t index = 1; index < shape.size(); ++index) {
            const std::string dim = "x" + std::to_string(shape[index].get<int>());
            min_dims += dim;
            opt_dims += dim;
        }
        command += " --minShapes=" + name + ":" + min_dims + " --optShapes=" + name + ":" + opt_dims + " --maxShapes=" + name + ":" + opt_dims;
    }
    return command;
}
