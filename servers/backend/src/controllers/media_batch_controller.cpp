#include "controllers/media_batch_controller.hpp"

#include "constants/media_const.hpp"
#include "db/database.hpp"
#include "response/response.hpp"

#include <drogon/MultiPart.h>
#include <zip.h>

#include <chrono>
#include <ctime>
#include <exception>
#include <set>
#include <stdexcept>

MediaBatchController::MediaBatchController() : service(Database::client()) {}

void MediaBatchController::batchCreate(const drogon::HttpRequestPtr& request,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value items(Json::arrayValue);
  const bool hasItems = body && body->isMember(media::KEY_ITEMS) && (*body)[media::KEY_ITEMS].isArray();
  if (hasItems) {
    items = (*body)[media::KEY_ITEMS];
  }
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    Json::Value created(Json::arrayValue);
    for (const auto& item : items) {
      created.append(service.create(item.get(media::KEY_TYPE, Json::Value()),
                                     item.get(media::KEY_NAME, Json::Value()),
                                     item.get(media::KEY_WIDTH, Json::Value()),
                                     item.get(media::KEY_HEIGHT, Json::Value()),
                                     item.get(media::KEY_DURATION, Json::Value()),
                                     item.get(media::KEY_FPS, Json::Value()),
                                     item.get(media::KEY_CODEC, Json::Value()),
                                     item.get(media::KEY_SIZE, Json::Value()),
                                     item.get(media::KEY_MD5, Json::Value()),
                                     item.get(media::KEY_FOLDER, Json::Value()),
                                     item.get(media::KEY_CREATED_AT, Json::Value()),
                                     item.get(media::KEY_UPDATED_AT, Json::Value()),
                                     item.get(media::KEY_USER_ID, Json::Value()),
                                     item.get(media::KEY_STREAM_ID, Json::Value()),
                                     item.get(media::KEY_PIPELINE_ID, Json::Value())));
    }
    Json::Value data;
    data[media::KEY_ITEMS] = created;
    response = http::Response::ok(data, media::MSG_BATCH_CREATE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_BATCH_CREATE_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaBatchController::batchUpload(const drogon::HttpRequestPtr& request,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    drogon::MultiPartParser parser;
    const bool parsed = parser.parse(request) == 0;
    if (!parsed) {
      throw std::runtime_error(media::MSG_BATCH_UPLOAD_ERROR);
    }
    const auto& fields = parser.getParameters();
    const std::string userId = fields.count(media::KEY_USER_ID) ? fields.at(media::KEY_USER_ID) : "";
    Json::Value created(Json::arrayValue);
    for (const auto& file : parser.getFiles()) {
      const std::string content(file.fileData(), file.fileLength());
      created.append(service.upload(userId, file.getFileName(), content));
    }
    Json::Value data;
    data[media::KEY_ITEMS] = created;
    response = http::Response::ok(data, media::MSG_BATCH_UPLOAD_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_BATCH_UPLOAD_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaBatchController::batchRemove(const drogon::HttpRequestPtr& request,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value ids(Json::arrayValue);
  const bool hasIds = body && body->isMember(media::KEY_IDS) && (*body)[media::KEY_IDS].isArray();
  if (hasIds) {
    ids = (*body)[media::KEY_IDS];
  }
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    Json::Value removed(Json::arrayValue);
    for (const auto& id : ids) {
      service.removeFile(id.asString());
      removed.append(service.remove(id.asString()));
    }
    Json::Value data;
    data[media::KEY_ITEMS] = removed;
    response = http::Response::ok(data, media::MSG_BATCH_REMOVE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_BATCH_REMOVE_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

std::filesystem::path MediaBatchController::zipPaths(
    const std::vector<std::pair<std::string, std::filesystem::path>>& files) {
  const std::filesystem::path archive = std::filesystem::temp_directory_path() /
      (std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".zip");
  int zipError = 0;
  zip_t* zip = zip_open(archive.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zipError);
  bool packed = zip != nullptr;
  std::set<std::string> used;
  for (const auto& file : files) {
    const std::string base = file.second.filename().string();
    const std::string entry = used.contains(base) ? file.first + "_" + base : base;
    used.insert(entry);
    zip_source_t* source = packed ? zip_source_file_create(file.second.c_str(), 0, -1, nullptr) : nullptr;
    const zip_int64_t added = source ? zip_file_add(zip, entry.c_str(), source, ZIP_FL_ENC_UTF_8) : -1;
    if (added < 0 && source) {
      zip_source_free(source);
    }
    packed = packed && added >= 0;
  }
  const int closed = zip && packed ? zip_close(zip) : -1;
  if (zip && (!packed || closed != 0)) {
    zip_discard(zip);
  }
  if (!packed || closed != 0) {
    std::filesystem::remove(archive);
    throw std::runtime_error(media::ERR_ZIP);
  }
  return archive;
}

void MediaBatchController::batchDownload(const drogon::HttpRequestPtr& request,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value ids(Json::arrayValue);
  const bool hasIds = body && body->isMember(media::KEY_IDS) && (*body)[media::KEY_IDS].isArray();
  if (hasIds) {
    ids = (*body)[media::KEY_IDS];
  }
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    std::vector<std::pair<std::string, std::filesystem::path>> files;
    for (const auto& id : ids) {
      const std::string mediaId = id.asString();
      files.emplace_back(mediaId, service.mediaId2Path(mediaId));
    }
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), media::DOWNLOAD_TIME_FORMAT, &local);
    const std::string name = std::string(media::DOWNLOAD_PREFIX) + stamp + media::DOWNLOAD_SUFFIX;
    response = drogon::HttpResponse::newFileResponse(zipPaths(files).string(), name);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_BATCH_DOWNLOAD_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}
