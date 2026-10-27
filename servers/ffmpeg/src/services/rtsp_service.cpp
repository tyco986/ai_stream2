#include "services/rtsp_service.hpp"

#include "constants/rtsp_const.hpp"
#include "services/video_service.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

extern char** environ;

std::mutex FFmpegRTSP::publishersMutex;
std::map<std::string, nlohmann::json> FFmpegRTSP::publishers;

nlohmann::json FFmpegRTSP::probe(const std::string& url) const {
    nlohmann::json result{{"status", "offline"}, {"width", nullptr}, {"height", nullptr}, {"fps", nullptr}, {"probed_at", ""}};
    const std::string command = std::string(rtsp::FFPROBE_BINARY)
        + " -v error -rtsp_transport tcp -timeout " + std::to_string(rtsp::FFPROBE_TIMEOUT * 1000000)
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

nlohmann::json FFmpegRTSP::publishStart(const std::string& name, const std::string& path) {
    const nlohmann::json probed = FFmpegVideo().probe(path);
    const bool ready = !name.empty() && name != "." && name != ".." && !probed[rtsp::KEY_WIDTH].is_null();
    nlohmann::json data = nlohmann::json::object();
    std::lock_guard<std::mutex> guard(publishersMutex);
    const bool occupied = publishers.contains(name);
    std::vector<std::string> arguments = {
        rtsp::FFMPEG_BINARY, "-hide_banner", "-loglevel", "error", "-re", "-stream_loop", "-1", "-i", path,
        "-c:v", "copy", "-an", "-f", "rtsp", "-rtsp_transport", "tcp", std::string(rtsp::URL_DOCKER_PREFIX) + name,
    };
    std::vector<char*> argv;
    for (std::string& argument : arguments) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);
    pid_t child = -1;
    const int spawned = ready && !occupied ? posix_spawnp(&child, argv[0], nullptr, nullptr, argv.data(), environ) : -1;
    if (spawned == 0) {
        const std::time_t now = std::time(nullptr);
        nlohmann::json record = {
            {rtsp::KEY_PATH, path},
            {rtsp::KEY_URL_DOCKER, std::string(rtsp::URL_DOCKER_PREFIX) + name},
            {rtsp::KEY_URL_HOST, std::string(rtsp::URL_HOST_PREFIX) + name},
            {rtsp::KEY_WIDTH, probed[rtsp::KEY_WIDTH]},
            {rtsp::KEY_HEIGHT, probed[rtsp::KEY_HEIGHT]},
            {rtsp::KEY_FPS, probed[rtsp::KEY_FPS]},
            {rtsp::KEY_CODEC, probed[rtsp::KEY_CODEC]},
            {rtsp::KEY_BITRATE, probed[rtsp::KEY_BITRATE]},
            {rtsp::KEY_LOOP, true},
            {rtsp::KEY_CREATED_AT, formatTime(now)},
            {rtsp::KEY_DURATION, 0},
            {rtsp::KEY_STATUS, rtsp::STATUS_OFFLINE},
            {rtsp::KEY_PROBED_AT, ""},
            {rtsp::KEY_PID, static_cast<int>(child)},
        };
        publishers.insert({name, record});
        data[name] = formatPublishResult(record);
    }
    return data;
}

nlohmann::json FFmpegRTSP::publishEnd(const std::string& name, const std::string& status, const std::string& probedAt) {
    std::lock_guard<std::mutex> guard(publishersMutex);
    const auto found = publishers.find(name);
    nlohmann::json data = nlohmann::json::object();
    if (found != publishers.end()) {
        found->second[rtsp::KEY_STATUS] = status;
        found->second[rtsp::KEY_PROBED_AT] = probedAt;
        data[name] = formatPublishResult(found->second);
    }
    return data;
}

nlohmann::json FFmpegRTSP::publishList(const std::vector<std::string>& names) {
    const std::set<std::string> selected(names.begin(), names.end());
    const bool listAll = selected.empty();
    std::lock_guard<std::mutex> guard(publishersMutex);
    nlohmann::json data = nlohmann::json::object();
    for (const auto& [key, record] : publishers) {
        const bool matched = listAll || selected.contains(key);
        if (matched) {
            data[key] = formatPublishResult(record);
        }
    }
    return data;
}

nlohmann::json FFmpegRTSP::publishRemove(const std::vector<std::string>& names) {
    const std::set<std::string> selected(names.begin(), names.end());
    const bool removeAll = selected.empty();
    std::vector<int> pids;
    {
        std::lock_guard<std::mutex> guard(publishersMutex);
        for (auto found = publishers.begin(); found != publishers.end();) {
            const bool matched = removeAll || selected.contains(found->first);
            if (matched) {
                pids.push_back(found->second[rtsp::KEY_PID].get<int>());
                found = publishers.erase(found);
            }
            if (!matched) {
                ++found;
            }
        }
    }
    for (const int pid : pids) {
        stop(pid);
    }
    const nlohmann::json data = publishList({});
    return data;
}

void FFmpegRTSP::stop(int pid) {
    ::kill(pid, SIGTERM);
    int status = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    pid_t waited = ::waitpid(pid, &status, WNOHANG);
    while (waited == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        waited = ::waitpid(pid, &status, WNOHANG);
    }
    if (waited == 0) {
        ::kill(pid, SIGKILL);
        ::waitpid(pid, &status, 0);
    }
}

std::string FFmpegRTSP::formatTime(std::time_t seconds) const {
    std::tm utc{};
    gmtime_r(&seconds, &utc);
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &utc);
    const std::string formatted(text);
    return formatted;
}

nlohmann::json FFmpegRTSP::formatPublishResult(const nlohmann::json& record) const {
    nlohmann::json shown = record;
    shown.erase(rtsp::KEY_PID);
    std::tm utc{};
    strptime(shown[rtsp::KEY_CREATED_AT].get<std::string>().c_str(), "%Y-%m-%dT%H:%M:%SZ", &utc);
    shown[rtsp::KEY_DURATION] = std::difftime(std::time(nullptr), timegm(&utc));
    return shown;
}

