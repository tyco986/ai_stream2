#include "logger/request_logger.hpp"

#include "constants/exporter_const.hpp"
#include "constants/logger_const.hpp"
#include "constants/response_const.hpp"
#include "response/response.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include <spdlog/sinks/daily_file_sink.h>

std::shared_ptr<spdlog::logger> RequestLogger::logger;

nlohmann::json RequestLogger::requestInput(const httplib::Request& request) {
    const httplib::FormData inputFile = request.form.get_file(exporter::FIELD_INPUT);
    const httplib::FormData configFile = request.form.get_file(exporter::FIELD_CONFIG);
    nlohmann::json input = nlohmann::json::object();
    if (!inputFile.filename.empty()) {
        input[exporter::FIELD_INPUT] = "multipart(" + inputFile.filename + ")";
    }
    if (!configFile.filename.empty()) {
        input[exporter::FIELD_CONFIG] = "multipart(" + configFile.filename + ")";
    }
    return input.empty() ? nlohmann::json("") : input;
}

nlohmann::json RequestLogger::responseOutput(const httplib::Response& response) {
    const std::string marker = "name=\"" + std::string(::response::KEY_FILE) + "\"; filename=\"";
    const std::size_t at = response.body.find(marker);
    const std::size_t start = at == std::string::npos ? std::string::npos : at + marker.size();
    const std::size_t end = start == std::string::npos ? std::string::npos : response.body.find('"', start);
    const bool named = end != std::string::npos;
    const bool bodyDict = !named && !response.body.empty() && response.body.front() == '{';
    const bool bodyList = !named && !response.body.empty() && response.body.front() == '[';
    nlohmann::json output = named ? nlohmann::json("multipart(" + response.body.substr(start, end - start) + ")") : "";
    output = bodyDict || bodyList ? nlohmann::json::parse(response.body) : output;
    return output;
}

httplib::Server::HandlerResponse RequestLogger::before(const httplib::Request&, httplib::Response& response) {
    response.user_data.set(request_log::STARTED, std::chrono::steady_clock::now());
    return httplib::Server::HandlerResponse::Unhandled;
}

void RequestLogger::onException(const httplib::Request&, httplib::Response& response, std::exception_ptr pointer) {
    std::string message = "internal error";
    try {
        std::rethrow_exception(pointer);
    } catch (const std::exception& error) {
        message = error.what();
    } catch (...) {
    }
    response.user_data.set(request_log::ERROR, message);
    Response::error(response, 500, message.c_str());
}

void RequestLogger::release() {
    logger.reset();
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
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    const int millis = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
    std::tm local{};
    localtime_r(&time, &local);
    std::ostringstream timestamp;
    timestamp << std::put_time(&local, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << millis;
    const std::string* error = response.user_data.get<std::string>(request_log::ERROR);
    nlohmann::json entry = {
        {request_log::KEY_TS, timestamp.str()},
        {request_log::KEY_METHOD, request.method},
        {request_log::KEY_URL, request.path + query},
        {request_log::KEY_CODE, response.status},
        {request_log::KEY_INPUT, RequestLogger::requestInput(request)},
        {request_log::KEY_OUTPUT, RequestLogger::responseOutput(response)},
        {request_log::KEY_LATENCY, 0},
        {request_log::KEY_IP, request.remote_addr},
        {request_log::KEY_IN_BYTES, inBytes},
        {request_log::KEY_OUT_BYTES, response.body.size()},
        {request_log::KEY_ERROR, error ? *error : ""},
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
