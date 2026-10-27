#include "routes/rtsp_route.hpp"

#include "constants/rtsp_const.hpp"
#include "response/response.hpp"
#include "services/rtsp_service.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

void RtspRoute::bind(httplib::Server& server) {
    server.Post(rtsp::RTSP_PROBE_PATH, probe);
    server.Post(rtsp::RTSP_PUBLISH_PATH, publish);
    server.Get(rtsp::RTSP_PUBLISH_PATH, publishList);
    server.Delete(rtsp::RTSP_PUBLISH_PATH, publishRemove);
}

void RtspRoute::probe(const httplib::Request& request, httplib::Response& response) {
    const auto body = nlohmann::json::parse(request.body, nullptr, false);
    const nlohmann::json urls = body.is_object() && body.contains(rtsp::KEY_URLS) && body[rtsp::KEY_URLS].is_array()
        ? body[rtsp::KEY_URLS]
        : nlohmann::json::array();
    nlohmann::json data = nlohmann::json::object();
    for (const auto& item : urls) {
        const std::string url = item.is_string() ? item.get<std::string>() : "";
        const nlohmann::json probed = item.is_string() ? FFmpegRTSP().probe(url) : nlohmann::json::object();
        data[url] = probed;
    }
    http::Response::ok(response, data, rtsp::MSG_PROBE_OK);
}

void RtspRoute::publish(const httplib::Request& request, httplib::Response& response) {
    const bool alive = mediamtxReady();
    const auto body = nlohmann::json::parse(request.body, nullptr, false);
    const nlohmann::json paths = body.is_object() && body.contains(rtsp::KEY_PATHS) && body[rtsp::KEY_PATHS].is_object()
        ? body[rtsp::KEY_PATHS]
        : nlohmann::json::object();
    nlohmann::json data = nlohmann::json::object();
    for (const auto& [name, path] : paths.items()) {
        const nlohmann::json published = alive && path.is_string() ? publishOne(name, path.get<std::string>()) : nlohmann::json::object();
        data.update(published);
    }
    if (!alive) {
        http::Response::error(response, 503, rtsp::MSG_MEDIAMTX_UNREACHABLE);
    }
    if (alive && !data.empty()) {
        http::Response::ok(response, data, rtsp::MSG_PUBLISH_OK);
    }
    if (alive && data.empty()) {
        http::Response::error(response, 400, rtsp::MSG_PUBLISH_ERROR);
    }
}

nlohmann::json RtspRoute::publishOne(const std::string& name, const std::string& path) {
    const nlohmann::json started = FFmpegRTSP().publishStart(name, path);
    const bool startedOk = !started.empty();
    const std::string url = startedOk ? started.begin().value().value(rtsp::KEY_URL_DOCKER, "") : "";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(rtsp::PUBLISH_TIMEOUT_SECONDS);
    bool ready = false;
    while (startedOk && !ready && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::seconds(rtsp::PUBLISH_POLL_SECONDS));
        ready = pathReady(name);
    }
    nlohmann::json data = nlohmann::json::object();
    if (ready) {
        const nlohmann::json probed = FFmpegRTSP().probe(url);
        data = FFmpegRTSP().publishEnd(name, probed.value(rtsp::KEY_STATUS, rtsp::STATUS_OFFLINE), probed.value(rtsp::KEY_PROBED_AT, ""));
    }
    if (startedOk && !ready) {
        FFmpegRTSP().publishRemove({name});
    }
    return data;
}

bool RtspRoute::mediamtxReady() {
    httplib::Client client(rtsp::MEDIAMTX_HOST, rtsp::MEDIAMTX_API_PORT);
    client.set_connection_timeout(rtsp::MEDIAMTX_HEALTH_TIMEOUT_SECONDS, 0);
    client.set_read_timeout(rtsp::MEDIAMTX_HEALTH_TIMEOUT_SECONDS, 0);
    const auto result = client.Get(rtsp::MEDIAMTX_INFO_PATH);
    const bool alive = result && result->status == 200;
    return alive;
}

bool RtspRoute::pathReady(const std::string& name) {
    httplib::Client client(rtsp::MEDIAMTX_HOST, rtsp::MEDIAMTX_API_PORT);
    client.set_connection_timeout(rtsp::PUBLISH_POLL_SECONDS, 0);
    client.set_read_timeout(rtsp::PUBLISH_POLL_SECONDS, 0);
    const auto result = client.Get(std::string(rtsp::MEDIAMTX_PATH_GET) + name);
    const nlohmann::json body = result ? nlohmann::json::parse(result->body, nullptr, false) : nlohmann::json();
    const bool ready = result && result->status == 200 && body.is_object() && body.value(rtsp::KEY_READY, false);
    return ready;
}

void RtspRoute::publishList(const httplib::Request& request, httplib::Response& response) {
    const std::vector<std::string> raw = request.get_param_values(rtsp::KEY_NAMES);
    std::vector<std::string> names;
    for (const std::string& item : raw) {
        if (!item.empty()) {
            names.push_back(item);
        }
    }
    http::Response::ok(response, FFmpegRTSP().publishList(names));
}

void RtspRoute::publishRemove(const httplib::Request& request, httplib::Response& response) {
    const std::vector<std::string> raw = request.get_param_values(rtsp::KEY_NAMES);
    std::vector<std::string> names;
    for (const std::string& item : raw) {
        if (!item.empty()) {
            names.push_back(item);
        }
    }
    http::Response::ok(response, FFmpegRTSP().publishRemove(names), rtsp::MSG_PUBLISH_REMOVE_OK);
}
