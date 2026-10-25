#pragma once

#include <ctime>
#include <map>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

class FFmpegRTSP {
public:
    nlohmann::json probe(const std::string& url) const;
    nlohmann::json publishStart(const std::string& path);
    nlohmann::json publishEnd(const std::string& name, const std::string& status, const std::string& probedAt);
    nlohmann::json publishList(const std::string& name);
    nlohmann::json publishRemove(const std::string& name);

private:
    void stop(int pid);
    std::string formatTime(std::time_t seconds) const;
    nlohmann::json formatPublishResult(const nlohmann::json& record) const;

    static std::mutex publishersMutex;
    static std::map<std::string, nlohmann::json> publishers;
};
