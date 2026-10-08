#pragma once

#include "libs/httplib.h"

class RtspRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void probe(const httplib::Request& request, httplib::Response& response);
};
