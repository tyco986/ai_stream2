#include "controllers/pipeline_controller.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/proxy_const.hpp"
#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <drogon/HttpClient.h>

#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
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
    logger.respond(request,
            http::Response::ok(service.list(request->getParameter(pipeline::QUERY_SEARCH), page, pageSize),
                               pipeline::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
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
    logger.respond(request, http::Response::ok(service.map(), pipeline::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
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
    logger.respond(request, http::Response::ok(service.get(pipeline_id), pipeline::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

PipelineController::ZipPart PipelineController::filePart(const std::string& body) {
  const std::string marker = "name=\"" + std::string(pipeline::FIELD_FILE) + "\"; filename=\"";
  const std::size_t named = body.find(marker);
  const std::size_t nameEnd = named == std::string::npos ? std::string::npos : body.find('"', named + marker.size());
  const std::size_t headerEnd = nameEnd == std::string::npos ? std::string::npos : body.find("\r\n\r\n", nameEnd);
  const std::string closing = "\r\n--" + std::string(pipeline::GENERATOR_BOUNDARY) + "--";
  const std::size_t end = headerEnd == std::string::npos ? std::string::npos : body.rfind(closing);
  const bool ready = headerEnd != std::string::npos && end != std::string::npos && end >= headerEnd + 4;
  ZipPart part;
  part.filename = ready ? std::filesystem::path(body.substr(named + marker.size(), nameEnd - named - marker.size())).filename().string() : "";
  part.content = ready ? body.substr(headerEnd + 4, end - (headerEnd + 4)) : "";
  if (!ready || part.filename.empty() || part.content.empty()) {
    throw std::runtime_error("generate failed");
  }
  return part;
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

std::string PipelineController::zips2Multipart(const std::vector<ZipPart>& files) {
  const std::string boundary = pipeline::MULTIPART_BOUNDARY;
  std::string body;
  for (const ZipPart& file : files) {
    body += "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"" + file.field +
        "\"; filename=\"" + file.filename + "\"\r\n"
        "Content-Type: application/zip\r\n\r\n" +
        file.content + "\r\n";
  }
  body += "--" + boundary + "--\r\n";
  return body;
}

drogon::Task<Json::Value> PipelineController::postGenerate(const std::string& yaml) {
  auto generateRequest = drogon::HttpRequest::newHttpRequest();
  generateRequest->setMethod(drogon::Post);
  generateRequest->setPath(pipeline::PATH_GENERATE);
  generateRequest->setContentTypeString(pipeline::MULTIPART_CONTENT_TYPE);
  generateRequest->setBody(yaml2Multipart(yaml));
  auto client = drogon::HttpClient::newHttpClient(generator::DES_BASE_URL);
  const auto generated = co_await client->sendRequestCoro(generateRequest, pipeline::GENERATE_TIMEOUT_SECONDS);
  const bool accepted = generated->statusCode() >= drogon::k200OK && generated->statusCode() < drogon::k300MultipleChoices;
  if (!accepted) {
    throw std::runtime_error("generate failed");
  }
  const ZipPart zip = filePart(std::string(generated->getBody()));
  std::filesystem::create_directories(pipeline::CONFIG_DIR);
  const std::filesystem::path path = std::filesystem::path(pipeline::CONFIG_DIR) / zip.filename;
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output.write(zip.content.data(), static_cast<std::streamsize>(zip.content.size()));
  if (!output) {
    throw std::runtime_error("generate failed");
  }
  Json::Value data;
  data[pipeline::KEY_CONFIG_PATH] = path.string();
  co_return data;
}

std::string PipelineController::logsBind() {
  std::ifstream mounts(pipeline::MOUNTINFO_PATH);
  std::string host;
  std::string line;
  const std::string logsDir = pipeline::LOGS_DIR;
  while (std::getline(mounts, line)) {
    std::istringstream row(line);
    std::string mountId;
    std::string parent;
    std::string device;
    std::string root;
    std::string point;
    row >> mountId >> parent >> device >> root >> point;
    const bool matched = point == logsDir || point.rfind(logsDir + "/", 0) == 0;
    const std::size_t extra = matched ? point.size() - logsDir.size() : 0;
    const bool usable = matched && root.size() >= extra;
    host = usable ? root.substr(0, root.size() - extra) : host;
  }
  if (host.empty()) {
    throw std::runtime_error("logs mount missing");
  }
  return host + ":" + logsDir;
}

drogon::Task<bool> PipelineController::runDeepstreamContainer(const std::string& containerName,
                                                                       const std::string& pipelineName) {
  auto docker = drogon::HttpClient::newHttpClient(pipeline::DOCKER_BASE_URL);
  const std::string baseUrl = "http://" + containerName + ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
  auto inspectRequest = drogon::HttpRequest::newHttpRequest();
  inspectRequest->setMethod(drogon::Get);
  inspectRequest->setPath("/containers/" + containerName + pipeline::DOCKER_JSON_PATH);
  const auto inspected = co_await docker->sendRequestCoro(inspectRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
  const bool exists = inspected->statusCode() == drogon::k200OK;
  bool running = false;
  if (exists) {
    try {
      auto statusRequest = drogon::HttpRequest::newHttpRequest();
      statusRequest->setMethod(drogon::Get);
      statusRequest->setPath(pipeline::DEEPSTREAM_STATUS_PATH);
      auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
      const auto polled = co_await deepstream->sendRequestCoro(statusRequest, pipeline::STATUS_TIMEOUT_SECONDS);
      Json::Value statusBody;
      const bool parsed = polled->statusCode() >= drogon::k200OK && polled->statusCode() < drogon::k300MultipleChoices &&
                          Json::Reader().parse(std::string(polled->getBody()), statusBody);
      const Json::Value payload = parsed && statusBody.isObject() && statusBody.isMember("data") && statusBody["data"].isObject() ? statusBody["data"] : statusBody;
      const bool known = parsed && payload.isObject() && payload.isMember(pipeline::KEY_STATUS) && payload[pipeline::KEY_STATUS].isString();
      running = known && payload[pipeline::KEY_STATUS].asString() == pipeline::STATUS_RUNNING;
    } catch (const std::exception&) {
      running = false;
    }
  }
  if (running) {
    throw std::runtime_error(pipeline::ERR_BUSY);
  }
  bool ready = exists;
  if (!exists) {
    Json::Value gpu;
    gpu["Driver"] = pipeline::DOCKER_GPU_DRIVER;
    gpu["Count"] = pipeline::DOCKER_GPU_COUNT;
    gpu["Capabilities"][0][0] = pipeline::DOCKER_GPU_CAPABILITY;
    Json::Value body;
    body["Image"] = pipeline::DEEPSTREAM_IMAGE;
    body["Cmd"][0] = pipeline::DEEPSTREAM_BIN;
    body["Cmd"][1] = pipelineName;
    body["HostConfig"]["NetworkMode"] = pipeline::DOCKER_NETWORK;
    body["HostConfig"]["Runtime"] = pipeline::DOCKER_RUNTIME;
    body["HostConfig"]["Binds"][0] = logsBind();
    body["HostConfig"]["Binds"][1] = pipeline::LOCALTIME_BIND;
    body["HostConfig"]["DeviceRequests"][0] = gpu;
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
  for (int attempt = 0; restarted && attempt < pipeline::HEALTH_ATTEMPTS && !healthy; ++attempt) {
    co_await drogon::sleepCoro(drogon::app().getLoop(), pipeline::HEALTH_INTERVAL_SECONDS);
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
  std::vector<ZipPart> files;
  if (exists) {
    std::ifstream configFile(configPath, std::ios::binary);
    std::string configZip = std::string(std::istreambuf_iterator<char>(configFile), std::istreambuf_iterator<char>());
    files.push_back(ZipPart{pipeline::FIELD_CONFIG, std::filesystem::path(configPath).filename().string(), configZip});
    const YAML::Node params = service.readParams(configPath);
    for (const std::string& path : service.getModelPaths(params)) {
      const bool modelExists = std::filesystem::is_regular_file(path);
      if (!modelExists) {
        throw std::runtime_error("model zip missing");
      }
      std::ifstream modelFile(path, std::ios::binary);
      std::string modelZip = std::string(std::istreambuf_iterator<char>(modelFile), std::istreambuf_iterator<char>());
      files.push_back(ZipPart{pipeline::FIELD_MODEL, std::filesystem::path(path).filename().string(), modelZip});
    }
    const std::string mediaZip = service.packMedia(params);
    if (!mediaZip.empty()) {
      files.push_back(ZipPart{pipeline::FIELD_MEDIA, pipeline::MEDIA_FILENAME, mediaZip});
    }
  }
  const std::string baseUrl = "http://" + containerName + ":" + std::to_string(pipeline::DEEPSTREAM_PORT);
  bool posted = false;
  if (!files.empty() && !files.front().content.empty()) {
    auto startRequest = drogon::HttpRequest::newHttpRequest();
    startRequest->setMethod(drogon::Post);
    startRequest->setPath(pipeline::DEEPSTREAM_START_PATH);
    startRequest->setContentTypeString(pipeline::MULTIPART_CONTENT_TYPE);
    startRequest->setBody(zips2Multipart(files));
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
    co_await drogon::sleepCoro(drogon::app().getLoop(), pipeline::HEALTH_INTERVAL_SECONDS);
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

drogon::Task<drogon::HttpResponsePtr> PipelineController::create(drogon::HttpRequestPtr request) {
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
                                                    body[pipeline::KEY_MEDIA_IDS],
                                                    body[pipeline::KEY_GIE_IDS],
                                                    body[pipeline::KEY_INPUT]),
                                  pipeline::MSG_CREATE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_CREATE_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::update(drogon::HttpRequestPtr request,
                                                                 std::string pipeline_id) {
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
                                                    body[pipeline::KEY_STREAM_IDS],
                                                    body[pipeline::KEY_MEDIA_IDS],
                                                    body[pipeline::KEY_GIE_IDS],
                                                    body[pipeline::KEY_INPUT]),
                                  pipeline::MSG_UPDATE_OK);
  } catch (const std::exception& error) {
    const bool conflict = std::string(error.what()) == pipeline::ERR_BUSY;
    response = http::Response::error(conflict ? drogon::k409Conflict : drogon::k500InternalServerError,
                                     conflict ? pipeline::ERR_BUSY : pipeline::MSG_UPDATE_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::start(drogon::HttpRequestPtr request,
                                                                std::string pipeline_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, pipeline::MSG_START_ERROR);
  std::string errorText;
  try {
    const Json::Value current = service.get(pipeline_id);
    const std::string containerName = std::string(pipeline::CONTAINER_PREFIX) + current[pipeline::KEY_NAME].asString();
    const std::string configPath = current[pipeline::KEY_CONFIG_PATH].asString();
    const std::string previous = current[pipeline::KEY_STATUS].asString();
    const bool starting = previous == pipeline::STATUS_STARTING;
    if (starting) {
      throw std::runtime_error(pipeline::ERR_BUSY);
    }
    service.setStatus(pipeline_id, pipeline::STATUS_STARTING);
    bool up = false;
    try {
      up = co_await runDeepstreamContainer(containerName, current[pipeline::KEY_NAME].asString());
    } catch (const std::exception& error) {
      const bool busy = std::string(error.what()) == pipeline::ERR_BUSY;
      if (busy) {
        service.setStatus(pipeline_id, previous);
        throw;
      }
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
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::probe(drogon::HttpRequestPtr request,
                                                                std::string pipeline_id) {
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
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::stop(drogon::HttpRequestPtr request,
                                                               std::string pipeline_id) {
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
    stopRequest->setPath(pipeline::DEEPSTREAM_STOP_PATH);
    auto deepstream = drogon::HttpClient::newHttpClient(baseUrl);
    const auto stopped = co_await deepstream->sendRequestCoro(stopRequest, pipeline::DOCKER_TIMEOUT_SECONDS);
    const bool idle = stopped->statusCode() == drogon::k409Conflict;
    const bool ok = stopped->statusCode() >= drogon::k200OK && stopped->statusCode() < drogon::k300MultipleChoices;
    if (idle) {
      throw std::runtime_error(pipeline::ERR_IDLE);
    }
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
    const bool idle = std::string(error.what()) == pipeline::ERR_IDLE;
    response = http::Response::error(idle ? drogon::k409Conflict : drogon::k500InternalServerError,
                                     idle ? pipeline::ERR_IDLE : pipeline::MSG_STOP_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> PipelineController::remove(drogon::HttpRequestPtr request,
                                                                 std::string pipeline_id) {
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
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}
