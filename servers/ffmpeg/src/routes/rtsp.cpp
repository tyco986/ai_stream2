#include "routes/rtsp.hpp"

#include "constants/rtsp.hpp"
#include "response/response.hpp"
#include "services/rtsp.hpp"

#include <string>

#include <nlohmann/json.hpp>

void RtspRoute::bind(httplib::Server& server) {
    server.Post(RTSP_PROBE_PATH, probe);
}

void RtspRoute::probe(const httplib::Request& request, httplib::Response& response) {
    const auto body = nlohmann::json::parse(request.body, nullptr, false);
    const std::string url = body.is_object() ? body.value("rtsp", "") : "";
    http::Response::ok(response, FFmpegRTSP().probe(url));
}
