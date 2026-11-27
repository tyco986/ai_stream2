#pragma once

#include "libs/httplib.h"
#include "services/update_service.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class UpdateRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void update(const httplib::Request& request, httplib::Response& response);
    static void apply(const UpdateImage& image);
    static void save(const std::string& name, const UpdateBlob& blob, const std::string& reference);
    static std::string request(const std::string& host, int port, const std::string& method, const std::string& path, const httplib::Headers& headers, httplib::Headers& responseHeaders, int okFrom, int okTo);
    static std::string encode(const std::string& value);
};
