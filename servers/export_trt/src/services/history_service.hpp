#pragma once

#include <string>

#include <nlohmann/json.hpp>

class History {
public:
    void save(const std::string& fileName, const std::string& fileContent);
    nlohmann::json get(const std::string& fileName);
    std::string loadFileContent(const std::string& fileName);
    void remove(const std::string& fileName);

private:
    nlohmann::json meta(const std::string& fileName);
};
