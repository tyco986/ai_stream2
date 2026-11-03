#pragma once

#include "libs/httplib.h"

class GenerateRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void generate(const httplib::Request& request, httplib::Response& response);
};
