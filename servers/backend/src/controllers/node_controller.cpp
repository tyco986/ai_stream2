#include "controllers/node_controller.hpp"

#include "response/response.hpp"

#include <drogon/HttpClient.h>

#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>

namespace {

Json::Value beszelResponseToJson(const std::string& text) {
  Json::Value body;
  Json::CharReaderBuilder builder;
  std::string errors;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  const bool parsed = reader->parse(text.data(), text.data() + text.size(), &body, &errors);
  if (!parsed) {
    throw std::runtime_error(errors);
  }
  return body;
}

}

void NodeController::getNodesInfo(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const std::string authToken = getBeszelToken();
    const Json::Value systems = getBeszelRecords(device::COLLECTION_SYSTEMS, "", authToken);
    const Json::Value details = getBeszelRecords(device::COLLECTION_DETAILS, "", authToken);
    const Json::Value stats = getBeszelRecords(device::COLLECTION_STATS, device::STATS_QUERY, authToken);
    const auto response = http::Response::ok(service.formatNodesInfo(systems, details, stats), device::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, device::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

std::string NodeController::getBeszelToken() {
  Json::Value payload(Json::objectValue);
  payload["identity"] = device::HUB_IDENTITY;
  payload["password"] = device::HUB_PASSWORD;
  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  const Json::Value body = requestBeszel(drogon::Post, device::AUTH_PATH, Json::writeString(writer, payload), "");
  const bool present = body.isMember("token") && body["token"].isString();
  if (!present) {
    throw std::runtime_error(device::MSG_LIST_ERROR);
  }
  return body["token"].asString();
}

Json::Value NodeController::requestBeszel(drogon::HttpMethod method,
                                 const std::string& path,
                                 const std::string& body,
                                 const std::string& authToken) {
  auto client = drogon::HttpClient::newHttpClient(device::HUB_BASE_URL);
  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(method);
  request->setPath(path);
  if (!authToken.empty()) {
    request->addHeader("authorization", std::string(device::AUTH_PREFIX) + authToken);
  }
  if (!body.empty()) {
    request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    request->setBody(body);
  }
  const auto result = client->sendRequest(request, device::REQUEST_TIMEOUT_SECONDS);
  const bool ok = result.first == drogon::ReqResult::Ok && result.second &&
                  result.second->statusCode() == drogon::k200OK;
  if (!ok) {
    throw std::runtime_error(device::MSG_LIST_ERROR);
  }
  return beszelResponseToJson(std::string(result.second->getBody()));
}

Json::Value NodeController::getBeszelRecords(const std::string& collection,
                                     const std::string& query,
                                     const std::string& authToken) {
  Json::Value items(Json::arrayValue);
  int page = 1;
  int total = 1;
  while ((page - 1) * device::PAGE_SIZE < total) {
    const std::string path = std::string(device::RECORDS_PREFIX) + collection + device::RECORDS_SUFFIX +
                             "?page=" + std::to_string(page) + "&perPage=" + std::to_string(device::PAGE_SIZE) + query;
    const Json::Value body = requestBeszel(drogon::Get, path, "", authToken);
    total = body.isMember("totalItems") && body["totalItems"].isNumeric() ? body["totalItems"].asInt() : 0;
    const Json::Value pageItems = body.isMember("items") && body["items"].isArray() ? body["items"] : Json::Value(Json::arrayValue);
    for (const auto& item : pageItems) {
      items.append(item);
    }
    page += 1;
  }
  return items;
}
