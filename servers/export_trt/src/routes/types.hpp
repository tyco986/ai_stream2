#pragma once

#include "libs/httplib.h"

class TypesRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void types(const httplib::Request& request, httplib::Response& response);
};
