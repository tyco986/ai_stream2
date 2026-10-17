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
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = stream::BATCH_DATA();
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
      const std::string name = current[stream::KEY_NAME].asString();
      const bool mounted = co_await MediaMtx::getPath(name);
      bool pathRemoved = true;
      if (mounted) {
        pathRemoved = co_await MediaMtx::deletePath(name);
      }
      if (pathRemoved) {
        try {
          removedRow = service.remove(id);
          removed = true;
        } catch (const std::exception&) {
          removed = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    const Json::Value row = removed ? removedRow : failedRow;
    const char* key = removed ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_DELETE_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchEnable(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = stream::BATCH_DATA();
  for (const auto& id : ids) {
    Json::Value current;
    bool found = false;
    try {
      current = service.get(id);
      found = true;
    } catch (const std::exception&) {
      found = false;
    }
    bool enabled = false;
    if (found) {
      const bool mounted = co_await MediaMtx::replacePath(current[stream::KEY_NAME].asString(),
                                                           current[stream::KEY_URL].asString(),
                                                           current[stream::KEY_RECORD].asBool());
      if (mounted) {
        try {
          service.setEnabled(id, true);
          enabled = true;
        } catch (const std::exception&) {
          enabled = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    Json::Value updated = current;
    updated[stream::KEY_ENABLED] = true;
    const Json::Value row = enabled ? updated : failedRow;
    const char* key = enabled ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_ENABLE_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchDisable(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = stream::BATCH_DATA();
  for (const auto& id : ids) {
    Json::Value current;
    bool found = false;
    try {
      current = service.get(id);
      found = true;
    } catch (const std::exception&) {
      found = false;
    }
    bool disabled = false;
    if (found) {
      const std::string name = current[stream::KEY_NAME].asString();
      const bool mounted = co_await MediaMtx::getPath(name);
      bool removed = true;
      if (mounted) {
        removed = co_await MediaMtx::deletePath(name);
      }
      if (removed) {
        try {
          service.setEnabled(id, false);
          disabled = true;
        } catch (const std::exception&) {
          disabled = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    Json::Value updated = current;
    updated[stream::KEY_ENABLED] = false;
    const Json::Value row = disabled ? updated : failedRow;
    const char* key = disabled ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_DISABLE_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchRecord(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = stream::BATCH_DATA();
  for (const auto& id : ids) {
    Json::Value current;
    bool found = false;
    try {
      current = service.get(id);
      found = true;
    } catch (const std::exception&) {
      found = false;
    }
    bool mounted = false;
    bool replaced = false;
    bool recorded = false;
    if (found) {
      mounted = co_await MediaMtx::getPath(current[stream::KEY_NAME].asString());
      if (mounted) {
        replaced = co_await MediaMtx::replacePath(current[stream::KEY_NAME].asString(),
                                                   current[stream::KEY_URL].asString(),
                                                   true);
      }
      if (replaced) {
        try {
          service.setRecord(id, true);
          recorded = true;
        } catch (const std::exception&) {
          recorded = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    Json::Value updated = current;
    updated[stream::KEY_RECORD] = true;
    const bool skipped = found && !mounted;
    const char* key = recorded ? stream::KEY_UPDATED_ROWS
                               : skipped ? stream::KEY_SKIPPED_ROWS : stream::KEY_FAILED_ROWS;
    const Json::Value row = recorded ? updated : skipped ? current : found ? current : failed;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_RECORD_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchUnrecord(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = stream::BATCH_DATA();
  for (const auto& id : ids) {
    Json::Value current;
    bool found = false;
    try {
      current = service.get(id);
      found = true;
    } catch (const std::exception&) {
      found = false;
    }
    bool unrecorded = false;
    if (found) {
      const std::string name = current[stream::KEY_NAME].asString();
      const bool mounted = co_await MediaMtx::getPath(name);
      bool replaced = true;
      if (mounted) {
        replaced = co_await MediaMtx::replacePath(name, current[stream::KEY_URL].asString(), false);
      }
      if (replaced) {
        try {
          service.setRecord(id, false);
          unrecorded = true;
        } catch (const std::exception&) {
          unrecorded = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    Json::Value updated = current;
    updated[stream::KEY_RECORD] = false;
    const Json::Value row = unrecorded ? updated : failedRow;
    const char* key = unrecorded ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_UNRECORD_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchProbe(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::vector<std::string> ids;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  Json::Value data = stream::BATCH_DATA();
  for (const auto& id : ids) {
    Json::Value current;
    bool found = false;
    try {
      current = service.get(id);
      found = true;
    } catch (const std::exception&) {
      found = false;
    }
    Json::Value probedRow;
    bool probed = false;
    if (found) {
      const ProbeResult result = co_await Ffmpeg::probe(current[stream::KEY_URL].asString());
      if (result.accepted) {
        try {
          probedRow = service.probe(id,
                                    result.data[stream::KEY_STATUS],
                                    result.data[stream::KEY_WIDTH],
                                    result.data[stream::KEY_HEIGHT],
                                    result.data[stream::KEY_FPS],
                                    result.data[stream::KEY_PROBED_AT]);
          probed = true;
        } catch (const std::exception&) {
          probed = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    const Json::Value row = probed ? probedRow : failedRow;
    const char* key = probed ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_PROBE_OK);
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}
