#pragma once

#include <atomic>
#include <string>

#include <nlohmann/json.hpp>

class History {
public:
    void save(const std::string& fileName, const std::string& fileContent);
    static void setEnabled(bool value);
    nlohmann::json get(const std::string& fileName);
    std::string loadFileContent(const std::string& fileName);
    void remove(const std::string& fileName);

private:
    static std::atomic<bool> enabled;
    nlohmann::json meta(const std::string& fileName);
};
