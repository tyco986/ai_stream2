#include "services/pipeline_service.hpp"

#include "constants/pipeline_const.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

#include <zip.h>

std::optional<PipelineRunner> PipelineService::runner;

bool PipelineService::status() {
    const bool alive = runner.has_value() && runner->status();
    return alive;
}

std::filesystem::path PipelineService::prepare(const httplib::FormData& config, const std::vector<httplib::FormData>& models) {
    std::filesystem::remove_all(pipeline::RUNTIME_ROOT);
    std::filesystem::create_directories(pipeline::RUNTIME_ROOT);
    const bool missing = config.content.empty() || config.filename.empty();
    if (missing) {
        throw std::runtime_error("config zip missing");
    }
    const std::filesystem::path configDir = std::filesystem::path(pipeline::RUNTIME_ROOT) / std::filesystem::path(config.filename).stem();
    extract(config.content, configDir);
    for (const httplib::FormData& model : models) {
        const bool modelMissing = model.content.empty() || model.filename.empty();
        if (modelMissing) {
            throw std::runtime_error("model zip missing");
        }
        const std::filesystem::path modelDir = std::filesystem::path(pipeline::RUNTIME_ROOT) / std::filesystem::path(model.filename).stem();
        extract(model.content, modelDir);
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
    runner.reset();
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
    zip_close(archive);
    zip_error_fini(&error);
}
