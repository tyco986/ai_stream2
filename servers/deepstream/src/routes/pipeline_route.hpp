#pragma once

#include "libs/httplib.h"

class PipelineRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void start(const httplib::Request& request, httplib::Response& response);
    static void stop(const httplib::Request& request, httplib::Response& response);
    static void status(const httplib::Request& request, httplib::Response& response);
};
