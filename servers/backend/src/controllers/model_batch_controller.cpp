#include "controllers/model_batch_controller.hpp"

#include "constants/proxy_const.hpp"
#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <drogon/HttpClient.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>

ModelBatchController::ModelBatchController() : service(Database::client()) {}

std::string ModelBatchController::zip2Bytes(const std::string& path) {
  std::ifstream stream(path, std::ios::binary);
  const bool opened = stream.is_open();
  std::string data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  if (!opened) {
    throw std::runtime_error("model file missing");
  }
  return data;
}

std::string ModelBatchController::zip2Multipart(const std::string& filename, const std::string& bytes) {
  const std::string body = "--" + std::string(model::MULTIPART_BOUNDARY) + "\r\n"
      "Content-Disposition: form-data; name=\"" + std::string(model::FIELD_INPUT) +
      "\"; filename=\"" + filename + "\"\r\n"
      "Content-Type: application/octet-stream\r\n\r\n" +
      bytes + "\r\n";
  return body;
}

std::string ModelBatchController::yaml2Multipart(const std::string& yaml) {
  const std::string boundary = model::MULTIPART_BOUNDARY;
  const std::string body = "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"" + std::string(model::FIELD_CONFIG) +
      "\"; filename=\"" + std::string(model::CONFIG_FILENAME) + "\"\r\n"
      "Content-Type: application/yaml\r\n\r\n" +
      yaml + "\r\n--" + boundary + "--\r\n";
  return body;
}

void ModelBatchController::respond(
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

drogon::Task<drogon::HttpResponsePtr> ModelBatchController::batchBuild(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value data = model::BATCH_DATA();
  const bool hasIds = body && (*body).isMember(model::KEY_IDS) && (*body)[model::KEY_IDS].isArray();
  if (hasIds) {
    for (const auto& node : (*body)[model::KEY_IDS]) {
      const std::string modelId = node.asString();
      bool building = false;
      try {
        const BuildStart recipes = service.buildStart(modelId);
        building = true;
        const std::string sourcePath = service.get(modelId)[model::KEY_SOURCE_PATH].asString();
        const std::filesystem::path source(sourcePath);
        auto onnxRequest = drogon::HttpRequest::newHttpRequest();
        onnxRequest->setMethod(drogon::Post);
        onnxRequest->setPath(model::PATH_EXPORT_ONNX);
        onnxRequest->addHeader("content-type", model::MULTIPART_CONTENT_TYPE);
        onnxRequest->setBody(zip2Multipart(source.filename().string(), zip2Bytes(sourcePath)) +
                              yaml2Multipart(recipes.exportOnnx));
        auto onnxClient = drogon::HttpClient::newHttpClient(export_onnx::BASE_URL);
        const auto onnxResponse = co_await onnxClient->sendRequestCoro(onnxRequest, export_onnx::REQUEST_TIMEOUT_SECONDS);
        const bool onnxOk = onnxResponse->statusCode() >= drogon::k200OK && onnxResponse->statusCode() < drogon::k300MultipleChoices;
        if (!onnxOk) {
          throw std::runtime_error("export onnx failed");
        }
        const ExportResponse onnx = service.buildParse(onnxResponse->getHeader("content-type"), std::string(onnxResponse->getBody()));
        auto trtRequest = drogon::HttpRequest::newHttpRequest();
        trtRequest->setMethod(drogon::Post);
        trtRequest->setPath(model::PATH_EXPORT_TRT);
        trtRequest->addHeader("content-type", model::MULTIPART_CONTENT_TYPE);
        trtRequest->setBody(zip2Multipart(onnx.fileName, onnx.fileContent) + yaml2Multipart(recipes.exportTrt));
        auto trtClient = drogon::HttpClient::newHttpClient(export_trt::BASE_URL);
        const auto trtResponse = co_await trtClient->sendRequestCoro(trtRequest, export_trt::REQUEST_TIMEOUT_SECONDS);
        const bool trtOk = trtResponse->statusCode() >= drogon::k200OK && trtResponse->statusCode() < drogon::k300MultipleChoices;
        if (!trtOk) {
          throw std::runtime_error("export trt failed");
        }
        const ExportResponse trt = service.buildParse(trtResponse->getHeader("content-type"), std::string(trtResponse->getBody()));
        const Json::Value built = service.buildEnd(modelId, trt.data, trt.fileName, trt.fileContent);
        building = false;
        data[model::KEY_UPDATED_ROWS].append(built);
      } catch (const std::exception&) {
        Json::Value failed;
        failed[model::KEY_ID] = modelId;
        if (building) {
          try {
            failed = service.buildEnd(modelId, Json::Value());
          } catch (const std::exception&) {
            failed[model::KEY_ID] = modelId;
          }
        }
        data[model::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[model::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_UPDATED_ROWS].size());
  data[model::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_FAILED_ROWS].size());
  data[model::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_SKIPPED_ROWS].size());
  drogon::HttpResponsePtr response = http::Response::ok(data, model::MSG_BUILD_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {});
  co_return response;
}

void ModelBatchController::batchStatus(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value data = model::BATCH_DATA();
  const bool hasIds = body && (*body).isMember(model::KEY_IDS) && (*body)[model::KEY_IDS].isArray();
  if (hasIds) {
    for (const auto& node : (*body)[model::KEY_IDS]) {
      try {
        const Json::Value model = service.get(node.asString());
        Json::Value item;
        item[model::KEY_ID] = model[model::KEY_ID];
        item[model::KEY_STATUS] = model[model::KEY_STATUS];
        item[model::KEY_BUILT_AT] = model[model::KEY_BUILT_AT];
        item[model::KEY_VERSION] = model[model::KEY_VERSION];
        item[model::KEY_NUM_CLASS] = model[model::KEY_NUM_CLASS];
        item[model::KEY_CLASSES] = model[model::KEY_CLASSES];
        data[model::KEY_UPDATED_ROWS].append(item);
      } catch (const std::exception&) {
        Json::Value failed;
        failed[model::KEY_ID] = node.asString();
        data[model::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[model::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_UPDATED_ROWS].size());
  data[model::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_FAILED_ROWS].size());
  data[model::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_SKIPPED_ROWS].size());
  respond(request, http::Response::ok(data, model::MSG_STATUS_OK), started, callback);
}

void ModelBatchController::batchRemove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value data = model::BATCH_DATA();
  const bool hasIds = body && (*body).isMember(model::KEY_IDS) && (*body)[model::KEY_IDS].isArray();
  if (hasIds) {
    for (const auto& node : (*body)[model::KEY_IDS]) {
      try {
        data[model::KEY_UPDATED_ROWS].append(service.remove(node.asString()));
      } catch (const std::exception&) {
        Json::Value failed;
        failed[model::KEY_ID] = node.asString();
        data[model::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[model::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_UPDATED_ROWS].size());
  data[model::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_FAILED_ROWS].size());
  data[model::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_SKIPPED_ROWS].size());
  respond(request, http::Response::ok(data, model::MSG_REMOVE_OK), started, callback);
}
