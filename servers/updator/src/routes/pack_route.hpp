#pragma once

#include "libs/httplib.h"

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class PackRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void pack(const httplib::Request& request, httplib::Response& response);
    static void push(const std::string& name, const std::string& tag);
    static nlohmann::json manifest(const std::string& name, const std::string& reference, std::string& digest);
    static std::vector<nlohmann::json> tree(const std::string& name, const std::string& digest);
    static void save(const std::string& name, const std::string& kind, const std::string& digest, const std::filesystem::path& file);
    static std::string request(const std::string& host, int port, const std::string& method, const std::string& path, const httplib::Headers& headers, httplib::Headers& responseHeaders);
    static std::string encode(const std::string& value);
};
