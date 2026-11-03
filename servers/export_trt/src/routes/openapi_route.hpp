#pragma once

#include "libs/httplib.h"

class OpenApiRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void spec(const httplib::Request& request, httplib::Response& response);
    static void docs(const httplib::Request& request, httplib::Response& response);
};
