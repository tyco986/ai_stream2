#include "controllers/pipeline_batch_controller.hpp"

#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <drogon/HttpClient.h>

#include <ctime>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

PipelineBatchController::PipelineBatchController() : service(Database::client()) {}

void PipelineBatchController::respond(
    const drogon::HttpRequestPtr& request,
    const drogon::HttpResponsePtr& response,
    std::chrono::steady_clock::time_point started,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback,
    const std::string& error) {
  const auto elapsed = std::chrono::steady_clock::now() - started;
  const std::string query = request->query();
  LogEntry entry;
  entry.method = request->methodString();
  entry.url = request->path();
  entry.url += query.empty() ? "" : "?" + query;
  entry.code = static_cast<int>(response->statusCode());
  entry.input = std::string(request->getBody());
  entry.output = std::string(response->getBody());
  entry.latency = std::chrono::duration<double, std::milli>(elapsed).count();
  entry.ip = request->peerAddr().toIp();
  entry.inBytes = request->bodyLength();
  entry.outBytes = response->getBodyLength();
  entry.error = error;
  logger.write(entry);
  callback(response);
}

drogon::Task<drogon::HttpResponsePtr> PipelineBatchController::batchStop(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  if (body && (*body).isMember(pipeline::KEY_IDS) && (*body)[pipeline::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[pipeline::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = pipeline::BATCH_DATA();
  for (const auto& id : ids) {
    Json::Value current;
    bool found = false;
    try {
      current = service.get(id);
      found = true;
    } catch (const std::exception&) {
      found = false;
    }
    Json::Value stoppedRow;
    bool stoppedOk = false;
    if (found) {
      const std::string containerName = std::string(pipeline::CONTAINER_PREFIX) + current[pipeline::KEY_NAME].asString();
      Json::Value startTime = current[pipeline::KEY_START_TIME];
      Json::Value duration = current[pipeline::KEY_DURATION];
      const std::string baseUrl = "http://" + containerName + ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
      try {
        auto statusRequest = drogon::HttpRequest::newHttpRequest();
        statusRequest->setMethod(drogon::Get);
        statusRequest->setPath(pipeline::DEEPSTREAM_STATUS_PATH);
        auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
        const auto polled = co_await deepstream->sendRequestCoro(statusRequest, pipeline::STATUS_TIMEOUT_SECONDS);
        Json::Value statusBody;
        const bool parsed = polled->statusCode() >= drogon::k200OK && polled->statusCode() < drogon::k300MultipleChoices &&
                            Json::Reader().parse(std::string(polled->getBody()), statusBody);
        const Json::Value payload = parsed && statusBody.isObject() && statusBody.isMember("data") && statusBody["data"].isObject()
                                        ? statusBody["data"]
                                        : statusBody;
        const bool known = parsed && payload.isObject();
        startTime = known && payload.isMember(pipeline::KEY_START_TIME) ? payload[pipeline::KEY_START_TIME] : startTime;
        duration = known && payload.isMember(pipeline::KEY_DURATION) ? payload[pipeline::KEY_DURATION] : duration;
        if (known && payload.isMember(pipeline::KEY_START_TIME)) {
          service.probe(id, current[pipeline::KEY_STATUS].asString(), startTime, current[pipeline::KEY_END_TIME], duration);
        }
      } catch (const std::exception&) {
      }
      try {
        auto stopRequest = drogon::HttpRequest::newHttpRequest();
        stopRequest->setMethod(drogon::Post);
        stopRequest->setPath("/containers/" + containerName + pipeline::DOCKER_STOP_PATH);
        auto docker = drogon::HttpClient::newHttpClient(pipeline::DOCKER_BASE_URL);
        const auto stopped = co_await docker->sendRequestCoro(stopRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
        const bool ok = stopped->statusCode() == drogon::k204NoContent || stopped->statusCode() == drogon::k304NotModified;
        if (!ok) {
          throw std::runtime_error("stop failed");
        }
        const std::time_t seconds = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm utc{};
        gmtime_r(&seconds, &utc);
        char endBuffer[32];
        std::strftime(endBuffer, sizeof(endBuffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
        stoppedRow = service.probe(id, pipeline::STATUS_STOPPED, startTime, Json::Value(endBuffer), duration);
        stoppedOk = true;
      } catch (const std::exception&) {
        stoppedOk = false;
      }
    }
    Json::Value failed;
    failed[pipeline::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    const Json::Value row = stoppedOk ? stoppedRow : failedRow;
    const char* key = stoppedOk ? pipeline::KEY_UPDATED_ROWS : pipeline::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[pipeline::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[pipeline::KEY_UPDATED_ROWS].size());
  data[pipeline::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[pipeline::KEY_FAILED_ROWS].size());
  data[pipeline::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[pipeline::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, pipeline::MSG_BATCH_STOP_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineBatchController::batchRemove(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  if (body && (*body).isMember(pipeline::KEY_IDS) && (*body)[pipeline::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[pipeline::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = pipeline::BATCH_DATA();
  for (const auto& id : ids) {
    Json::Value current;
    bool found = false;
    try {
      current = service.get(id);
      found = true;
    } catch (const std::exception&) {
      found = false;
    }
    Json::Value removedRow;
    bool removed = false;
    if (found) {
      const std::string containerName = std::string(pipeline::CONTAINER_PREFIX) + current[pipeline::KEY_NAME].asString();
      try {
        auto docker = drogon::HttpClient::newHttpClient(pipeline::DOCKER_BASE_URL);
        auto inspectRequest = drogon::HttpRequest::newHttpRequest();
        inspectRequest->setMethod(drogon::Get);
        inspectRequest->setPath("/containers/" + containerName + pipeline::DOCKER_JSON_PATH);
        const auto inspected = co_await docker->sendRequestCoro(inspectRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
        const bool exists = inspected->statusCode() == drogon::k200OK;
        const bool known = exists || inspected->statusCode() == drogon::k404NotFound;
        if (!known) {
          throw std::runtime_error("remove failed");
        }
        if (exists) {
          auto stopRequest = drogon::HttpRequest::newHttpRequest();
          stopRequest->setMethod(drogon::Post);
          stopRequest->setPath("/containers/" + containerName + pipeline::DOCKER_STOP_PATH);
          const auto stopped = co_await docker->sendRequestCoro(stopRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
          const bool stoppedOk = stopped->statusCode() == drogon::k204NoContent || stopped->statusCode() == drogon::k304NotModified;
          if (!stoppedOk) {
            throw std::runtime_error("remove failed");
          }
          auto deleteRequest = drogon::HttpRequest::newHttpRequest();
          deleteRequest->setMethod(drogon::Delete);
          deleteRequest->setPath("/containers/" + containerName);
          const auto deleted = co_await docker->sendRequestCoro(deleteRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
          if (deleted->statusCode() != drogon::k204NoContent) {
            throw std::runtime_error("remove failed");
          }
        }
        removedRow = service.remove(id);
        removed = true;
      } catch (const std::exception&) {
        removed = false;
      }
    }
    Json::Value failed;
    failed[pipeline::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    const Json::Value row = removed ? removedRow : failedRow;
    const char* key = removed ? pipeline::KEY_UPDATED_ROWS : pipeline::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[pipeline::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[pipeline::KEY_UPDATED_ROWS].size());
  data[pipeline::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[pipeline::KEY_FAILED_ROWS].size());
  data[pipeline::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[pipeline::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, pipeline::MSG_BATCH_REMOVE_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}
