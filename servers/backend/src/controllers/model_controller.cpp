#include "controllers/model_controller.hpp"

#include "constants/proxy_const.hpp"
#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <drogon/HttpClient.h>
#include <drogon/MultiPart.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>

ModelController::ModelController() : service(Database::client()) {}

std::string ModelController::zip2Bytes(const std::string& path) {
  std::ifstream stream(path, std::ios::binary);
  const bool opened = stream.is_open();
  std::string data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  if (!opened) {
    throw std::runtime_error("model file missing");
  }
  return data;
}

std::string ModelController::zip2Multipart(const std::string& filename, const std::string& bytes) {
  const std::string body = "--" + std::string(model::MULTIPART_BOUNDARY) + "\r\n"
      "Content-Disposition: form-data; name=\"" + std::string(model::FIELD_INPUT) +
      "\"; filename=\"" + filename + "\"\r\n"
      "Content-Type: application/octet-stream\r\n\r\n" +
      bytes + "\r\n";
  return body;
}

std::string ModelController::yaml2Multipart(const std::string& yaml) {
  const std::string boundary = model::MULTIPART_BOUNDARY;
  const std::string body = "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"" + std::string(model::FIELD_CONFIG) +
      "\"; filename=\"" + std::string(model::CONFIG_FILENAME) + "\"\r\n"
      "Content-Type: application/yaml\r\n\r\n" +
      yaml + "\r\n--" + boundary + "--\r\n";
  return body;
}

void ModelController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(model::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(model::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? model::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? model::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    logger.respond(request,
            http::Response::ok(service.list(request->getParameter(model::QUERY_SEARCH), page, pageSize),
                               model::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, model::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

void ModelController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::MultiPartParser parser;
  const bool parsed = parser.parse(request) == 0;
  const auto& fields = parser.getParameters();
  const Json::Value name = fields.count(model::KEY_NAME) ? Json::Value(fields.at(model::KEY_NAME)) : Json::Value("");
  const Json::Value type = fields.count(model::KEY_TYPE) ? Json::Value(fields.at(model::KEY_TYPE)) : Json::Value("");
  const Json::Value batchSize = fields.count(model::KEY_BATCH_SIZE)
      ? Json::Value(std::stoi(fields.at(model::KEY_BATCH_SIZE)))
      : Json::Value(0);
  const Json::Value batchMode =
      fields.count(model::KEY_BATCH_MODE) ? Json::Value(fields.at(model::KEY_BATCH_MODE)) : Json::Value("");
  const Json::Value precision =
      fields.count(model::KEY_PRECISION) ? Json::Value(fields.at(model::KEY_PRECISION)) : Json::Value("");
  const Json::Value optLevel =
      fields.count(model::KEY_OPT_LEVEL) ? Json::Value(std::stoi(fields.at(model::KEY_OPT_LEVEL))) : Json::Value();
  const Json::Value device =
      fields.count(model::KEY_DEVICE) ? Json::Value(std::stoi(fields.at(model::KEY_DEVICE))) : Json::Value();
  const Json::Value maxDet =
      fields.count(model::KEY_MAX_DET) ? Json::Value(std::stoi(fields.at(model::KEY_MAX_DET))) : Json::Value();
  const Json::Value conf = fields.count(model::KEY_CONF) ? Json::Value(fields.at(model::KEY_CONF)) : Json::Value();
  const Json::Value iou = fields.count(model::KEY_IOU) ? Json::Value(fields.at(model::KEY_IOU)) : Json::Value();
  try {
    drogon::HttpResponsePtr response =
        http::Response::error(drogon::k400BadRequest, model::MSG_CREATE_ERROR);
    if (parsed) {
      response = http::Response::ok(
          service.create(name,
                         type,
                         batchSize,
                         batchMode,
                         precision,
                         optLevel,
                         device,
                         maxDet,
                         conf,
                         iou),
          model::MSG_CREATE_OK);
    }
    logger.respond(request, response, started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, model::MSG_CREATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void ModelController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& model_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    logger.respond(request, http::Response::ok(service.get(model_id), model::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, model::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

void ModelController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& model_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::MultiPartParser parser;
  const bool parsed = parser.parse(request) == 0;
  const auto& fields = parser.getParameters();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k400BadRequest, model::MSG_UPDATE_ERROR);
  std::string errorText;
  try {
    if (parsed) {
      const Json::Value name =
          fields.count(model::KEY_NAME) ? Json::Value(fields.at(model::KEY_NAME)) : Json::Value();
      const Json::Value type =
          fields.count(model::KEY_TYPE) ? Json::Value(fields.at(model::KEY_TYPE)) : Json::Value();
      const Json::Value batchSize = fields.count(model::KEY_BATCH_SIZE)
          ? Json::Value(std::stoi(fields.at(model::KEY_BATCH_SIZE)))
          : Json::Value();
      const Json::Value batchMode =
          fields.count(model::KEY_BATCH_MODE) ? Json::Value(fields.at(model::KEY_BATCH_MODE)) : Json::Value();
      const Json::Value precision =
          fields.count(model::KEY_PRECISION) ? Json::Value(fields.at(model::KEY_PRECISION)) : Json::Value();
      const Json::Value optLevel =
          fields.count(model::KEY_OPT_LEVEL) ? Json::Value(std::stoi(fields.at(model::KEY_OPT_LEVEL))) : Json::Value();
      const Json::Value device =
          fields.count(model::KEY_DEVICE) ? Json::Value(std::stoi(fields.at(model::KEY_DEVICE))) : Json::Value();
      const Json::Value maxDet =
          fields.count(model::KEY_MAX_DET) ? Json::Value(std::stoi(fields.at(model::KEY_MAX_DET))) : Json::Value();
      const Json::Value conf = fields.count(model::KEY_CONF) ? Json::Value(fields.at(model::KEY_CONF)) : Json::Value();
      const Json::Value iou = fields.count(model::KEY_IOU) ? Json::Value(fields.at(model::KEY_IOU)) : Json::Value();
      response = http::Response::ok(
          service.update(model_id,
                         name,
                         type,
                         batchSize,
                         batchMode,
                         precision,
                         optLevel,
                         device,
                         maxDet,
                         conf,
                         iou),
          model::MSG_UPDATE_OK);
    }
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool conflict = reason == model::ERR_BUILDING || reason == model::ERR_BUILT;
    const char* message = reason == model::ERR_BUILT ? model::ERR_BUILT : model::MSG_UPDATE_ERROR;
    response = http::Response::error(conflict ? drogon::k409Conflict : drogon::k500InternalServerError, message);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void ModelController::importModels(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::MultiPartParser parser;
  const bool parsed = parser.parse(request) == 0;
  std::vector<ImportFile> files;
  for (const auto& file : parser.getFiles()) {
    const bool matched = file.getItemName() == model::FIELD_FILE;
    if (matched) {
      ImportFile item;
      item.fileName = file.getFileName();
      item.fileContent = std::string(file.fileData(), file.fileLength());
      files.push_back(item);
    }
  }
  drogon::HttpResponsePtr response = http::Response::error(drogon::k400BadRequest, model::MSG_IMPORT_ERROR);
  std::string errorText;
  try {
    if (parsed && !files.empty()) {
      response = http::Response::ok(service.importModel(files), model::MSG_IMPORT_OK);
    }
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, model::MSG_IMPORT_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void ModelController::upload(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& model_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::MultiPartParser parser;
  const bool parsed = parser.parse(request) == 0;
  std::string fileName;
  std::string fileContent;
  for (const auto& file : parser.getFiles()) {
    const bool matched = file.getItemName() == model::FIELD_INPUT;
    fileName = matched ? file.getFileName() : fileName;
    fileContent = matched ? std::string(file.fileData(), file.fileLength()) : fileContent;
  }
  drogon::HttpResponsePtr response = http::Response::error(drogon::k400BadRequest, model::MSG_UPLOAD_ERROR);
  std::string errorText;
  try {
    if (parsed && !fileName.empty()) {
      response = http::Response::ok(service.upload(model_id, fileName, fileContent), model::MSG_UPLOAD_OK);
    }
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool conflict = reason == model::ERR_BUILDING;
    response = http::Response::error(conflict ? drogon::k409Conflict : drogon::k500InternalServerError,
                                     conflict ? model::ERR_BUILDING : model::MSG_UPLOAD_ERROR);
    errorText = reason;
  }
  logger.respond(request, response, started, callback, errorText);
}

void ModelController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& model_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.remove(model_id), model::MSG_REMOVE_OK);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool occupied = reason == model::ERR_OCCUPIED;
    const bool conflict = reason == model::ERR_BUILDING || occupied;
    const char* message = occupied ? model::ERR_OCCUPIED : model::MSG_REMOVE_ERROR;
    response = http::Response::error(conflict ? drogon::k409Conflict : drogon::k500InternalServerError, message);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

drogon::Task<drogon::HttpResponsePtr> ModelController::build(drogon::HttpRequestPtr request,
                                                                     std::string model_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, model::MSG_BUILD_ERROR);
  std::string errorText;
  bool building = false;
  try {
    const BuildStart recipes = service.buildStart(model_id);
    building = true;
    const std::string sourcePath = service.get(model_id)[model::KEY_SOURCE_PATH].asString();
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
    const Json::Value built = service.buildEnd(model_id, trt.data, trt.fileName, trt.fileContent);
    building = false;
    response = http::Response::ok(built, model::MSG_BUILD_OK);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool conflict = reason == model::ERR_BUILDING;
    if (building) {
      service.buildEnd(model_id, Json::Value());
    }
    response = http::Response::error(conflict ? drogon::k409Conflict : drogon::k500InternalServerError,
                                     conflict ? model::ERR_BUILDING : model::MSG_BUILD_ERROR);
    errorText = reason;
  }
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

void ModelController::status(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& model_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const Json::Value model = service.get(model_id);
    Json::Value data;
    data[model::KEY_ID] = model[model::KEY_ID];
    data[model::KEY_STATUS] = model[model::KEY_STATUS];
    data[model::KEY_BUILT_AT] = model[model::KEY_BUILT_AT];
    data[model::KEY_VERSION] = model[model::KEY_VERSION];
    data[model::KEY_NUM_CLASS] = model[model::KEY_NUM_CLASS];
    data[model::KEY_CLASSES] = model[model::KEY_CLASSES];
    logger.respond(request, http::Response::ok(data, model::MSG_STATUS_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, model::MSG_STATUS_ERROR),
            started,
            callback,
            error.what());
  }
}

void ModelController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    logger.respond(request, http::Response::ok(service.map(), model::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, model::MSG_MAP_ERROR),
            started,
            callback,
            error.what());
  }
}
