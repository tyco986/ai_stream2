#pragma once

#include "libs/httplib.h"

class HealthRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void health(const httplib::Request& request, httplib::Response& response);
};
