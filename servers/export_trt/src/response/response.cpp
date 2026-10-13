#include "response/response.hpp"

#include "constants/response.hpp"

#include <string>

void http::Response::ok(httplib::Response& response, const nlohmann::json& data, const char* message, const std::string& fileName, const std::string& fileContent) {
    const bool attached = !fileContent.empty();
    const std::string boundary = ::response::BOUNDARY;
    const std::string body = "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_SUCCESS) + "\"\r\n\r\n"
        "true\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_MESSAGE) + "\"\r\n\r\n" +
        message + "\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_DATA) + "\"\r\n"
        "Content-Type: application/json\r\n\r\n" +
        data.dump() + "\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_FILE) + "\"; filename=\"" + fileName + "\"\r\n"
        "Content-Type: application/zip\r\n\r\n" +
        fileContent + "\r\n"
        "--" + boundary + "--\r\n";
    if (attached) {
        response.status = 200;
        response.set_content(body, ::response::MULTIPART_CONTENT_TYPE);
    }
    if (!attached) {
        envelope(response, true, 200, message, data);
    }
}

void http::Response::error(httplib::Response& response, int status, const char* message) {
    envelope(response, false, status, message, nullptr);
}

void http::Response::envelope(httplib::Response& response, bool success, int status, const char* message, const nlohmann::json& data) {
    const nlohmann::json body = {
        {::response::KEY_SUCCESS, success},
        {::response::KEY_MESSAGE, message},
        {::response::KEY_DATA, data},
        {::response::KEY_FILE, nullptr},
    };
    response.status = status;
    response.set_content(body.dump(), "application/json");
}
