#include "mediamtx/mediamtx.hpp"

#include "constants/mediamtx_const.hpp"
#include "constants/proxy_const.hpp"

#include <drogon/HttpClient.h>
#include <json/json.h>

#include <exception>

drogon::Task<bool> MediaMtx::replacePath(const std::string& name, const std::string& source, bool record) {
  Json::Value body;
  body[mediamtx_backend::KEY_SOURCE] = source;
  body[mediamtx_backend::KEY_RECORD] = record;
  auto request = drogon::HttpRequest::newHttpJsonRequest(body);
  request->setMethod(drogon::Post);
  request->setPath(std::string(mediamtx_backend::REPLACE_PATH) + name);
  auto client = drogon::HttpClient::newHttpClient(mediamtx::BASE_URL);
  bool mounted = false;
  try {
    const auto response = co_await client->sendRequestCoro(request, mediamtx_backend::REQUEST_TIMEOUT_SECONDS);
    mounted = response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
  } catch (const std::exception&) {
    mounted = false;
  }
  co_return mounted;
}

drogon::Task<bool> MediaMtx::getPath(const std::string& name) {
  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Get);
  request->setPath(std::string(mediamtx_backend::GET_PATH) + name);
  auto client = drogon::HttpClient::newHttpClient(mediamtx::BASE_URL);
  bool found = false;
  try {
    const auto response = co_await client->sendRequestCoro(request, mediamtx_backend::REQUEST_TIMEOUT_SECONDS);
    found = response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
  } catch (const std::exception&) {
    found = false;
  }
  co_return found;
}

drogon::Task<bool> MediaMtx::deletePath(const std::string& name) {
  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Delete);
  request->setPath(std::string(mediamtx_backend::DELETE_PATH) + name);
  auto client = drogon::HttpClient::newHttpClient(mediamtx::BASE_URL);
  bool removed = false;
  try {
    const auto response = co_await client->sendRequestCoro(request, mediamtx_backend::REQUEST_TIMEOUT_SECONDS);
    removed = response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
  } catch (const std::exception&) {
    removed = false;
  }
  co_return removed;
}
