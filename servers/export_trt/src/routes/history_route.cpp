#include "routes/history_route.hpp"

#include "constants/history_const.hpp"
#include "constants/response_const.hpp"
#include "response/response.hpp"
#include "services/history_service.hpp"

#include <utility>
#include <vector>

std::string HistoryRoute::strip(const std::string& raw) {
    const std::size_t first = raw.find_first_not_of(" \t\r\n");
    const std::size_t last = raw.find_last_not_of(" \t\r\n");
    const std::string name = first == std::string::npos ? "" : raw.substr(first, last - first + 1);
    return name;
}

void HistoryRoute::bind(httplib::Server& server) {
    server.Get(history::PATH, get);
    server.Post(history::PATH, download);
    server.Delete(history::PATH, remove);
}

void HistoryRoute::get(const httplib::Request& request, httplib::Response& response) {
    const std::string fileName = strip(request.get_param_value(history::FIELD_FILENAME));
    Response::ok(response, History().get(fileName));
}

void HistoryRoute::download(const httplib::Request& request, httplib::Response& response) {
    History history;
    const std::string fileName = strip(request.get_param_value(history::FIELD_FILENAME));
    const nlohmann::json chosen = history.get(fileName);
    const bool missing = !fileName.empty() && !chosen.contains(fileName);
    const std::string error = missing ? std::string(history::NOT_FOUND) + fileName : "";
    std::vector<std::pair<std::string, nlohmann::json>> items;
    std::vector<std::pair<std::string, std::string>> contents;
    for (const auto& item : chosen.items()) {
        const std::string fileContent = error.empty() ? history.loadFileContent(item.key()) : "";
        items.emplace_back(item.key(), item.value());
        contents.emplace_back(item.key(), fileContent);
    }
    if (error.empty()) {
        response.status = 200;
        response.set_content(zip2Multipart(items, contents), ::response::MULTIPART_CONTENT_TYPE);
    }
    if (!error.empty()) {
        Response::error(response, 400, error.c_str());
    }
}

void HistoryRoute::remove(const httplib::Request& request, httplib::Response& response) {
    History history;
    const std::string fileName = strip(request.get_param_value(history::FIELD_FILENAME));
    const nlohmann::json chosen = history.get(fileName);
    const bool missing = !fileName.empty() && !chosen.contains(fileName);
    const std::string error = missing ? std::string(history::NOT_FOUND) + fileName : "";
    for (const auto& item : chosen.items()) {
        if (error.empty()) {
            history.remove(item.key());
        }
    }
    if (error.empty()) {
        Response::ok(response);
    }
    if (!error.empty()) {
        Response::error(response, 400, error.c_str());
    }
}

std::string HistoryRoute::zip2Multipart(const std::vector<std::pair<std::string, nlohmann::json>>& items, const std::vector<std::pair<std::string, std::string>>& contents) {
    std::string data;
    for (std::size_t index = 0; index < items.size() && index < contents.size(); ++index) {
        const std::string& fileName = items[index].first;
        const std::string& fileContent = contents[index].second;
        const std::string boundary = std::string(::response::BOUNDARY) + "-" + fileName;
        const std::string part = "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_SUCCESS) + "\"\r\n\r\n"
            "true\r\n"
            "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_DATA) + "\"\r\n"
            "Content-Type: application/json\r\n\r\n" +
            items[index].second.dump() + "\r\n"
            "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_MESSAGE) + "\"\r\n\r\n"
            "\r\n"
            "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_FILE) + "\"; filename=\"" + fileName + "\"\r\n"
            "Content-Type: application/zip\r\n\r\n" +
            fileContent + "\r\n"
            "--" + boundary + "--\r\n";
        data += "--" + std::string(history::BOUNDARY) + "\r\n"
            "Content-Disposition: form-data; name=\"" + fileName + "\"\r\n"
            "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n\r\n" +
            part + "\r\n";
    }
    data += "--" + std::string(history::BOUNDARY) + "--\r\n";
    const std::string body = "--" + std::string(::response::BOUNDARY) + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_SUCCESS) + "\"\r\n\r\n"
        "true\r\n"
        "--" + std::string(::response::BOUNDARY) + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_DATA) + "\"\r\n"
        "Content-Type: multipart/form-data; boundary=" + std::string(history::BOUNDARY) + "\r\n\r\n" +
        data + "\r\n"
        "--" + std::string(::response::BOUNDARY) + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(::response::KEY_MESSAGE) + "\"\r\n\r\n"
        "\r\n"
        "--" + std::string(::response::BOUNDARY) + "--\r\n";
    return body;
}
