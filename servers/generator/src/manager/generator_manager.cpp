#include "manager/generator_manager.hpp"

#include "constants/generator_manager_const.hpp"
#include "event_generator/det_image_presence.hpp"
#include "event_generator/det_rtsp_presence.hpp"
#include "event_generator/det_sahi_rtsp_presence.hpp"
#include "event_generator/det_sahi_vis_rtsp_presence.hpp"
#include "event_generator/det_sahi_vis_video_presence.hpp"
#include "event_generator/det_video_presence.hpp"
#include "event_generator/det_vis_rtsp_presence.hpp"
#include "event_generator/det_vis_video_presence.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_rtsp.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_rtsp.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_video.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_vis_rtsp.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_vis_video.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_video.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_vis_rtsp.hpp"
#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_vis_video.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_rtsp.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_rtsp.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_video.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_vis_rtsp.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_vis_video.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_video.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_vis_rtsp.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_vis_video.hpp"
#include "topdown_pose_generator/topdown_pose_image.hpp"
#include "topdown_pose_generator/topdown_pose_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose_sahi_image.hpp"
#include "topdown_pose_generator/topdown_pose_sahi_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose_sahi_video.hpp"
#include "topdown_pose_generator/topdown_pose_sahi_vis_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose_sahi_vis_video.hpp"
#include "topdown_pose_generator/topdown_pose_video.hpp"
#include "topdown_pose_generator/topdown_pose_vis_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose_vis_video.hpp"
#include "yolo_generator/det_image.hpp"
#include "yolo_generator/det_rtsp.hpp"
#include "yolo_generator/det_sahi_image.hpp"
#include "yolo_generator/det_sahi_rtsp.hpp"
#include "yolo_generator/det_sahi_video.hpp"
#include "yolo_generator/det_sahi_vis_rtsp.hpp"
#include "yolo_generator/det_sahi_vis_video.hpp"
#include "yolo_generator/det_video.hpp"
#include "yolo_generator/det_vis_rtsp.hpp"
#include "yolo_generator/det_vis_video.hpp"
#include "yolo_generator/pose_image.hpp"
#include "yolo_generator/pose_rtsp.hpp"
#include "yolo_generator/pose_sahi_image.hpp"
#include "yolo_generator/pose_sahi_rtsp.hpp"
#include "yolo_generator/pose_sahi_video.hpp"
#include "yolo_generator/pose_sahi_vis_rtsp.hpp"
#include "yolo_generator/pose_sahi_vis_video.hpp"
#include "yolo_generator/pose_video.hpp"
#include "yolo_generator/pose_vis_rtsp.hpp"
#include "yolo_generator/pose_vis_video.hpp"
#include "yolo_generator/seg_image.hpp"
#include "yolo_generator/seg_rtsp.hpp"
#include "yolo_generator/seg_sahi_image.hpp"
#include "yolo_generator/seg_sahi_rtsp.hpp"
#include "yolo_generator/seg_sahi_video.hpp"
#include "yolo_generator/seg_sahi_vis_rtsp.hpp"
#include "yolo_generator/seg_sahi_vis_video.hpp"
#include "yolo_generator/seg_video.hpp"
#include "yolo_generator/seg_vis_rtsp.hpp"
#include "yolo_generator/seg_vis_video.hpp"

#include <cstdio>
#include <optional>
#include <stdexcept>
#include <utility>

#include <zip.h>

namespace {

std::string zipDocuments(const std::map<std::string, std::string>& documents) {
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t* buffer = zip_source_buffer_create(nullptr, 0, 1, &error);
    if (buffer == nullptr) {
        const std::string message = zip_error_strerror(&error);
        zip_error_fini(&error);
        throw std::runtime_error(message);
    }
    zip_source_keep(buffer);
    zip_t* archive = zip_open_from_source(buffer, ZIP_CREATE, &error);
    if (archive == nullptr) {
        const std::string message = zip_error_strerror(&error);
        zip_source_free(buffer);
        zip_error_fini(&error);
        throw std::runtime_error(message);
    }
    for (const auto& item : documents) {
        zip_source_t* source = zip_source_buffer(archive, item.second.data(), item.second.size(), 0);
        const zip_int64_t added = source == nullptr ? -1 : zip_file_add(archive, item.first.c_str(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
        if (added < 0) {
            zip_source_free(source);
            zip_discard(archive);
            zip_error_fini(&error);
            throw std::runtime_error("zip add failed: " + item.first);
        }
    }
    if (zip_close(archive) < 0) {
        zip_discard(archive);
        zip_error_fini(&error);
        throw std::runtime_error("zip close failed");
    }
    if (zip_source_open(buffer) < 0 || zip_source_seek(buffer, 0, SEEK_END) < 0) {
        zip_source_free(buffer);
        zip_error_fini(&error);
        throw std::runtime_error("zip read failed");
    }
    const zip_int64_t size = zip_source_tell(buffer);
    zip_source_seek(buffer, 0, SEEK_SET);
    std::string bytes(static_cast<std::size_t>(size < 0 ? 0 : size), '\0');
    const zip_int64_t read = zip_source_read(buffer, bytes.data(), bytes.size());
    zip_source_close(buffer);
    zip_source_free(buffer);
    zip_error_fini(&error);
    if (read != size) {
        throw std::runtime_error("zip read short");
    }
    return bytes;
}

std::optional<nlohmann::json> optionalJson(const nlohmann::json& payload, const char* key) {
    const bool present = payload.contains(key) && !payload.at(key).is_null();
    std::optional<nlohmann::json> value = present ? std::optional<nlohmann::json>(payload.at(key)) : std::nullopt;
    return value;
}

template <typename Generator>
std::string buildVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildSahiVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildSahiVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildSahiRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildEventImage(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_IMAGES),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_EVENT_CODER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER),
        optionalJson(payload, generator_manager::KEY_CAPTURER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildEventVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_EVENT_CODER),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER),
        optionalJson(payload, generator_manager::KEY_CAPTURER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildEventVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_EVENT_CODER),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER),
        optionalJson(payload, generator_manager::KEY_CAPTURER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildEventSahiVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SAHI),
        payload.at(generator_manager::KEY_EVENT_CODER),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER),
        optionalJson(payload, generator_manager::KEY_CAPTURER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildEventRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_EVENT_CODER),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER),
        optionalJson(payload, generator_manager::KEY_CAPTURER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildEventSahiRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SAHI),
        payload.at(generator_manager::KEY_EVENT_CODER),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER),
        optionalJson(payload, generator_manager::KEY_CAPTURER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildImage(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_IMAGES),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildSahiImage(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_IMAGES),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownSahiVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownSahiVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownSahiRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownImage(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_IMAGES),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildTopdownSahiImage(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_IMAGES),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildStgcnppVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildStgcnppVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildStgcnppSahiVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildStgcnppSahiVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildStgcnppRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildStgcnppSahiRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildRtmposeVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildRtmposeVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildRtmposeSahiVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildRtmposeSahiVisVideo(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_VIDEOS),
        payload.at(generator_manager::KEY_OUTPUT).get<std::string>(),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildRtmposeRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

template <typename Generator>
std::string buildRtmposeSahiRtsp(const nlohmann::json& payload, const std::filesystem::path& configSaveDir) {
    Generator generator(
        payload.at(generator_manager::KEY_PIPELINE_NAME).get<std::string>(),
        payload.at(generator_manager::KEY_STREAMS),
        optionalJson(payload, generator_manager::KEY_ANALYZER),
        payload.at(generator_manager::KEY_PGIE),
        payload.at(generator_manager::KEY_SGIE),
        payload.at(generator_manager::KEY_STGCNPP),
        payload.at(generator_manager::KEY_SAHI),
        optionalJson(payload, generator_manager::KEY_TRACKER),
        optionalJson(payload, generator_manager::KEY_LOGGER),
        optionalJson(payload, generator_manager::KEY_DRAWER));
    generator.init();
    std::string archive = zipDocuments(generator.documents(configSaveDir));
    return archive;
}

}

const std::map<std::string, GeneratorManager::Factory>& GeneratorManager::factories() {
    static const std::map<std::string, Factory> registered = {
        {"DetImagePresenceGenerator", buildEventImage<DetImagePresenceGenerator>},
        {"DetVideoPresenceGenerator", buildEventVideo<DetVideoPresenceGenerator>},
        {"DetVisVideoPresenceGenerator", buildEventVisVideo<DetVisVideoPresenceGenerator>},
        {"DetSahiVisVideoPresenceGenerator", buildEventSahiVisVideo<DetSahiVisVideoPresenceGenerator>},
        {"DetRTSPPresenceGenerator", buildEventRtsp<DetRtspPresenceGenerator>},
        {"DetVisRTSPPresenceGenerator", buildEventRtsp<DetVisRtspPresenceGenerator>},
        {"DetSahiRTSPPresenceGenerator", buildEventSahiRtsp<DetSahiRtspPresenceGenerator>},
        {"DetSahiVisRTSPPresenceGenerator", buildEventSahiRtsp<DetSahiVisRtspPresenceGenerator>},
        {"DetImageGenerator", buildImage<DetImageGenerator>},
        {"DetSahiImageGenerator", buildSahiImage<DetSahiImageGenerator>},
        {"SegImageGenerator", buildImage<SegImageGenerator>},
        {"SegSahiImageGenerator", buildSahiImage<SegSahiImageGenerator>},
        {"PoseImageGenerator", buildImage<PoseImageGenerator>},
        {"PoseSahiImageGenerator", buildSahiImage<PoseSahiImageGenerator>},
        {"DetVideoGenerator", buildVideo<DetVideoGenerator>},
        {"DetVisVideoGenerator", buildVisVideo<DetVisVideoGenerator>},
        {"DetSahiVideoGenerator", buildSahiVideo<DetSahiVideoGenerator>},
        {"DetSahiVisVideoGenerator", buildSahiVisVideo<DetSahiVisVideoGenerator>},
        {"SegVideoGenerator", buildVideo<SegVideoGenerator>},
        {"SegVisVideoGenerator", buildVisVideo<SegVisVideoGenerator>},
        {"SegSahiVideoGenerator", buildSahiVideo<SegSahiVideoGenerator>},
        {"SegSahiVisVideoGenerator", buildSahiVisVideo<SegSahiVisVideoGenerator>},
        {"PoseVideoGenerator", buildVideo<PoseVideoGenerator>},
        {"PoseVisVideoGenerator", buildVisVideo<PoseVisVideoGenerator>},
        {"PoseSahiVideoGenerator", buildSahiVideo<PoseSahiVideoGenerator>},
        {"PoseSahiVisVideoGenerator", buildSahiVisVideo<PoseSahiVisVideoGenerator>},
        {"DetRTSPGenerator", buildRtsp<DetRtspGenerator>},
        {"DetVisRTSPGenerator", buildRtsp<DetVisRtspGenerator>},
        {"DetSahiRTSPGenerator", buildSahiRtsp<DetSahiRtspGenerator>},
        {"DetSahiVisRTSPGenerator", buildSahiRtsp<DetSahiVisRtspGenerator>},
        {"SegRTSPGenerator", buildRtsp<SegRtspGenerator>},
        {"SegVisRTSPGenerator", buildRtsp<SegVisRtspGenerator>},
        {"SegSahiRTSPGenerator", buildSahiRtsp<SegSahiRtspGenerator>},
        {"SegSahiVisRTSPGenerator", buildSahiRtsp<SegSahiVisRtspGenerator>},
        {"PoseRTSPGenerator", buildRtsp<PoseRtspGenerator>},
        {"PoseVisRTSPGenerator", buildRtsp<PoseVisRtspGenerator>},
        {"PoseSahiRTSPGenerator", buildSahiRtsp<PoseSahiRtspGenerator>},
        {"PoseSahiVisRTSPGenerator", buildSahiRtsp<PoseSahiVisRtspGenerator>},
        {"TopdownPoseImageGenerator", buildTopdownImage<TopdownPoseImageGenerator>},
        {"TopdownPoseSahiImageGenerator", buildTopdownSahiImage<TopdownPoseSahiImageGenerator>},
        {"TopdownPoseVideoGenerator", buildTopdownVideo<TopdownPoseVideoGenerator>},
        {"TopdownPoseVisVideoGenerator", buildTopdownVisVideo<TopdownPoseVisVideoGenerator>},
        {"TopdownPoseSahiVideoGenerator", buildTopdownSahiVideo<TopdownPoseSahiVideoGenerator>},
        {"TopdownPoseSahiVisVideoGenerator", buildTopdownSahiVisVideo<TopdownPoseSahiVisVideoGenerator>},
        {"TopdownPoseRTSPGenerator", buildTopdownRtsp<TopdownPoseRtspGenerator>},
        {"TopdownPoseVisRTSPGenerator", buildTopdownRtsp<TopdownPoseVisRtspGenerator>},
        {"TopdownPoseSahiRTSPGenerator", buildTopdownSahiRtsp<TopdownPoseSahiRtspGenerator>},
        {"TopdownPoseSahiVisRTSPGenerator", buildTopdownSahiRtsp<TopdownPoseSahiVisRtspGenerator>},
        {"StgcnppYoloPoseVideoGenerator", buildStgcnppVideo<StgcnppYoloPoseVideoGenerator>},
        {"StgcnppYoloPoseVisVideoGenerator", buildStgcnppVisVideo<StgcnppYoloPoseVisVideoGenerator>},
        {"StgcnppYoloPoseSahiVideoGenerator", buildStgcnppSahiVideo<StgcnppYoloPoseSahiVideoGenerator>},
        {"StgcnppYoloPoseSahiVisVideoGenerator", buildStgcnppSahiVisVideo<StgcnppYoloPoseSahiVisVideoGenerator>},
        {"StgcnppYoloPoseRTSPGenerator", buildStgcnppRtsp<StgcnppYoloPoseRtspGenerator>},
        {"StgcnppYoloPoseVisRTSPGenerator", buildStgcnppRtsp<StgcnppYoloPoseVisRtspGenerator>},
        {"StgcnppYoloPoseSahiRTSPGenerator", buildStgcnppSahiRtsp<StgcnppYoloPoseSahiRtspGenerator>},
        {"StgcnppYoloPoseSahiVisRTSPGenerator", buildStgcnppSahiRtsp<StgcnppYoloPoseSahiVisRtspGenerator>},
        {"StgcnppRtmposeVideoGenerator", buildRtmposeVideo<StgcnppRtmposeVideoGenerator>},
        {"StgcnppRtmposeVisVideoGenerator", buildRtmposeVisVideo<StgcnppRtmposeVisVideoGenerator>},
        {"StgcnppRtmposeSahiVideoGenerator", buildRtmposeSahiVideo<StgcnppRtmposeSahiVideoGenerator>},
        {"StgcnppRtmposeSahiVisVideoGenerator", buildRtmposeSahiVisVideo<StgcnppRtmposeSahiVisVideoGenerator>},
        {"StgcnppRtmposeRTSPGenerator", buildRtmposeRtsp<StgcnppRtmposeRtspGenerator>},
        {"StgcnppRtmposeVisRTSPGenerator", buildRtmposeRtsp<StgcnppRtmposeVisRtspGenerator>},
        {"StgcnppRtmposeSahiRTSPGenerator", buildRtmposeSahiRtsp<StgcnppRtmposeSahiRtspGenerator>},
        {"StgcnppRtmposeSahiVisRTSPGenerator", buildRtmposeSahiRtsp<StgcnppRtmposeSahiVisRtspGenerator>},
    };
    return registered;
}

const std::map<std::string, YAML::Node>& GeneratorManager::schemas() {
    static const std::map<std::string, YAML::Node> indexed = [] {
        const std::filesystem::path schemaDir(generator_manager::SCHEMA_DIR);
        if (!std::filesystem::is_directory(schemaDir)) {
            throw std::runtime_error("schema directory missing: " + schemaDir.string());
        }
        std::map<std::string, YAML::Node> loaded;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(schemaDir)) {
            const bool yamlFile = entry.is_regular_file() && (entry.path().extension() == ".yaml" || entry.path().extension() == ".yml");
            if (yamlFile) {
                const YAML::Node data = YAML::LoadFile(entry.path().string());
                if (!data.IsMap() || !data[generator_manager::KEY_TYPE] || !data[generator_manager::KEY_TYPE].IsScalar()) {
                    throw std::runtime_error("schema YAML missing type: " + entry.path().string());
                }
                const std::string generatorName = data[generator_manager::KEY_TYPE].as<std::string>();
                if (loaded.contains(generatorName)) {
                    throw std::runtime_error("duplicate schema generator: " + generatorName);
                }
                loaded.emplace(generatorName, data);
            }
        }
        std::vector<std::string> missing;
        for (const auto& item : factories()) {
            if (!loaded.contains(item.first)) {
                missing.push_back(item.first);
            }
        }
        std::vector<std::string> extra;
        for (const auto& item : loaded) {
            if (!factories().contains(item.first)) {
                extra.push_back(item.first);
            }
        }
        std::string mismatch;
        for (const std::string& name : missing) {
            mismatch += " missing=" + name;
        }
        for (const std::string& name : extra) {
            mismatch += " extra=" + name;
        }
        if (!mismatch.empty()) {
            throw std::runtime_error("schema coverage mismatch" + mismatch);
        }
        return loaded;
    }();
    return indexed;
}

std::vector<std::string> GeneratorManager::types() {
    std::vector<std::string> names;
    for (const auto& item : factories()) {
        names.push_back(item.first);
    }
    return names;
}

YAML::Node GeneratorManager::schema(const std::string& type) {
    const auto found = schemas().find(type);
    if (found == schemas().end()) {
        throw std::runtime_error("unknown type " + type);
    }
    YAML::Node data = found->second;
    return data;
}

std::string GeneratorManager::generate(const std::string& type, const std::filesystem::path& configSaveDir, const nlohmann::json& payload) {
    const auto found = factories().find(type);
    if (found == factories().end()) {
        throw std::runtime_error("unknown type " + type);
    }
    std::string archive = found->second(payload, configSaveDir);
    return archive;
}
