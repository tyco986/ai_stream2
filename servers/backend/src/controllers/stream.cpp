#include "controllers/stream.hpp"

#include "constants/stream.hpp"
#include "constants/stream_group.hpp"
#include "db/database.hpp"
#include "ffmpeg/ffmpeg.hpp"
#include "mediamtx/mediamtx.hpp"
#include "response/response.hpp"
#include "log/base_logger.hpp"
#include "services/stream.hpp"

#include <chrono>
#include <exception>

void StreamController::respond(
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

void StreamController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(stream::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(stream::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? stream::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? stream::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    StreamService service(Database::client());
    respond(request,
            http::Response::ok(
                service.list(request->getParameter(stream::QUERY_GROUP_ID),
                             request->getParameter(stream::QUERY_STREAM_ID),
                             request->getParameter(stream::QUERY_SEARCH),
                             page,
                             pageSize),
                stream::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream::MSG_LIST_ERROR),
            started,
            callback);
  }
}

void StreamController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto body = request->getJsonObject();
  const std::string name = body ? (*body)[stream::KEY_NAME].asString() : "";
  const std::string url = body ? (*body)[stream::KEY_URL].asString() : "";
  const std::string groupId = body && (*body).isMember(stream::KEY_GROUP_ID)
      ? (*body)[stream::KEY_GROUP_ID].asString()
      : stream_group::ALL_GROUP_ID;
  const bool enabled = !body || !(*body).isMember(stream::KEY_ENABLED) || (*body)[stream::KEY_ENABLED].asBool();
  const bool recording = body && (*body).isMember(stream::KEY_RECORDING) && (*body)[stream::KEY_RECORDING].asBool();
  const auto started = std::chrono::steady_clock::now();
  const auto save = [this, request, started, name, groupId, url, enabled, recording, callback]() {
    try {
      StreamService service(Database::client());
      const Json::Value data =
          service.create(*Database::client(), name, groupId, url, enabled, recording);
      respond(request, http::Response::ok(data, stream::MSG_CREATE_OK), started, callback);
    } catch (const std::exception&) {
      respond(request,
              http::Response::error(drogon::k500InternalServerError, stream::MSG_CREATE_ERROR),
              started,
              callback);
    }
  };
  if (enabled) {
    MediaMtx::replacePath(name, url, recording, [this, request, started, save, callback](bool mounted) {
      if (mounted) {
        save();
      } else {
        respond(request,
                http::Response::error(drogon::k500InternalServerError, stream::MSG_CREATE_ERROR),
                started,
                callback);
      }
    });
  } else {
    save();
  }
}

void StreamController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  std::string oldName;
  std::string name;
  std::string groupId;
  std::string url;
  bool enabled = false;
  bool recording = false;
  bool mount = false;
  bool unmount = false;
  try {
    StreamService service(Database::client());
    const Json::Value current = service.get(stream_id);
    oldName = current[stream::KEY_NAME].asString();
    const bool wasEnabled = current[stream::KEY_ENABLED].asBool();
    name = body && (*body).isMember(stream::KEY_NAME) ? (*body)[stream::KEY_NAME].asString() : oldName;
    groupId = body && (*body).isMember(stream::KEY_GROUP_ID) ? (*body)[stream::KEY_GROUP_ID].asString()
                                                             : current[stream::KEY_GROUP_ID].asString();
    url = body && (*body).isMember(stream::KEY_URL) ? (*body)[stream::KEY_URL].asString()
                                                    : current[stream::KEY_URL].asString();
    enabled = body && (*body).isMember(stream::KEY_ENABLED) ? (*body)[stream::KEY_ENABLED].asBool() : wasEnabled;
    recording = body && (*body).isMember(stream::KEY_RECORDING) ? (*body)[stream::KEY_RECORDING].asBool()
                                                                : current[stream::KEY_RECORDING].asBool();
    const bool rename = name != oldName;
    const bool pathChanged = rename || url != current[stream::KEY_URL].asString() ||
        recording != current[stream::KEY_RECORDING].asBool();
    mount = enabled && (!wasEnabled || pathChanged);
    unmount = wasEnabled && (!enabled || rename);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream::MSG_UPDATE_ERROR),
            started,
            callback);
    return;
  }
  const auto save = [this, request, started, stream_id, name, groupId, url, enabled, recording, callback]() {
    try {
      StreamService service(Database::client());
      const Json::Value data =
          service.update(*Database::client(), stream_id, name, groupId, url, enabled, recording);
      respond(request, http::Response::ok(data, stream::MSG_UPDATE_OK), started, callback);
    } catch (const std::exception&) {
      respond(request,
              http::Response::error(drogon::k500InternalServerError, stream::MSG_UPDATE_ERROR),
              started,
              callback);
    }
  };
  if (unmount && mount) {
    MediaMtx::deletePath(oldName, [this, request, started, name, url, recording, save, callback](bool removed) {
      if (!removed) {
        respond(request,
                http::Response::error(drogon::k500InternalServerError, stream::MSG_UPDATE_ERROR),
                started,
                callback);
      } else {
        MediaMtx::replacePath(name, url, recording, [this, request, started, save, callback](bool mounted) {
          if (mounted) {
            save();
          } else {
            respond(request,
                    http::Response::error(drogon::k500InternalServerError, stream::MSG_UPDATE_ERROR),
                    started,
                    callback);
          }
        });
      }
    });
  } else if (unmount) {
    MediaMtx::deletePath(oldName, [this, request, started, save, callback](bool removed) {
      if (removed) {
        save();
      } else {
        respond(request,
                http::Response::error(drogon::k500InternalServerError, stream::MSG_UPDATE_ERROR),
                started,
                callback);
      }
    });
  } else if (mount) {
    MediaMtx::replacePath(name, url, recording, [this, request, started, save, callback](bool mounted) {
      if (mounted) {
        save();
      } else {
        respond(request,
                http::Response::error(drogon::k500InternalServerError, stream::MSG_UPDATE_ERROR),
                started,
                callback);
      }
    });
  } else {
    save();
  }
}

void StreamController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  std::string name;
  bool enabled = false;
  try {
    StreamService service(Database::client());
    const Json::Value current = service.get(stream_id);
    name = current[stream::KEY_NAME].asString();
    enabled = current[stream::KEY_ENABLED].asBool();
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream::MSG_DELETE_ERROR),
            started,
            callback);
    return;
  }
  const auto erase = [this, request, started, stream_id, callback]() {
    try {
      StreamService service(Database::client());
      service.remove(stream_id);
      respond(request,
              http::Response::ok(Json::Value(Json::nullValue), stream::MSG_DELETE_OK),
              started,
              callback);
    } catch (const std::exception&) {
      respond(request,
              http::Response::error(drogon::k500InternalServerError, stream::MSG_DELETE_ERROR),
              started,
              callback);
    }
  };
  if (enabled) {
    MediaMtx::deletePath(name, [this, request, started, erase, callback](bool removed) {
      if (!removed) {
        respond(request,
                http::Response::error(drogon::k500InternalServerError, stream::MSG_DELETE_ERROR),
                started,
                callback);
      } else {
        erase();
      }
    });
  } else {
    erase();
  }
}

void StreamController::probe(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  std::string url;
  try {
    StreamService service(Database::client());
    url = service.get(stream_id)[stream::KEY_URL].asString();
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream::MSG_PROBE_ERROR),
            started,
            callback);
    return;
  }
  Ffmpeg::probe(url, [this, request, started, stream_id, callback = std::move(callback)](bool accepted, const Json::Value& result) {
    if (accepted) {
      try {
        StreamService service(Database::client());
        respond(request,
                http::Response::ok(service.probe(stream_id, result), stream::MSG_PROBE_OK),
                started,
                callback);
      } catch (const std::exception&) {
        respond(request,
                http::Response::error(drogon::k500InternalServerError, stream::MSG_PROBE_ERROR),
                started,
                callback);
      }
    } else {
      respond(request,
              http::Response::error(drogon::k500InternalServerError, stream::MSG_PROBE_ERROR),
              started,
              callback);
    }
  });
}

void StreamController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    StreamService service(Database::client());
    respond(request, http::Response::ok(service.get(stream_id), stream::MSG_GET_OK), started, callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream::MSG_GET_ERROR),
            started,
            callback);
  }
}
