#pragma once

#include "libs/httplib.h"

#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

class HistoryRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void get(const httplib::Request& request, httplib::Response& response);
    static void download(const httplib::Request& request, httplib::Response& response);
    static void remove(const httplib::Request& request, httplib::Response& response);
    static std::string strip(const std::string& raw);
    static std::string zip2Multipart(const std::vector<std::pair<std::string, nlohmann::json>>& items, const std::vector<std::pair<std::string, std::string>>& contents);
};
