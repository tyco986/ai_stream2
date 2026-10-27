#include "logger/request_logger.hpp"

#include "constants/logger_const.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include <spdlog/sinks/daily_file_sink.h>

std::shared_ptr<spdlog::logger> RequestLogger::logger;

httplib::Server::HandlerResponse RequestLogger::before(const httplib::Request&, httplib::Response& response) {
    response.user_data.set(request_log::STARTED, std::chrono::steady_clock::now());
    return httplib::Server::HandlerResponse::Unhandled;
}

std::shared_ptr<spdlog::logger> RequestLogger::instance() {
    const bool missing = !logger;
    if (missing) {
        std::filesystem::create_directories(request_log::ROOT);
        const std::string path = std::filesystem::path(request_log::ROOT) / (std::string(request_log::PAGE) + request_log::SUFFIX);
        auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(path, 0, 0, false, request_log::MAX_DAYS);
        logger = std::make_shared<spdlog::logger>(request_log::PAGE, sink);
        logger->set_pattern("%v");
        logger->flush_on(spdlog::level::info);
    }
    return logger;
}

void RequestLogger::write(const httplib::Request& request, const httplib::Response& response) {
    const std::chrono::steady_clock::time_point* started = response.user_data.get<std::chrono::steady_clock::time_point>(request_log::STARTED);
    const double latencyMs = started ? std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - *started).count() : 0;
    const bool multipart = response.get_header_value("Content-Type").starts_with("multipart/");
    const bool jsonBody = !response.body.empty() && (response.body.front() == '{' || response.body.front() == '[');
    nlohmann::json output = "";
    output = multipart ? nlohmann::json(request_log::MULTIPART_OUTPUT) : output;
    output = !multipart && jsonBody ? nlohmann::json::parse(response.body) : output;
    nlohmann::json header = nlohmann::json::object();
    for (const char* name : request_log::HEADERS) {
        const std::string value = request.get_header_value(name);
        if (!value.empty()) {
            header[name] = value;
        }
    }
    const std::string query = request.target.find('?') == std::string::npos ? "" : request.target.substr(request.target.find('?'));
    const std::string contentLength = request.get_header_value("Content-Length");
    const std::size_t inBytes = contentLength.empty() ? request.body.size() : static_cast<std::size_t>(std::stoull(contentLength));
    std::ostringstream latency;
    latency << std::fixed << std::setprecision(2) << latencyMs;
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    const int millis = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
    nlohmann::json entry = {
        {request_log::KEY_TS, std::format("{:%Y-%m-%d %H:%M:%S}.{:03d}", std::chrono::zoned_time{std::chrono::current_zone(), now}, millis)},
        {request_log::KEY_METHOD, request.method},
        {request_log::KEY_URL, request.path + query},
        {request_log::KEY_CODE, response.status},
        {request_log::KEY_INPUT, ""},
        {request_log::KEY_OUTPUT, output},
        {request_log::KEY_LATENCY, 0},
        {request_log::KEY_IP, request.remote_addr},
        {request_log::KEY_IN_BYTES, inBytes},
        {request_log::KEY_OUT_BYTES, response.body.size()},
        {request_log::KEY_ERROR, ""},
        {request_log::KEY_HEADER, header},
    };
    std::string text = entry.dump();
    const std::string placeholder = request_log::LATENCY_PLACEHOLDER;
    const std::size_t at = text.find(placeholder);
    if (at != std::string::npos) {
        text.replace(at, placeholder.size(), "\"latency\":" + latency.str());
    }
    instance()->info(text);
}
