#include "routes/video_route.hpp"

#include "constants/video_const.hpp"
#include "response/response.hpp"
#include "services/video_service.hpp"

#include <string>

#include <nlohmann/json.hpp>

void VideoRoute::bind(httplib::Server& server) {
    server.Post(video::VIDEO_PROBE_PATH, probe);
}

void VideoRoute::probe(const httplib::Request& request, httplib::Response& response) {
    const auto body = nlohmann::json::parse(request.body, nullptr, false);
    const std::string path = body.is_object() ? body.value(video::KEY_PATH, "") : "";
    http::Response::ok(response, FFmpegVideo().probe(path), video::MSG_PROBE_OK);
}
