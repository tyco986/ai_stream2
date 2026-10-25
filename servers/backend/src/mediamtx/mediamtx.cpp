#include "mediamtx/mediamtx.hpp"

#include "constants/mediamtx_const.hpp"
#include "constants/proxy_const.hpp"

#include <drogon/HttpClient.h>
#include <json/json.h>

#include <cctype>
#include <exception>

std::string MediaMtx::selfPublishSource(const std::string& name, const std::string& source) {
  const std::size_t scheme = source.find("://");
  const std::size_t authority = scheme == std::string::npos ? std::string::npos : scheme + 3;
  // Host is the authority up to ':' or '/'.
  std::string host;
  if (authority != std::string::npos) {
    const std::size_t end = source.find_first_of(":/", authority);
    host = source.substr(authority, end == std::string::npos ? std::string::npos : end - authority);
  }
  for (char& item : host) {
    item = static_cast<char>(std::tolower(static_cast<unsigned char>(item)));
  }
  // Path is the URL path without leading or trailing slashes.
  std::string path;
  if (authority != std::string::npos) {
    const std::size_t slash = source.find('/', authority);
    const std::size_t begin = slash == std::string::npos ? std::string::npos : source.find_first_not_of('/', slash);
    const std::size_t end = source.find_last_not_of('/');
    path = begin == std::string::npos ? std::string() : source.substr(begin, end - begin + 1);
  }
  // Same path on this MediaMTX host means the url points at the path being configured.
  bool knownHost = false;
  for (const char* item : mediamtx_backend::PUBLISHER_HOSTS) {
    knownHost = knownHost || host == item;
  }
  const bool selfPublish = path == name && knownHost;
  const std::string resolved = selfPublish ? mediamtx_backend::SOURCE_PUBLISHER : source;
  return resolved;
}

drogon::Task<bool> MediaMtx::replacePath(const std::string& name, const std::string& source, bool record) {
  Json::Value body;
  body[mediamtx_backend::KEY_SOURCE] = selfPublishSource(name, source);
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
