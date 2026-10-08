#pragma once

#include "libs/httplib.h"

#include <nlohmann/json.hpp>

namespace http {

class Response {
public:
    static void ok(httplib::Response& response, const nlohmann::json& data = nlohmann::json::object(), const char* message = "");
    static void error(httplib::Response& response, int status, const char* message);

private:
    static void envelope(httplib::Response& response, bool success, int status, const char* message, const nlohmann::json& data);
};

}
