#include "controllers/pipeline_analyzer_controller.hpp"

#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <drogon/MultiPart.h>

#include <exception>
#include <stdexcept>
#include <string>

PipelineAnalyzerController::PipelineAnalyzerController() : service(Database::client()) {}

AnalyzerForm PipelineAnalyzerController::multipart2Form(const drogon::HttpRequestPtr& request) {
  drogon::MultiPartParser parser;
  const bool parsed = parser.parse(request) == 0;
  if (!parsed) {
    throw std::runtime_error("invalid analyzer");
  }
  const auto& fields = parser.getParameters();
  AnalyzerForm form;
  form.name = fields.count(pipeline_analyzer::KEY_NAME) ? Json::Value(fields.at(pipeline_analyzer::KEY_NAME)) : Json::Value();
  const bool hasAnnotations = fields.count(pipeline_analyzer::KEY_ANNOTATIONS) != 0;
  const bool annotationsParsed =
      hasAnnotations && Json::Reader().parse(fields.at(pipeline_analyzer::KEY_ANNOTATIONS), form.annotations);
  form.annotations = annotationsParsed ? form.annotations : Json::Value();
  for (const auto& file : parser.getFiles()) {
    const bool matched = file.getItemName() == pipeline_analyzer::FIELD_FILE;
    form.fileName = matched ? file.getFileName() : form.fileName;
    form.fileContent = matched ? std::string(file.fileData(), file.fileLength()) : form.fileContent;
  }
  return form;
}

void PipelineAnalyzerController::respond(
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

void PipelineAnalyzerController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& analyzer_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.get(analyzer_id), pipeline_analyzer::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_analyzer::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineAnalyzerController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const AnalyzerForm form = multipart2Form(request);
    respond(request,
            http::Response::ok(service.create(form.name, form.annotations, form.fileName, form.fileContent),
                               pipeline_analyzer::MSG_CREATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_analyzer::MSG_CREATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineAnalyzerController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& analyzer_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const AnalyzerForm form = multipart2Form(request);
    respond(request,
            http::Response::ok(service.update(analyzer_id, form.name, form.annotations, form.fileName, form.fileContent),
                               pipeline_analyzer::MSG_UPDATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_analyzer::MSG_UPDATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineAnalyzerController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& analyzer_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.remove(analyzer_id), pipeline_analyzer::MSG_REMOVE_OK), started, callback);
  } catch (const std::exception& error) {
    const bool occupied = std::string(error.what()) == pipeline_analyzer::ERR_OCCUPIED;
    respond(request,
            http::Response::error(occupied ? drogon::k409Conflict : drogon::k500InternalServerError,
                                  occupied ? pipeline_analyzer::ERR_OCCUPIED : pipeline_analyzer::MSG_REMOVE_ERROR),
            started,
            callback,
            error.what());
  }
}
