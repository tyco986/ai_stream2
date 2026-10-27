#include "controllers/stream_batch_controller.hpp"

#include "db/database.hpp"
#include "ffmpeg/ffmpeg.hpp"
#include "mediamtx/mediamtx.hpp"
#include "response/response.hpp"
#include "logger/base_logger.hpp"

#include <chrono>
#include <exception>
#include <functional>
#include <string>
#include <vector>

StreamBatchController::StreamBatchController() : service(Database::client()) {}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchCreate(drogon::HttpRequestPtr request) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value items(Json::arrayValue);
  if (body && (*body).isMember(stream::KEY_ITEMS) && (*body)[stream::KEY_ITEMS].isArray()) {
    items = (*body)[stream::KEY_ITEMS];
  }
  Json::Value data = stream::BATCH_DATA();
  int outcome = 0;
  for (const auto& item : items) {
    const Json::Value name = item.get(stream::KEY_NAME, Json::Value());
    const Json::Value groupId = item.get(stream::KEY_GROUP_ID, Json::Value());
    const Json::Value url = item.get(stream::KEY_URL, Json::Value());
    const Json::Value mount = item.get(stream::KEY_MOUNT, Json::Value());
    const Json::Value recording = item.get(stream::KEY_RECORD, Json::Value());
    const Json::Value status = item.get(stream::KEY_STATUS, Json::Value());
    const Json::Value width = item.get(stream::KEY_WIDTH, Json::Value());
    const Json::Value height = item.get(stream::KEY_HEIGHT, Json::Value());
    const Json::Value fps = item.get(stream::KEY_FPS, Json::Value());
    const Json::Value probedAt = item.get(stream::KEY_PROBED_AT, Json::Value());
    const bool wantMount = mount.isNull() || mount.asBool();
    const std::string pathName = name.isString() ? name.asString() : std::string();
    const std::string pathSource = url.isString() ? url.asString() : std::string();
    const bool recordFlag = recording.isBool() && recording.asBool();
    PathReplace pathResult = PathReplace::mounted;
    if (wantMount) {
      pathResult = co_await MediaMtx::replacePath(pathName, pathSource, recordFlag);
    }
    const int rankByResult[] = {0, 2, 1};
    const bool mounted = pathResult == PathReplace::mounted;
    const Json::Value storedMount = mounted ? mount : Json::Value(false);
    Json::Value createdRow;
    bool created = false;
    try {
      createdRow = service.create(name, groupId, url, storedMount, recording, status, width, height, fps, probedAt);
      created = true;
    } catch (const std::exception&) {
      created = false;
    }
    const int rowOutcome = created ? rankByResult[static_cast<int>(pathResult)] : 0;
    outcome = rowOutcome > outcome ? rowOutcome : outcome;
    const Json::Value row = created ? createdRow : item;
    const char* key = created ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const char* messages[] = {
      stream::MSG_BATCH_CREATE_OK,
      stream::MSG_BATCH_CREATE_REJECTED,
      stream::MSG_BATCH_CREATE_PARTIAL};
  const char* message = messages[outcome];
  const drogon::HttpResponsePtr response = outcome == 0 ? http::Response::ok(data, message) : http::Response::partial(data, message);
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchRemove(drogon::HttpRequestPtr request) {
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
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchMount(drogon::HttpRequestPtr request) {
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
    if (found) {
      const bool pathMounted = co_await MediaMtx::replacePath(current[stream::KEY_NAME].asString(),
                                                               current[stream::KEY_URL].asString(),
                                                               current[stream::KEY_RECORD].asBool()) == PathReplace::mounted;
      if (pathMounted) {
        try {
          service.setMount(id, true);
          mounted = true;
        } catch (const std::exception&) {
          mounted = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    Json::Value updated = current;
    updated[stream::KEY_MOUNT] = true;
    const Json::Value row = mounted ? updated : failedRow;
    const char* key = mounted ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_MOUNT_OK);
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchUnmount(drogon::HttpRequestPtr request) {
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
    bool unmounted = false;
    if (found) {
      const std::string name = current[stream::KEY_NAME].asString();
      const bool mounted = co_await MediaMtx::getPath(name);
      bool removed = true;
      if (mounted) {
        removed = co_await MediaMtx::deletePath(name);
      }
      if (removed) {
        try {
          service.setMount(id, false);
          service.setRecord(id, false);
          unmounted = true;
        } catch (const std::exception&) {
          unmounted = false;
        }
      }
    }
    Json::Value failed;
    failed[stream::KEY_ID] = id;
    const Json::Value failedRow = found ? current : failed;
    Json::Value updated = current;
    updated[stream::KEY_MOUNT] = false;
    updated[stream::KEY_RECORD] = false;
    const Json::Value row = unmounted ? updated : failedRow;
    const char* key = unmounted ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_UNMOUNT_OK);
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchRecord(drogon::HttpRequestPtr request) {
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
                                                   true) == PathReplace::mounted;
      }
      if (replaced) {
        try {
          service.setRecord(id, true);
          service.setMount(id, true);
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
    updated[stream::KEY_MOUNT] = true;
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
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchUnrecord(drogon::HttpRequestPtr request) {
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
    bool unrecorded = false;
    if (found) {
      const std::string name = current[stream::KEY_NAME].asString();
      mounted = co_await MediaMtx::getPath(name);
      bool replaced = true;
      if (mounted) {
        replaced = co_await MediaMtx::replacePath(name, current[stream::KEY_URL].asString(), false) == PathReplace::mounted;
      }
      if (replaced) {
        try {
          service.setRecord(id, false);
          service.setMount(id, mounted);
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
    updated[stream::KEY_MOUNT] = mounted;
    const Json::Value row = unrecorded ? updated : failedRow;
    const char* key = unrecorded ? stream::KEY_UPDATED_ROWS : stream::KEY_FAILED_ROWS;
    data[key].append(row);
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  const drogon::HttpResponsePtr response = http::Response::ok(data, stream::MSG_BATCH_UNRECORD_OK);
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamBatchController::batchProbe(drogon::HttpRequestPtr request) {
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
      const ProbeResult result = co_await Ffmpeg::rtspProbe(current[stream::KEY_URL].asString());
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
  logger.respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, "");
  co_return response;
}
