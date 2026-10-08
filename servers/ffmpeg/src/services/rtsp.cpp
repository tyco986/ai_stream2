#include "services/rtsp.hpp"

#include "constants/rtsp.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#include <sys/wait.h>

#include <nlohmann/json.hpp>

nlohmann::json FFmpegRTSP::probe(const std::string& url) const {
    nlohmann::json result{{"status", "offline"}, {"width", nullptr}, {"height", nullptr}, {"fps", nullptr}, {"probed_at", ""}};
    const std::string command = std::string(FFPROBE_BINARY)
        + " -v error -rtsp_transport tcp -timeout " + std::to_string(FFPROBE_TIMEOUT * 1000000)
        + " -select_streams v:0 -show_entries stream=width,height,r_frame_rate -of json '" + url + "'";
    std::string output;
    char buffer[4096];
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe) {
        while (std::fgets(buffer, sizeof(buffer), pipe)) output += buffer;
    }
    const int status = pipe ? pclose(pipe) : -1;
    const auto parsed = nlohmann::json::parse(output, nullptr, false);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && parsed.contains("streams") && !parsed["streams"].empty()) {
        const auto& stream = parsed["streams"][0];
        const std::string rate = stream.value("r_frame_rate", "0/1");
        const auto slash = rate.find('/');
        result["status"] = "online";
        result["width"] = stream.value("width", 0);
        result["height"] = stream.value("height", 0);
        result["fps"] = std::atof(rate.c_str()) / (slash == std::string::npos ? 1.0 : std::atof(rate.c_str() + slash + 1));
    }
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
    gmtime_r(&now, &utc);
    char probedAt[32];
    std::strftime(probedAt, sizeof(probedAt), "%Y-%m-%dT%H:%M:%SZ", &utc);
    result["probed_at"] = probedAt;
    return result;
}
