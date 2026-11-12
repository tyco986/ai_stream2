#include "routes/pipeline_route.hpp"

#include "constants/pipeline_const.hpp"
#include "response/response.hpp"
#include "services/pipeline_service.hpp"

#include "nlohmann/json.hpp"

#include <chrono>
#include <ctime>
#include <optional>
#include <stdexcept>

nlohmann::json formatTime(const std::optional<std::chrono::system_clock::time_point>& time) {
    std::time_t seconds = 0;
    std::tm local{};
    char buffer[40] = {};
    if (time.has_value()) {
        seconds = std::chrono::system_clock::to_time_t(*time);
        localtime_r(&seconds, &local);
        std::strftime(buffer, sizeof(buffer), pipeline::TIME_FORMAT, &local);
    }
    return time.has_value() ? nlohmann::json(buffer) : nlohmann::json(nullptr);
}

void PipelineRoute::bind(httplib::Server& server) {
    server.Post(pipeline::START_PATH, start);
    server.Post(pipeline::START_LOCAL_PATH, startLocal);
    server.Post(pipeline::STOP_PATH, stop);
    server.Get(pipeline::STATUS_PATH, status);
}

void PipelineRoute::start(const httplib::Request& request, httplib::Response& response) {
    const bool running = PipelineService::status().status == pipeline::STATUS_RUNNING;
    if (running) {
        Response::error(response, pipeline::STATUS_CONFLICT, pipeline::ERR_RUNNING);
    }
    if (!running) {
        const httplib::FormData config = request.form.get_file(pipeline::FIELD_CONFIG);
        const std::vector<httplib::FormData> models = request.form.get_files(pipeline::FIELD_MODEL);
        const httplib::FormData media = request.form.get_file(pipeline::FIELD_MEDIA);
        const std::filesystem::path configDir = PipelineService::prepare(config, models, media);
        PipelineService::start(configDir);
        Response::ok(response);
    }
}

void PipelineRoute::startLocal(const httplib::Request& request, httplib::Response& response) {
    const bool running = PipelineService::status().status == pipeline::STATUS_RUNNING;
    if (running) {
        Response::error(response, pipeline::STATUS_CONFLICT, pipeline::ERR_RUNNING);
    }
    const nlohmann::json body = nlohmann::json::parse(request.body, nullptr, false);
    const bool valid = !running && body.is_object() && body.contains(pipeline::FIELD_CONFIG) && body[pipeline::FIELD_CONFIG].is_string();
    if (!running && !valid) {
        throw std::runtime_error("invalid config");
    }
    if (valid) {
        const std::filesystem::path configDir = PipelineService::prepareLocal(body[pipeline::FIELD_CONFIG].get<std::string>());
        PipelineService::start(configDir);
        Response::ok(response);
    }
}

void PipelineRoute::status(const httplib::Request&, httplib::Response& response) {
    const PipelineStatus report = PipelineService::status();
    const nlohmann::json data = {
        {pipeline::KEY_STATUS, report.status},
        {pipeline::KEY_START_TIME, formatTime(report.startTime)},
        {pipeline::KEY_END_TIME, formatTime(report.endTime)},
        {pipeline::KEY_DURATION, report.duration.has_value() ? nlohmann::json(*report.duration) : nlohmann::json(nullptr)},
    };
    Response::ok(response, data);
}

void PipelineRoute::stop(const httplib::Request&, httplib::Response& response) {
    const bool running = PipelineService::status().status == pipeline::STATUS_RUNNING;
    if (!running) {
        Response::error(response, pipeline::STATUS_CONFLICT, pipeline::ERR_IDLE);
    }
    if (running) {
        PipelineService::stop();
        Response::ok(response);
    }
}
