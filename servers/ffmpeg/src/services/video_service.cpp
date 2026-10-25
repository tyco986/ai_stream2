#include "services/video_service.hpp"

#include "constants/video_const.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#include <sys/wait.h>

#include <nlohmann/json.hpp>

nlohmann::json FFmpegVideo::probe(const std::string& path) const {
    nlohmann::json result{
        {video::KEY_WIDTH, nullptr},
        {video::KEY_HEIGHT, nullptr},
        {video::KEY_FPS, nullptr},
        {video::KEY_DURATION, nullptr},
        {video::KEY_CODEC, nullptr},
        {video::KEY_BITRATE, nullptr},
    };
    const bool safe = path.find('\'') == std::string::npos;
    const std::string command = std::string(video::FFPROBE_BINARY)
        + " -v error -select_streams v:0 -show_entries stream=width,height,avg_frame_rate,duration,codec_name,bit_rate -of json '"
        + path + "'";
    std::string output;
    char buffer[4096];
    FILE* pipe = safe ? popen(command.c_str(), "r") : nullptr;
    if (pipe) {
        while (std::fgets(buffer, sizeof(buffer), pipe)) output += buffer;
    }
    const int status = pipe ? pclose(pipe) : -1;
    const auto parsed = nlohmann::json::parse(output, nullptr, false);
    const bool accepted = WIFEXITED(status) && WEXITSTATUS(status) == 0 && parsed.contains("streams") && !parsed["streams"].empty();
    if (accepted) {
        const auto& stream = parsed["streams"][0];
        const std::string rate = stream.value("avg_frame_rate", "0/1");
        const auto slash = rate.find('/');
        const double numerator = std::atof(rate.c_str());
        const double denominator = slash == std::string::npos ? 1.0 : std::atof(rate.c_str() + slash + 1);
        result[video::KEY_WIDTH] = stream.value("width", 0);
        result[video::KEY_HEIGHT] = stream.value("height", 0);
        result[video::KEY_FPS] = denominator == 0 ? 0.0 : numerator / denominator;
        result[video::KEY_DURATION] = stream.contains("duration") ? std::atof(stream.value("duration", "0").c_str()) : 0.0;
        result[video::KEY_CODEC] = stream.value("codec_name", "");
        result[video::KEY_BITRATE] = stream.contains("bit_rate") ? std::atoll(stream.value("bit_rate", "0").c_str()) : 0;
    }
    return result;
}
