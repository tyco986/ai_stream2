#include "controllers/pipeline_gie_batch_controller.hpp"

#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

PipelineGieBatchController::PipelineGieBatchController() : service(Database::client()) {}

void PipelineGieBatchController::batchRemove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value data = pipeline_gie::BATCH_DATA();
  const bool hasIds = body && (*body).isMember(pipeline_gie::KEY_IDS) && (*body)[pipeline_gie::KEY_IDS].isArray();
  if (hasIds) {
    for (const auto& node : (*body)[pipeline_gie::KEY_IDS]) {
      try {
        data[pipeline_gie::KEY_UPDATED_ROWS].append(service.remove(node.asString()));
      } catch (const std::exception&) {
        Json::Value failed;
        failed[pipeline_gie::KEY_ID] = node.asString();
        data[pipeline_gie::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[pipeline_gie::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[pipeline_gie::KEY_UPDATED_ROWS].size());
  data[pipeline_gie::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[pipeline_gie::KEY_FAILED_ROWS].size());
  data[pipeline_gie::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[pipeline_gie::KEY_SKIPPED_ROWS].size());
  logger.respond(request, http::Response::ok(data, pipeline_gie::MSG_REMOVE_OK), started, callback);
}
