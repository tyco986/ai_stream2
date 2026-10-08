#include "mediamtx/mediamtx.hpp"

#include "constants/mediamtx.hpp"
#include "constants/proxy.hpp"

#include <drogon/HttpClient.h>
#include <json/json.h>

void MediaMtx::replacePath(const std::string& name,
                           const std::string& source,
                           bool record,
                           std::function<void(bool)> callback) {
  Json::Value body;
  body[mediamtx_backend::KEY_SOURCE] = source;
  body[mediamtx_backend::KEY_RECORD] = record;
  auto request = drogon::HttpRequest::newHttpJsonRequest(body);
  request->setMethod(drogon::Post);
  request->setPath(std::string(mediamtx_backend::REPLACE_PATH) + name);
  auto client = drogon::HttpClient::newHttpClient(mediamtx::BASE_URL);
  client->sendRequest(
      request,
      [callback = std::move(callback), client](drogon::ReqResult result,
                                               const drogon::HttpResponsePtr& response) {
        const bool mounted = result == drogon::ReqResult::Ok && response &&
            response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
        callback(mounted);
      },
      mediamtx_backend::REQUEST_TIMEOUT_SECONDS);
}

void MediaMtx::deletePath(const std::string& name, std::function<void(bool)> callback) {
  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Delete);
  request->setPath(std::string(mediamtx_backend::DELETE_PATH) + name);
  auto client = drogon::HttpClient::newHttpClient(mediamtx::BASE_URL);
  client->sendRequest(
      request,
      [callback = std::move(callback), client](drogon::ReqResult result,
                                               const drogon::HttpResponsePtr& response) {
        const bool removed = result == drogon::ReqResult::Ok && response &&
            response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
        callback(removed);
      },
      mediamtx_backend::REQUEST_TIMEOUT_SECONDS);
}
