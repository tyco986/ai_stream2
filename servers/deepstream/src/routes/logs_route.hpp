#pragma once

#include "libs/httplib.h"

class LogsRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void remove(const httplib::Request& request, httplib::Response& response);
};
