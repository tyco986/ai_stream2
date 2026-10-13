#include "response/response.hpp"

#include "constants/response.hpp"

void http::Response::ok(httplib::Response& response, const nlohmann::json& data, const char* message) {
    envelope(response, true, 200, message, data);
}

void http::Response::error(httplib::Response& response, int status, const char* message) {
    envelope(response, false, status, message, nullptr);
}

void http::Response::envelope(httplib::Response& response, bool success, int status, const char* message, const nlohmann::json& data) {
    const nlohmann::json body = {
        {::response::KEY_SUCCESS, success},
        {::response::KEY_MESSAGE, message},
        {::response::KEY_DATA, data},
    };
    response.status = status;
    response.set_content(body.dump(), "application/json");
}
