#pragma once

#include "libs/httplib.h"

#include <memory>

#include <spdlog/spdlog.h>

class RequestLogger {
public:
    static httplib::Server::HandlerResponse before(const httplib::Request& request, httplib::Response& response);
    static void write(const httplib::Request& request, const httplib::Response& response);

private:
    static std::shared_ptr<spdlog::logger> logger;
    static std::shared_ptr<spdlog::logger> instance();
};
