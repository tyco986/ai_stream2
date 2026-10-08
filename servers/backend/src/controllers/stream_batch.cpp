#include "controllers/stream_batch.hpp"

#include "db/database.hpp"
#include "ffmpeg/ffmpeg.hpp"
#include "mediamtx/mediamtx.hpp"
#include "response/response.hpp"
#include "log/base_logger.hpp"
#include "services/stream.hpp"

#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct BatchRemoveState {
  std::vector<std::string> names;
  std::vector<std::string> ids;
  std::function<void(const drogon::HttpResponsePtr&)> callback;
  std::size_t index;
};

struct BatchEnableState {
  std::vector<std::string> ids;
  std::vector<std::string> names;
  std::vector<std::string> urls;
  std::vector<bool> recordings;
  std::function<void(const drogon::HttpResponsePtr&)> callback;
  std::size_t index;
};

struct BatchDisableState {
  std::vector<std::string> ids;
  std::vector<std::string> names;
  std::vector<bool> enabled;
  std::function<void(const drogon::HttpResponsePtr&)> callback;
  std::size_t index;
};

struct BatchRecordState {
  std::vector<std::string> ids;
  std::vector<std::string> names;
  std::vector<std::string> urls;
  std::vector<std::string> skipped;
  std::function<void(const drogon::HttpResponsePtr&)> callback;
  std::size_t index;
};

struct BatchUnrecordState {
  std::vector<std::string> ids;
  std::vector<std::string> names;
  std::vector<std::string> urls;
  std::vector<bool> enabled;
  std::function<void(const drogon::HttpResponsePtr&)> callback;
  std::size_t index;
};

struct BatchProbeState {
  std::vector<std::string> ids;
  std::vector<std::string> urls;
  Json::Value results;
  std::function<void(const drogon::HttpResponsePtr&)> callback;
  std::size_t index;
};

void eraseRemoved(const std::shared_ptr<BatchRemoveState>& state) {
  try {
    StreamService service(Database::client());
    service.batchRemove(state->ids);
    Json::Value data;
    data[stream::KEY_REMOVED_COUNT] = static_cast<Json::UInt64>(state->ids.size());
    state->callback(http::Response::ok(data, stream::MSG_BATCH_DELETE_OK));
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DELETE_ERROR));
  }
}

void unmountNext(const std::shared_ptr<BatchRemoveState>& state) {
  if (state->index < state->names.size()) {
    MediaMtx::deletePath(state->names[state->index], [state](bool removed) {
      if (removed) {
        state->index += 1;
        unmountNext(state);
      } else {
        state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DELETE_ERROR));
      }
    });
  } else {
    eraseRemoved(state);
  }
}

void mountNext(const std::shared_ptr<BatchEnableState>& state) {
  if (state->index < state->ids.size()) {
    const std::string id = state->ids[state->index];
    const std::string name = state->names[state->index];
    const std::string url = state->urls[state->index];
    const bool recording = state->recordings[state->index];
    MediaMtx::replacePath(name, url, recording, [state, id](bool mounted) {
      if (!mounted) {
        state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_ENABLE_ERROR));
      } else {
        try {
          StreamService service(Database::client());
          service.enable(*Database::client(), id);
          state->index += 1;
          mountNext(state);
        } catch (const std::exception&) {
          state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_ENABLE_ERROR));
        }
      }
    });
  } else {
    Json::Value data;
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(state->ids.size());
    state->callback(http::Response::ok(data, stream::MSG_BATCH_ENABLE_OK));
  }
}

void disableCurrent(const std::shared_ptr<BatchDisableState>& state);

void disableNext(const std::shared_ptr<BatchDisableState>& state) {
  if (state->index < state->ids.size()) {
    disableCurrent(state);
  } else {
    Json::Value data;
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(state->ids.size());
    state->callback(http::Response::ok(data, stream::MSG_BATCH_DISABLE_OK));
  }
}

void writeDisabled(const std::shared_ptr<BatchDisableState>& state) {
  try {
    StreamService service(Database::client());
    service.disable(*Database::client(), state->ids[state->index]);
    state->index += 1;
    disableNext(state);
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DISABLE_ERROR));
  }
}

void disableCurrent(const std::shared_ptr<BatchDisableState>& state) {
  if (state->enabled[state->index]) {
    MediaMtx::deletePath(state->names[state->index], [state](bool removed) {
      if (removed) {
        writeDisabled(state);
      } else {
        state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DISABLE_ERROR));
      }
    });
  } else {
    writeDisabled(state);
  }
}

void recordNext(const std::shared_ptr<BatchRecordState>& state) {
  if (state->index < state->ids.size()) {
    const std::string id = state->ids[state->index];
    const std::string name = state->names[state->index];
    const std::string url = state->urls[state->index];
    MediaMtx::replacePath(name, url, true, [state, id](bool mounted) {
      if (!mounted) {
        state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_RECORD_ERROR));
      } else {
        try {
          StreamService service(Database::client());
          service.record(*Database::client(), id);
          state->index += 1;
          recordNext(state);
        } catch (const std::exception&) {
          state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_RECORD_ERROR));
        }
      }
    });
  } else {
    Json::Value data;
    Json::Value skipped(Json::arrayValue);
    for (const auto& id : state->skipped) {
      skipped.append(id);
    }
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(state->ids.size());
    data[stream::KEY_SKIPPED_IDS] = skipped;
    state->callback(http::Response::ok(data, stream::MSG_BATCH_RECORD_OK));
  }
}

void unrecordNext(const std::shared_ptr<BatchUnrecordState>& state) {
  if (state->index < state->ids.size()) {
    const std::string id = state->ids[state->index];
    if (state->enabled[state->index]) {
      const std::string name = state->names[state->index];
      const std::string url = state->urls[state->index];
      MediaMtx::replacePath(name, url, false, [state, id](bool mounted) {
        if (!mounted) {
          state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_UNRECORD_ERROR));
        } else {
          try {
            StreamService service(Database::client());
            service.unrecord(*Database::client(), id);
            state->index += 1;
            unrecordNext(state);
          } catch (const std::exception&) {
            state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_UNRECORD_ERROR));
          }
        }
      });
    } else {
      try {
        StreamService service(Database::client());
        service.unrecord(*Database::client(), id);
        state->index += 1;
        unrecordNext(state);
      } catch (const std::exception&) {
        state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_UNRECORD_ERROR));
      }
    }
  } else {
    Json::Value data;
    data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(state->ids.size());
    state->callback(http::Response::ok(data, stream::MSG_BATCH_UNRECORD_OK));
  }
}

void probeNext(const std::shared_ptr<BatchProbeState>& state) {
  if (state->index < state->ids.size()) {
    const std::string id = state->ids[state->index];
    Ffmpeg::probe(state->urls[state->index], [state, id](bool accepted, const Json::Value& result) {
      if (accepted) {
        try {
          StreamService service(Database::client());
          state->results.append(service.probe(id, result));
          state->index += 1;
          probeNext(state);
        } catch (const std::exception&) {
          state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_PROBE_ERROR));
        }
      } else {
        state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_PROBE_ERROR));
      }
    });
  } else {
    Json::Value data;
    data[stream::KEY_RESULTS] = state->results;
    state->callback(http::Response::ok(data, stream::MSG_BATCH_PROBE_OK));
  }
}

void StreamBatchController::respond(
    const drogon::HttpRequestPtr& request,
    const drogon::HttpResponsePtr& response,
    std::chrono::steady_clock::time_point started,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback) {
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
  logger.write(entry);
  callback(response);
}

void StreamBatchController::batchRemove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  auto state = std::make_shared<BatchRemoveState>();
  state->index = 0;
  state->callback = [this, request, started, callback = std::move(callback)](
                        const drogon::HttpResponsePtr& response) {
    respond(request, response, started, callback);
  };
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      state->ids.push_back(node.asString());
    }
  }
  try {
    StreamService service(Database::client());
    for (const auto& id : state->ids) {
      const Json::Value current = service.get(id);
      if (current[stream::KEY_ENABLED].asBool()) {
        state->names.push_back(current[stream::KEY_NAME].asString());
      }
    }
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DELETE_ERROR));
    return;
  }
  unmountNext(state);
}

void StreamBatchController::batchEnable(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  auto state = std::make_shared<BatchEnableState>();
  state->index = 0;
  state->callback = [this, request, started, callback = std::move(callback)](
                        const drogon::HttpResponsePtr& response) {
    respond(request, response, started, callback);
  };
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      state->ids.push_back(node.asString());
    }
  }
  try {
    StreamService service(Database::client());
    for (const auto& id : state->ids) {
      const Json::Value current = service.get(id);
      state->names.push_back(current[stream::KEY_NAME].asString());
      state->urls.push_back(current[stream::KEY_URL].asString());
      state->recordings.push_back(current[stream::KEY_RECORDING].asBool());
    }
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_ENABLE_ERROR));
    return;
  }
  mountNext(state);
}

void StreamBatchController::batchDisable(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  auto state = std::make_shared<BatchDisableState>();
  state->index = 0;
  state->callback = [this, request, started, callback = std::move(callback)](
                        const drogon::HttpResponsePtr& response) {
    respond(request, response, started, callback);
  };
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      state->ids.push_back(node.asString());
    }
  }
  try {
    StreamService service(Database::client());
    for (const auto& id : state->ids) {
      const Json::Value current = service.get(id);
      state->names.push_back(current[stream::KEY_NAME].asString());
      state->enabled.push_back(current[stream::KEY_ENABLED].asBool());
    }
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_DISABLE_ERROR));
    return;
  }
  disableNext(state);
}

void StreamBatchController::batchRecord(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  auto state = std::make_shared<BatchRecordState>();
  state->index = 0;
  state->callback = [this, request, started, callback = std::move(callback)](
                        const drogon::HttpResponsePtr& response) {
    respond(request, response, started, callback);
  };
  std::vector<std::string> ids;
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      ids.push_back(node.asString());
    }
  }
  try {
    StreamService service(Database::client());
    for (const auto& id : ids) {
      const Json::Value current = service.get(id);
      if (current[stream::KEY_ENABLED].asBool()) {
        state->ids.push_back(id);
        state->names.push_back(current[stream::KEY_NAME].asString());
        state->urls.push_back(current[stream::KEY_URL].asString());
      } else {
        state->skipped.push_back(id);
      }
    }
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_RECORD_ERROR));
    return;
  }
  recordNext(state);
}

void StreamBatchController::batchUnrecord(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  auto state = std::make_shared<BatchUnrecordState>();
  state->index = 0;
  state->callback = [this, request, started, callback = std::move(callback)](
                        const drogon::HttpResponsePtr& response) {
    respond(request, response, started, callback);
  };
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      state->ids.push_back(node.asString());
    }
  }
  try {
    StreamService service(Database::client());
    for (const auto& id : state->ids) {
      const Json::Value current = service.get(id);
      state->names.push_back(current[stream::KEY_NAME].asString());
      state->urls.push_back(current[stream::KEY_URL].asString());
      state->enabled.push_back(current[stream::KEY_ENABLED].asBool());
    }
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_UNRECORD_ERROR));
    return;
  }
  unrecordNext(state);
}

void StreamBatchController::batchProbe(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  auto state = std::make_shared<BatchProbeState>();
  state->index = 0;
  state->results = Json::Value(Json::arrayValue);
  state->callback = [this, request, started, callback = std::move(callback)](
                        const drogon::HttpResponsePtr& response) {
    respond(request, response, started, callback);
  };
  if (body && (*body).isMember(stream::KEY_IDS) && (*body)[stream::KEY_IDS].isArray()) {
    for (const auto& node : (*body)[stream::KEY_IDS]) {
      state->ids.push_back(node.asString());
    }
  }
  try {
    StreamService service(Database::client());
    for (const auto& id : state->ids) {
      state->urls.push_back(service.get(id)[stream::KEY_URL].asString());
    }
  } catch (const std::exception&) {
    state->callback(http::Response::error(drogon::k500InternalServerError, stream::MSG_BATCH_PROBE_ERROR));
    return;
  }
  probeNext(state);
}
