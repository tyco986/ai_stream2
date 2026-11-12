#include "services/pipeline_service.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/pipeline_runner_const.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

#include <zip.h>

std::optional<PipelineRunner> PipelineService::runner;

PipelineStatus PipelineService::status() {
    PipelineStatus idle;
    idle.status = pipeline::STATUS_STOPPED;
    return runner.has_value() ? runner->status() : idle;
}

std::filesystem::path PipelineService::prepare(const httplib::FormData& config,
                                                 const std::vector<httplib::FormData>& models,
                                                 const httplib::FormData& media) {
    std::filesystem::remove_all(pipeline::RUNTIME_ROOT);
    std::filesystem::create_directories(pipeline::RUNTIME_ROOT);
    const bool missing = config.content.empty() || config.filename.empty();
    if (missing) {
        throw std::runtime_error("config zip missing");
    }
    const std::filesystem::path configDir = pipeline::CONFIG_DIR;
    extract(config.content, configDir);
    for (const httplib::FormData& model : models) {
        const bool modelMissing = model.content.empty() || model.filename.empty();
        if (modelMissing) {
            throw std::runtime_error("model zip missing");
        }
        const std::filesystem::path modelDir = std::filesystem::path(pipeline::MODELS_DIR) / std::filesystem::path(model.filename).stem();
        extract(model.content, modelDir);
    }
    if (!media.content.empty()) {
        extractMedia(media.content);
    }
    return configDir;
}

std::filesystem::path PipelineService::prepareLocal(const std::string& configPath) {
    std::filesystem::remove_all(pipeline::RUNTIME_ROOT);
    std::filesystem::create_directories(pipeline::RUNTIME_ROOT);
    const bool missing = !std::filesystem::is_regular_file(configPath);
    if (missing) {
        throw std::runtime_error("config zip missing");
    }
    const std::filesystem::path configDir = pipeline::CONFIG_DIR;
    extractPath(configPath, configDir);
    const std::filesystem::path paramsPath = configDir / pipeline_runner::PARAMS_FILE;
    const bool paramsMissing = !std::filesystem::is_regular_file(paramsPath);
    if (paramsMissing) {
        throw std::runtime_error("params.yml missing");
    }
    const YAML::Node params = YAML::LoadFile(paramsPath.string());
    std::vector<std::string> modelPaths;
    collectModelZips(params, modelPaths);
    for (const std::string& path : modelPaths) {
        const bool modelMissing = !std::filesystem::is_regular_file(path);
        if (modelMissing) {
            throw std::runtime_error("model zip missing");
        }
        const std::filesystem::path modelDir = std::filesystem::path(pipeline::MODELS_DIR) / std::filesystem::path(path).stem();
        extractPath(path, modelDir);
    }
    return configDir;
}

void PipelineService::start(const std::filesystem::path& configDir) {
    runner.emplace(configDir.string());
    runner->run();
}

void PipelineService::stop() {
    if (runner.has_value()) {
        runner->stop();
    }
}

void PipelineService::extract(const std::string& bytes, const std::filesystem::path& dest) {
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t* source = zip_source_buffer_create(bytes.data(), bytes.size(), 0, &error);
    zip_t* archive = source == nullptr ? nullptr : zip_open_from_source(source, ZIP_RDONLY, &error);
    if (source == nullptr || archive == nullptr) {
        const std::string message = zip_error_strerror(&error);
        if (source != nullptr && archive == nullptr) {
            zip_source_free(source);
        }
        zip_error_fini(&error);
        throw std::runtime_error(message);
    }
    writeEntries(archive, dest, error);
    zip_close(archive);
    zip_error_fini(&error);
}

void PipelineService::extractPath(const std::string& path, const std::filesystem::path& dest) {
    int code = 0;
    zip_error_t error;
    zip_t* archive = zip_open(path.c_str(), ZIP_RDONLY, &code);
    zip_error_init_with_code(&error, code);
    if (archive == nullptr) {
        const std::string message = zip_error_strerror(&error);
        zip_error_fini(&error);
        throw std::runtime_error(message);
    }
    writeEntries(archive, dest, error);
    zip_close(archive);
    zip_error_fini(&error);
}

void PipelineService::writeEntries(zip_t* archive, const std::filesystem::path& dest, zip_error_t& error) {
    std::filesystem::create_directories(dest);
    const zip_int64_t count = zip_get_num_entries(archive, 0);
    std::vector<char> buffer(pipeline::READ_BUFFER);
    for (zip_int64_t index = 0; index < count; ++index) {
        const char* entryName = zip_get_name(archive, index, 0);
        const std::string name = entryName == nullptr ? "" : entryName;
        const bool directoryEntry = name.empty() || name.back() == '/';
        zip_file_t* entry = directoryEntry ? nullptr : zip_fopen_index(archive, index, 0);
        if (!directoryEntry && entry == nullptr) {
            zip_discard(archive);
            zip_error_fini(&error);
            throw std::runtime_error("failed to read zip entry: " + name);
        }
        const std::filesystem::path path = dest / std::filesystem::path(name).filename();
        std::ofstream output;
        if (entry != nullptr) {
            output.open(path, std::ios::binary);
        }
        zip_int64_t read = entry == nullptr ? 0 : zip_fread(entry, buffer.data(), buffer.size());
        while (entry != nullptr && read > 0) {
            output.write(buffer.data(), read);
            read = zip_fread(entry, buffer.data(), buffer.size());
        }
        const bool shortRead = entry != nullptr && read < 0;
        if (entry != nullptr) {
            zip_fclose(entry);
        }
        if (shortRead || (entry != nullptr && !output)) {
            zip_discard(archive);
            zip_error_fini(&error);
            throw std::runtime_error("failed to write " + path.string());
        }
    }
}

void PipelineService::collectModelZips(const YAML::Node& node, std::vector<std::string>& paths) {
    const bool map = node && node.IsMap();
    const bool sequence = node && node.IsSequence();
    if (map) {
        for (const auto& item : node) {
            const bool modelZip = item.first.as<std::string>() == pipeline::KEY_MODEL_ZIP && item.second.IsScalar();
            if (modelZip) {
                paths.push_back(item.second.as<std::string>());
            }
            collectModelZips(item.second, paths);
        }
    }
    if (sequence) {
        for (const auto& item : node) {
            collectModelZips(item, paths);
        }
    }
}

void PipelineService::extractMedia(const std::string& bytes) {
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t* source = zip_source_buffer_create(bytes.data(), bytes.size(), 0, &error);
    zip_t* archive = source == nullptr ? nullptr : zip_open_from_source(source, ZIP_RDONLY, &error);
    if (source == nullptr || archive == nullptr) {
        const std::string message = zip_error_strerror(&error);
        if (source != nullptr && archive == nullptr) {
            zip_source_free(source);
        }
        zip_error_fini(&error);
        throw std::runtime_error(message);
    }
    const zip_int64_t count = zip_get_num_entries(archive, 0);
    std::vector<char> buffer(pipeline::READ_BUFFER);
    for (zip_int64_t index = 0; index < count; ++index) {
        const char* entryName = zip_get_name(archive, index, 0);
        const std::string name = entryName == nullptr ? "" : entryName;
        const std::filesystem::path relative(name);
        bool safe = !name.empty() && name.back() != '/' && !relative.is_absolute();
        for (const std::filesystem::path& part : relative) {
            safe = safe && part != ".." && part != ".";
        }
        zip_file_t* entry = safe ? zip_fopen_index(archive, index, 0) : nullptr;
        if (!safe || entry == nullptr) {
            zip_discard(archive);
            zip_error_fini(&error);
            throw std::runtime_error("failed to read zip entry: " + name);
        }
        const std::filesystem::path path = std::filesystem::path("/") / relative;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary);
        zip_int64_t read = zip_fread(entry, buffer.data(), buffer.size());
        while (read > 0) {
            output.write(buffer.data(), read);
            read = zip_fread(entry, buffer.data(), buffer.size());
        }
        const bool shortRead = read < 0;
        zip_fclose(entry);
        if (shortRead || !output) {
            zip_discard(archive);
            zip_error_fini(&error);
            throw std::runtime_error("failed to write " + path.string());
        }
    }
    zip_close(archive);
    zip_error_fini(&error);
}
