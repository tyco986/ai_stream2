#pragma once

#include "libs/httplib.h"

class LogsRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void enable(const httplib::Request& request, httplib::Response& response);
    static void disable(const httplib::Request& request, httplib::Response& response);
    static void remove(const httplib::Request& request, httplib::Response& response);
};
