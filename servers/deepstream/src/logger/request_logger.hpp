#pragma once

#include "libs/httplib.h"

#include <exception>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

class RequestLogger {
public:
    static httplib::Server::HandlerResponse before(const httplib::Request& request, httplib::Response& response);
    static void onException(const httplib::Request& request, httplib::Response& response, std::exception_ptr pointer);
    static void write(const httplib::Request& request, const httplib::Response& response);
    static void setPipelineName(const std::string& name);
    static void release();

private:
    static nlohmann::json requestInput(const httplib::Request& request);
    static nlohmann::json responseOutput(const httplib::Response& response);
    static std::string pipelineName;
    static std::shared_ptr<spdlog::logger> logger;
    static std::shared_ptr<spdlog::logger> instance();
};
