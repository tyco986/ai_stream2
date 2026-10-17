#include "controllers/pipeline_controller.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/proxy_const.hpp"
#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <drogon/HttpClient.h>

#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

PipelineController::PipelineController() : service(Database::client()) {}

void PipelineController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(pipeline::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(pipeline::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? pipeline::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? pipeline::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    respond(request,
            http::Response::ok(service.list(request->getParameter(pipeline::QUERY_SEARCH), page, pageSize),
                               pipeline::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.map(), pipeline::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline::MSG_MAP_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& pipeline_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.get(pipeline_id), pipeline::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

std::string PipelineController::yaml2Multipart(const std::string& yaml) {
  const std::string boundary = pipeline::MULTIPART_BOUNDARY;
  const std::string body = "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"" + std::string(pipeline::FIELD_INPUT) +
      "\"; filename=\"" + std::string(pipeline::CONFIG_FILENAME) + "\"\r\n"
      "Content-Type: application/yaml\r\n\r\n" +
      yaml + "\r\n--" + boundary + "--\r\n";
  return body;
}

std::string PipelineController::zip2Multipart(const std::string& zip) {
  const std::string boundary = pipeline::MULTIPART_BOUNDARY;
  const std::string body = "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"" + std::string(pipeline::FIELD_INPUT) +
      "\"; filename=\"" + std::string(pipeline::ZIP_FILENAME) + "\"\r\n"
      "Content-Type: application/zip\r\n\r\n" +
      zip + "\r\n--" + boundary + "--\r\n";
  return body;
}

drogon::Task<Json::Value> PipelineController::postGenerate(const std::string& yaml) {
  auto generateRequest = drogon::HttpRequest::newHttpRequest();
  generateRequest->setMethod(drogon::Post);
  generateRequest->setPath(pipeline::PATH_GENERATE);
  generateRequest->addHeader("content-type", pipeline::MULTIPART_CONTENT_TYPE);
  generateRequest->setBody(yaml2Multipart(yaml));
  auto client = drogon::HttpClient::newHttpClient(generator::BASE_URL);
  const auto generated = co_await client->sendRequestCoro(generateRequest, pipeline::GENERATE_TIMEOUT_SECONDS);
  const bool accepted = generated->statusCode() >= drogon::k200OK && generated->statusCode() < drogon::k300MultipleChoices;
  if (!accepted) {
    throw std::runtime_error("generate failed");
  }
  Json::Value envelope;
  const bool read = Json::Reader().parse(std::string(generated->getBody()), envelope);
  const Json::Value data = read && envelope.isMember("data") ? envelope["data"] : Json::Value();
  const bool ready = data.isObject() && data.isMember(pipeline::KEY_CONFIG_PATH);
  if (!ready) {
    throw std::runtime_error("generate failed");
  }
  co_return data;
}

drogon::Task<bool> PipelineController::runDeepstreamContainer(const std::string& containerName) {
  auto docker = drogon::HttpClient::newHttpClient(pipeline::DOCKER_BASE_URL);
  auto inspectRequest = drogon::HttpRequest::newHttpRequest();
  inspectRequest->setMethod(drogon::Get);
  inspectRequest->setPath("/containers/" + containerName + pipeline::DOCKER_JSON_PATH);
  const auto inspected = co_await docker->sendRequestCoro(inspectRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
  const bool exists = inspected->statusCode() == drogon::k200OK;
  bool ready = exists;
  if (!exists) {
    Json::Value body;
    body["Image"] = pipeline::DEEPSTREAM_IMAGE;
    body["HostConfig"]["NetworkMode"] = pipeline::DOCKER_NETWORK;
    auto createRequest = drogon::HttpRequest::newHttpJsonRequest(body);
    createRequest->setMethod(drogon::Post);
    createRequest->setPath(std::string(pipeline::DOCKER_CREATE_PATH) + "?name=" + containerName);
    const auto created = co_await docker->sendRequestCoro(createRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
    ready = created->statusCode() == drogon::k201Created;
  }
  bool restarted = false;
  if (ready) {
    auto restartRequest = drogon::HttpRequest::newHttpRequest();
    restartRequest->setMethod(drogon::Post);
    restartRequest->setPath("/containers/" + containerName + pipeline::DOCKER_RESTART_PATH);
    const auto restartedResponse = co_await docker->sendRequestCoro(restartRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
    restarted = restartedResponse->statusCode() == drogon::k204NoContent;
  }
  bool healthy = false;
  const std::string baseUrl = "http://" + containerName + ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
  for (int attempt = 0; restarted && attempt < pipeline::HEALTH_ATTEMPTS && !healthy; ++attempt) {
    co_await drogon::sleepCoro(std::chrono::duration<double>(pipeline::HEALTH_INTERVAL_SECONDS));
    try {
      auto healthRequest = drogon::HttpRequest::newHttpRequest();
      healthRequest->setMethod(drogon::Get);
      healthRequest->setPath(pipeline::DEEPSTREAM_HEALTH_PATH);
      auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
      const auto health = co_await deepstream->sendRequestCoro(healthRequest, pipeline::HEALTH_TIMEOUT_SECONDS);
      healthy = health->statusCode() >= drogon::k200OK && health->statusCode() < drogon::k300MultipleChoices;
    } catch (const std::exception&) {
      healthy = false;
    }
  }
  co_return healthy;
}

drogon::Task<Json::Value> PipelineController::postDeepstreamStart(const std::string& containerName,
                                                                      const std::string& configPath) {
  const bool exists = std::filesystem::is_regular_file(configPath);
  std::string zip;
  if (exists) {
    std::ifstream file(configPath, std::ios::binary);
    zip.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  }
  const std::string baseUrl = "http://" + containerName + ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
  bool posted = false;
  if (exists && !zip.empty()) {
    auto startRequest = drogon::HttpRequest::newHttpRequest();
    startRequest->setMethod(drogon::Post);
    startRequest->setPath(pipeline::DEEPSTREAM_START_PATH);
    startRequest->addHeader("content-type", pipeline::MULTIPART_CONTENT_TYPE);
    startRequest->setBody(zip2Multipart(zip));
    auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
    const auto started = co_await deepstream->sendRequestCoro(startRequest, pipeline::START_TIMEOUT_SECONDS);
    posted = started->statusCode() >= drogon::k200OK && started->statusCode() < drogon::k300MultipleChoices;
  }
  std::string status = posted ? std::string(pipeline::STATUS_STARTING) : std::string(pipeline::STATUS_STOPPED);
  Json::Value startTime;
  Json::Value endTime;
  Json::Value duration;
  for (int attempt = 0;
       posted && status != pipeline::STATUS_RUNNING && status != pipeline::STATUS_STOPPED && attempt < pipeline::STATUS_ATTEMPTS;
       ++attempt) {
    co_await drogon::sleepCoro(std::chrono::duration<double>(pipeline::HEALTH_INTERVAL_SECONDS));
    try {
      auto statusRequest = drogon::HttpRequest::newHttpRequest();
      statusRequest->setMethod(drogon::Get);
      statusRequest->setPath(pipeline::DEEPSTREAM_STATUS_PATH);
      auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
      const auto polled = co_await deepstream->sendRequestCoro(statusRequest, pipeline::STATUS_TIMEOUT_SECONDS);
      Json::Value body;
      const bool read = polled->statusCode() >= drogon::k200OK && polled->statusCode() < drogon::k300MultipleChoices &&
                        Json::Reader().parse(std::string(polled->getBody()), body);
      const Json::Value payload = read && body.isObject() && body.isMember("data") && body["data"].isObject() ? body["data"] : body;
      const bool known = read && payload.isObject() && payload.isMember(pipeline::KEY_STATUS) && payload[pipeline::KEY_STATUS].isString();
      status = known ? payload[pipeline::KEY_STATUS].asString() : status;
      startTime = known && payload.isMember(pipeline::KEY_START_TIME) ? payload[pipeline::KEY_START_TIME] : startTime;
      endTime = known && payload.isMember(pipeline::KEY_END_TIME) ? payload[pipeline::KEY_END_TIME] : endTime;
      duration = known && payload.isMember(pipeline::KEY_DURATION) ? payload[pipeline::KEY_DURATION] : duration;
    } catch (const std::exception&) {
    }
  }
  const bool terminal = status == pipeline::STATUS_RUNNING || status == pipeline::STATUS_STOPPED;
  Json::Value data;
  if (!posted) {
    data[pipeline::KEY_STATUS] = pipeline::STATUS_STOPPED;
  } else if (terminal) {
    data[pipeline::KEY_STATUS] = status;
    data[pipeline::KEY_START_TIME] = startTime;
    data[pipeline::KEY_END_TIME] = endTime;
    data[pipeline::KEY_DURATION] = duration;
  }
  co_return data;
}

void PipelineController::respond(
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

drogon::Task<drogon::HttpResponsePtr> PipelineController::create(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_CREATE_ERROR);
  std::string errorText;
  try {
    Json::Value body;
    const bool parsed = Json::Reader().parse(std::string(request->getBody()), body) && body.isObject();
    if (!parsed) {
      throw std::runtime_error("invalid pipeline");
    }
    const Json::Value data = co_await postGenerate(service.createStart(body[pipeline::KEY_INPUT]));
    response = http::Response::ok(service.createEnd(Json::Value(),
                                                    data[pipeline::KEY_CONFIG_PATH],
                                                    body[pipeline::KEY_NAME],
                                                    body[pipeline::KEY_TYPE],
                                                    body[pipeline::KEY_ANALYZER_IDS],
                                                    body[pipeline::KEY_STREAM_IDS],
                                                    body[pipeline::KEY_GIE_IDS],
                                                    body[pipeline::KEY_INPUT]),
                                  pipeline::MSG_CREATE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_CREATE_ERROR);
    errorText = error.what();
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::update(const drogon::HttpRequestPtr& request,
                                                                 const std::string& pipeline_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_UPDATE_ERROR);
  std::string errorText;
  try {
    const Json::Value current = service.get(pipeline_id);
    const std::string containerName = std::string(pipeline::CONTAINER_PREFIX) + current[pipeline::KEY_NAME].asString();
    const std::string baseUrl = "http://" + containerName + ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
    bool running = false;
    try {
      auto statusRequest = drogon::HttpRequest::newHttpRequest();
      statusRequest->setMethod(drogon::Get);
      statusRequest->setPath(pipeline::DEEPSTREAM_STATUS_PATH);
      auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
      const auto polled = co_await deepstream->sendRequestCoro(statusRequest, pipeline::STATUS_TIMEOUT_SECONDS);
      Json::Value body;
      const bool parsed = polled->statusCode() >= drogon::k200OK && polled->statusCode() < drogon::k300MultipleChoices &&
                          Json::Reader().parse(std::string(polled->getBody()), body);
      const Json::Value payload = parsed && body.isObject() && body.isMember("data") && body["data"].isObject() ? body["data"] : body;
      const bool known = parsed && payload.isObject() && payload.isMember(pipeline::KEY_STATUS) && payload[pipeline::KEY_STATUS].isString();
      running = known && payload[pipeline::KEY_STATUS].asString() == pipeline::STATUS_RUNNING;
    } catch (const std::exception&) {
      running = false;
    }
    const bool starting = current[pipeline::KEY_STATUS].asString() == pipeline::STATUS_STARTING;
    if (running || starting) {
      throw std::runtime_error(pipeline::ERR_BUSY);
    }
    Json::Value body;
    const bool parsed = Json::Reader().parse(std::string(request->getBody()), body) && body.isObject();
    if (!parsed) {
      throw std::runtime_error("invalid pipeline");
    }
    const Json::Value data = co_await postGenerate(service.createStart(body[pipeline::KEY_INPUT]));
    response = http::Response::ok(service.update(pipeline_id,
                                                    Json::Value(pipeline::STATUS_STOPPED),
                                                    data[pipeline::KEY_CONFIG_PATH],
                                                    body[pipeline::KEY_NAME],
                                                    body[pipeline::KEY_TYPE],
                                                    body[pipeline::KEY_ANALYZER_IDS],
                                                    body[pipeline::KEY_GIE_IDS],
                                                    body[pipeline::KEY_INPUT]),
                                  pipeline::MSG_UPDATE_OK);
  } catch (const std::exception& error) {
    const bool conflict = std::string(error.what()) == pipeline::ERR_BUSY;
    response = http::Response::error(conflict ? drogon::k409Conflict : drogon::k500InternalServerError,
                                     conflict ? pipeline::ERR_BUSY : pipeline::MSG_UPDATE_ERROR);
    errorText = error.what();
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::start(const drogon::HttpRequestPtr& request,
                                                                const std::string& pipeline_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_START_ERROR);
  std::string errorText;
  try {
    const Json::Value current = service.get(pipeline_id);
    const std::string containerName = std::string(pipeline::CONTAINER_PREFIX) + current[pipeline::KEY_NAME].asString();
    const std::string configPath = current[pipeline::KEY_CONFIG_PATH].asString();
    const std::string baseUrl = "http://" + containerName + ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
    bool running = false;
    try {
      auto statusRequest = drogon::HttpRequest::newHttpRequest();
      statusRequest->setMethod(drogon::Get);
      statusRequest->setPath(pipeline::DEEPSTREAM_STATUS_PATH);
      auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
      const auto polled = co_await deepstream->sendRequestCoro(statusRequest, pipeline::STATUS_TIMEOUT_SECONDS);
      Json::Value body;
      const bool parsed = polled->statusCode() >= drogon::k200OK && polled->statusCode() < drogon::k300MultipleChoices &&
                          Json::Reader().parse(std::string(polled->getBody()), body);
      const Json::Value payload = parsed && body.isObject() && body.isMember("data") && body["data"].isObject() ? body["data"] : body;
      const bool known = parsed && payload.isObject() && payload.isMember(pipeline::KEY_STATUS) && payload[pipeline::KEY_STATUS].isString();
      running = known && payload[pipeline::KEY_STATUS].asString() == pipeline::STATUS_RUNNING;
    } catch (const std::exception&) {
      running = false;
    }
    const bool starting = current[pipeline::KEY_STATUS].asString() == pipeline::STATUS_STARTING;
    if (running || starting) {
      throw std::runtime_error(pipeline::ERR_BUSY);
    }
    service.setStatus(pipeline_id, pipeline::STATUS_STARTING);
    bool up = false;
    try {
      up = co_await runDeepstreamContainer(containerName);
    } catch (const std::exception&) {
      up = false;
    }
    Json::Value data = service.setStatus(pipeline_id, pipeline::STATUS_STOPPED);
    if (up) {
      try {
        const Json::Value polled = co_await postDeepstreamStart(containerName, configPath);
        const bool terminal = polled.isObject() && polled.isMember(pipeline::KEY_STATUS) &&
                              (polled[pipeline::KEY_STATUS].asString() == pipeline::STATUS_RUNNING ||
                               polled[pipeline::KEY_STATUS].asString() == pipeline::STATUS_STOPPED);
        const bool measured = terminal && polled.isMember(pipeline::KEY_START_TIME);
        data = measured ? service.probe(pipeline_id,
                                        polled[pipeline::KEY_STATUS].asString(),
                                        polled[pipeline::KEY_START_TIME],
                                        polled[pipeline::KEY_END_TIME],
                                        polled[pipeline::KEY_DURATION])
                        : (terminal ? service.setStatus(pipeline_id, polled[pipeline::KEY_STATUS].asString())
                                    : service.get(pipeline_id));
      } catch (const std::exception&) {
        data = service.setStatus(pipeline_id, pipeline::STATUS_STOPPED);
      }
    }
    response = http::Response::ok(data, pipeline::MSG_START_OK);
  } catch (const std::exception& error) {
    const bool conflict = std::string(error.what()) == pipeline::ERR_BUSY;
    response = http::Response::error(conflict ? drogon::k409Conflict : drogon::k500InternalServerError,
                                     conflict ? pipeline::ERR_BUSY : pipeline::MSG_START_ERROR);
    errorText = error.what();
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::probe(const drogon::HttpRequestPtr& request,
                                                                const std::string& pipeline_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_PROBE_ERROR);
  std::string errorText;
  try {
    const Json::Value current = service.get(pipeline_id);
    const std::string baseUrl = "http://" + std::string(pipeline::CONTAINER_PREFIX) + current[pipeline::KEY_NAME].asString() +
                                ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
    auto statusRequest = drogon::HttpRequest::newHttpRequest();
    statusRequest->setMethod(drogon::Get);
    statusRequest->setPath(pipeline::DEEPSTREAM_STATUS_PATH);
    auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
    const auto polled = co_await deepstream->sendRequestCoro(statusRequest, pipeline::STATUS_TIMEOUT_SECONDS);
    Json::Value body;
    const bool parsed = polled->statusCode() >= drogon::k200OK && polled->statusCode() < drogon::k300MultipleChoices &&
                        Json::Reader().parse(std::string(polled->getBody()), body);
    const Json::Value payload = parsed && body.isObject() && body.isMember("data") && body["data"].isObject() ? body["data"] : body;
    const bool known = parsed && payload.isObject() && payload.isMember(pipeline::KEY_STATUS) && payload[pipeline::KEY_STATUS].isString();
    if (!known) {
      throw std::runtime_error("probe failed");
    }
    const Json::Value startTime = payload.isMember(pipeline::KEY_START_TIME) ? payload[pipeline::KEY_START_TIME] : Json::Value();
    const Json::Value endTime = payload.isMember(pipeline::KEY_END_TIME) ? payload[pipeline::KEY_END_TIME] : Json::Value();
    const Json::Value duration = payload.isMember(pipeline::KEY_DURATION) ? payload[pipeline::KEY_DURATION] : Json::Value();
    response = http::Response::ok(service.probe(pipeline_id, payload[pipeline::KEY_STATUS].asString(), startTime, endTime, duration),
                                  pipeline::MSG_PROBE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_PROBE_ERROR);
    errorText = error.what();
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::stop(const drogon::HttpRequestPtr& request,
                                                               const std::string& pipeline_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_STOP_ERROR);
  std::string errorText;
  try {
    const Json::Value current = service.get(pipeline_id);
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
      Json::Value body;
      const bool parsed = polled->statusCode() >= drogon::k200OK && polled->statusCode() < drogon::k300MultipleChoices &&
                          Json::Reader().parse(std::string(polled->getBody()), body);
      const Json::Value payload = parsed && body.isObject() && body.isMember("data") && body["data"].isObject() ? body["data"] : body;
      const bool known = parsed && payload.isObject();
      startTime = known && payload.isMember(pipeline::KEY_START_TIME) ? payload[pipeline::KEY_START_TIME] : startTime;
      duration = known && payload.isMember(pipeline::KEY_DURATION) ? payload[pipeline::KEY_DURATION] : duration;
      if (known && payload.isMember(pipeline::KEY_START_TIME)) {
        service.probe(pipeline_id, current[pipeline::KEY_STATUS].asString(), startTime, current[pipeline::KEY_END_TIME], duration);
      }
    } catch (const std::exception&) {
    }
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
    response = http::Response::ok(service.probe(pipeline_id, pipeline::STATUS_STOPPED, startTime, Json::Value(endBuffer), duration),
                                  pipeline::MSG_STOP_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_STOP_ERROR);
    errorText = error.what();
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::remove(const drogon::HttpRequestPtr& request,
                                                                 const std::string& pipeline_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_REMOVE_ERROR);
  std::string errorText;
  try {
    const Json::Value current = service.get(pipeline_id);
    const std::string containerName = std::string(pipeline::CONTAINER_PREFIX) + current[pipeline::KEY_NAME].asString();
    auto docker = drogon::HttpClient::newHttpClient(pipeline::DOCKER_BASE_URL);
    auto inspectRequest = drogon::HttpRequest::newHttpRequest();
    inspectRequest->setMethod(drogon::Get);
    inspectRequest->setPath("/containers/" + containerName + pipeline::DOCKER_JSON_PATH);
    const auto inspected = co_await docker->sendRequestCoro(inspectRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
    const bool exists = inspected->statusCode() == drogon::k200OK;
    if (inspected->statusCode() != drogon::k200OK && inspected->statusCode() != drogon::k404NotFound) {
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
    response = http::Response::ok(service.remove(pipeline_id), pipeline::MSG_REMOVE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_REMOVE_ERROR);
    errorText = error.what();
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}
