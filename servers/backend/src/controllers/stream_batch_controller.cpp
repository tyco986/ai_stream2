#include "controllers/stream_batch_controller.hpp"

#include "db/database.hpp"
#include "ffmpeg/ffmpeg.hpp"
#include "mediamtx/mediamtx.hpp"
#include "response/response.hpp"
#include "log/base_logger.hpp"

#include <chrono>
#include <exception>
#include <functional>
#include <string>
#include <vector>

StreamBatchController::StreamBatchController() : service(Database::client()) {}

void StreamBatchController::respond(
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

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchRemove(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  std::vector<std::string> names;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DELETE_ERROR);
  std::string errorText;
  bool ready = false;
  try {
    for (const auto& id : ids) {
      names.push_back(service.get(id)[stream::KEY_NAME].asString());
    }
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  bool removed = ready;
  for (std::size_t index = 0; removed && index < names.size(); index += 1) {
    if (co_await MediaMtx::getPath(names[index])) {
      removed = co_await MediaMtx::deletePath(names[index]);
    }
  }
  if (removed) {
    try {
      service.batchRemove(ids);
      Json::Value data;
      data[stream::KEY_REMOVED_COUNT] = static_cast<Json::UInt64>(ids.size());
      response = http::Response::ok(data, stream::MSG_BATCH_DELETE_OK);
    } catch (const std::exception& error) {
      errorText = error.what();
    }
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchEnable(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  std::vector<std::string> names;
  std::vector<std::string> urls;
  std::vector<bool> recordings;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_ENABLE_ERROR);
  std::string errorText;
  bool ready = false;
  try {
    for (const auto& id : ids) {
      const Json::Value current = service.get(id);
      names.push_back(current[stream::KEY_NAME].asString());
      urls.push_back(current[stream::KEY_URL].asString());
      recordings.push_back(current[stream::KEY_RECORD].asBool());
    }
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  bool mounted = ready;
  std::size_t index = 0;
  for (; mounted && index < ids.size(); index += 1) {
    mounted = co_await MediaMtx::replacePath(names[index], urls[index], recordings[index]);
    if (mounted) {
      try {
        service.enable(ids[index]);
      } catch (const std::exception& error) {
        mounted = false;
        errorText = error.what();
      }
    }
  }
  if (mounted) {
    Json::Value data;
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(ids.size());
    response = http::Response::ok(data, stream::MSG_BATCH_ENABLE_OK);
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchDisable(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  std::vector<std::string> names;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DISABLE_ERROR);
  std::string errorText;
  bool ready = false;
  try {
    for (const auto& id : ids) {
      names.push_back(service.get(id)[stream::KEY_NAME].asString());
    }
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  bool done = ready;
  for (std::size_t index = 0; done && index < ids.size(); index += 1) {
    bool removed = true;
    if (co_await MediaMtx::getPath(names[index])) {
      removed = co_await MediaMtx::deletePath(names[index]);
    }
    done = removed;
    if (done) {
      try {
        service.disable(ids[index]);
      } catch (const std::exception& error) {
        done = false;
        errorText = error.what();
      }
    }
  }
  if (done) {
    Json::Value data;
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(ids.size());
    response = http::Response::ok(data, stream::MSG_BATCH_DISABLE_OK);
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchRecord(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  std::vector<std::string> names;
  std::vector<std::string> urls;
  std::vector<std::string> skipped;
  std::vector<std::string> recorded;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_RECORD_ERROR);
  std::string errorText;
  bool ready = false;
  try {
    for (const auto& id : ids) {
      const Json::Value current = service.get(id);
      names.push_back(current[stream::KEY_NAME].asString());
      urls.push_back(current[stream::KEY_URL].asString());
    }
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  bool done = ready;
  for (std::size_t index = 0; done && index < ids.size(); index += 1) {
    const bool mountedNow = co_await MediaMtx::getPath(names[index]);
    bool replaced = true;
    if (mountedNow) {
      replaced = co_await MediaMtx::replacePath(names[index], urls[index], true);
    }
    done = replaced;
    if (done && mountedNow) {
      try {
        service.record(ids[index]);
        recorded.push_back(ids[index]);
      } catch (const std::exception& error) {
        done = false;
        errorText = error.what();
      }
    }
    if (done && !mountedNow) {
      skipped.push_back(ids[index]);
    }
  }
  if (done) {
    Json::Value data;
    Json::Value skippedIds(Json::arrayValue);
    for (const auto& id : skipped) {
      skippedIds.append(id);
    }
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(recorded.size());
    data[stream::KEY_SKIPPED_IDS] = skippedIds;
    response = http::Response::ok(data, stream::MSG_BATCH_RECORD_OK);
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchUnrecord(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  std::vector<std::string> names;
  std::vector<std::string> urls;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_UNRECORD_ERROR);
  std::string errorText;
  bool ready = false;
  try {
    for (const auto& id : ids) {
      const Json::Value current = service.get(id);
      names.push_back(current[stream::KEY_NAME].asString());
      urls.push_back(current[stream::KEY_URL].asString());
    }
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  bool done = ready;
  for (std::size_t index = 0; done && index < ids.size(); index += 1) {
    bool replaced = true;
    if (co_await MediaMtx::getPath(names[index])) {
      replaced = co_await MediaMtx::replacePath(names[index], urls[index], false);
    }
    done = replaced;
    if (done) {
      try {
        service.unrecord(ids[index]);
      } catch (const std::exception& error) {
        done = false;
        errorText = error.what();
      }
    }
  }
  if (done) {
    Json::Value data;
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(ids.size());
    response = http::Response::ok(data, stream::MSG_BATCH_UNRECORD_OK);
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchProbe(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  std::vector<std::string> urls;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_PROBE_ERROR);
  std::string errorText;
  bool ready = false;
  try {
    for (const auto& id : ids) {
      urls.push_back(service.get(id)[stream::KEY_URL].asString());
    }
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  Json::Value results(Json::arrayValue);
  bool accepted = ready;
  for (std::size_t index = 0; accepted && index < ids.size(); index += 1) {
    const ProbeResult probed = co_await Ffmpeg::probe(urls[index]);
    accepted = probed.accepted;
    if (accepted) {
      try {
        results.append(service.probe(ids[index],
                                     probed.data[stream::KEY_STATUS],
                                     probed.data[stream::KEY_WIDTH],
                                     probed.data[stream::KEY_HEIGHT],
                                     probed.data[stream::KEY_FPS],
                                     probed.data[stream::KEY_PROBED_AT]));
      } catch (const std::exception& error) {
        accepted = false;
        errorText = error.what();
      }
    }
  }
  if (accepted) {
    Json::Value data;
    data[stream::KEY_RESULTS] = results;
    response = http::Response::ok(data, stream::MSG_BATCH_PROBE_OK);
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}
