#pragma once

#include "libs/httplib.h"

#include <string>

#include <nlohmann/json.hpp>

class RtspRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void probe(const httplib::Request& request, httplib::Response& response);
    static void publish(const httplib::Request& request, httplib::Response& response);
    static nlohmann::json publishOne(const std::string& name, const std::string& path);
    static bool pathReady(const std::string& name);
    static bool mediamtxReady();
    static void publishList(const httplib::Request& request, httplib::Response& response);
    static void publishRemove(const httplib::Request& request, httplib::Response& response);
};
