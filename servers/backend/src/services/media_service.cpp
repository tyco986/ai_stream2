#include "services/media_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/media_const.hpp"
#include "searcher/searcher.hpp"

#include <MediaInfoDLL/MediaInfoDLL.h>
#include <openssl/evp.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace {

const char* SELECT_MEDIA =
    "id::text AS id, type, name, width, height, duration, fps, codec, size, md5, folder, "
    "to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"') AS created_at, "
    "to_char(updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"') AS updated_at, "
    "user_id::text AS user_id, stream_id::text AS stream_id, pipeline_id::text AS pipeline_id ";

class MediaInfoBuffer {
 public:
  MediaInfoBuffer() : handle(nullptr) {
    const bool loaded = MediaInfoDLL_Load() == 1;
    handle = loaded ? MediaInfo_New() : nullptr;
  }
  ~MediaInfoBuffer() {
    if (handle) {
      MediaInfo_Delete(handle);
    }
  }
  void* require() {
    if (!handle) {
      throw std::runtime_error(media::ERR_MEDIAINFO);
    }
    return handle;
  }
  void* handle;
};

std::string digestHex(const unsigned char* digest, unsigned int length) {
  std::string hex(static_cast<std::size_t>(length) * 2, '0');
  for (unsigned int index = 0; index < length; ++index) {
    hex[index * 2] = "0123456789abcdef"[digest[index] >> 4];
    hex[index * 2 + 1] = "0123456789abcdef"[digest[index] & 0x0f];
  }
  return hex;
}

}

MediaService::MediaService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)), root(media::MEDIA_DIR) {}

Json::Value MediaService::listUploadFile(int page,
                                     long pageSize,
                                     const std::string& userId,
                                     const std::string& type,
                                     const std::string& search) {
  const Json::Value data = listFile(media::UPLOAD_DIR, "user_id", userId, type, page, pageSize, search);
  return data;
}

Json::Value MediaService::listPipelineFile(int page,
                                       long pageSize,
                                       const std::string& type,
                                       const std::string& search,
                                       const std::string& pipelineId) {
  const Json::Value data = listFile(media::PIPELINE_DIR, "pipeline_id", pipelineId, type, page, pageSize, search);
  return data;
}

Json::Value MediaService::listRecordingFile(int page,
                                        long pageSize,
                                        const std::string& type,
                                        const std::string& search,
                                        const std::string& streamId) {
  const Json::Value data = listFile(media::RECORDING_DIR, "stream_id", streamId, type, page, pageSize, search);
  return data;
}

Json::Value MediaService::listUploadFolder() {
  const Json::Value names = listFolder(media::UPLOAD_DIR, "SELECT username AS name, id::text AS id FROM \"user\"");
  return names;
}

Json::Value MediaService::listPipelineFolder() {
  const Json::Value names = listFolder(media::PIPELINE_DIR, "SELECT name, id::text AS id FROM pipeline");
  return names;
}

Json::Value MediaService::listRecordingFolder() {
  const Json::Value names = listFolder(media::RECORDING_DIR, "SELECT name, id::text AS id FROM stream");
  return names;
}

Json::Value MediaService::listFolder(const std::string& folder, const std::string& sql) {
  const auto rows = Sql::sync({}, dbClient, sql);
  std::map<std::string, std::string> ids;
  for (const auto& row : rows) {
    ids[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  const std::filesystem::path scan = std::filesystem::path(root) / folder;
  Json::Value names(Json::objectValue);
  const bool rooted = std::filesystem::is_directory(scan);
  const std::filesystem::directory_iterator end;
  std::filesystem::directory_iterator entry = rooted
      ? std::filesystem::directory_iterator(scan, std::filesystem::directory_options::skip_permission_denied)
      : end;
  for (; entry != end; ++entry) {
    const std::string name = entry->path().filename().string();
    if (entry->is_directory() && ids.count(name) != 0) {
      names[name] = ids[name];
    }
  }
  return names;
}

void MediaService::reloadFile(const std::string& folder, const std::string& ownerColumn, const std::string& ownerId) {
  const std::filesystem::path prefix = (std::filesystem::path(root) / folder).lexically_normal();
  const std::string prefixText = prefix.string();
  const bool upload = folder == media::UPLOAD_DIR;
  const bool pipeline = folder == media::PIPELINE_DIR;
  const bool recording = folder == media::RECORDING_DIR;
  const std::string ownerName = ownerId.empty()
      ? ""
      : (upload ? userId2Name(ownerId) : (pipeline ? pipelineId2Name(ownerId) : streamId2Name(ownerId)));
  const std::filesystem::path scan = ownerName.empty() ? prefix : prefix / ownerName;
  const auto users = Sql::sync({}, dbClient, "SELECT username, id::text AS id FROM \"user\"");
  const auto pipelines = Sql::sync({}, dbClient, "SELECT name, id::text AS id FROM pipeline");
  const auto streams = Sql::sync({}, dbClient, "SELECT name, id::text AS id FROM stream");
  std::map<std::string, std::string> userIds;
  std::map<std::string, std::string> pipelineIds;
  std::map<std::string, std::string> streamIds;
  std::string adminId;
  for (const auto& row : users) {
    userIds[row["username"].as<std::string>()] = row["id"].as<std::string>();
    adminId = row["username"].as<std::string>() == media::ADMIN_USERNAME ? row["id"].as<std::string>() : adminId;
  }
  if ((pipeline || recording) && adminId.empty()) {
    throw std::runtime_error(media::ERR_ADMIN);
  }
  for (const auto& row : pipelines) {
    pipelineIds[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  for (const auto& row : streams) {
    streamIds[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  const auto stored = Sql::sync({}, dbClient,
      std::string("SELECT ") + SELECT_MEDIA + ", EXTRACT(EPOCH FROM updated_at)::bigint AS " + media::UPDATED_SEC +
          " FROM media WHERE (folder = $1 OR folder LIKE $1 || '/%') AND (" + ownerColumn + "::text = $2 OR $2 = '')",
      prefixText,
      ownerId);
  std::map<std::string, Json::Value> rowsByKey;
  for (const auto& row : stored) {
    Json::Value data = mediaRow(row);
    data[media::UPDATED_SEC] = static_cast<Json::Int64>(row[media::UPDATED_SEC].as<long>());
    const std::string key = std::filesystem::path(data[media::KEY_FOLDER].asString()).lexically_normal().string() + "\n" +
        std::filesystem::path(data[media::KEY_NAME].asString()).filename().string();
    const auto found = rowsByKey.find(key);
    const bool occupied = found != rowsByKey.end();
    if (!occupied) {
      rowsByKey.emplace(key, data);
    }
    if (occupied) {
      const bool newer = data[media::UPDATED_SEC].asInt64() > found->second[media::UPDATED_SEC].asInt64();
      const std::string dropId = newer ? found->second[media::KEY_ID].asString() : data[media::KEY_ID].asString();
      remove(dropId);
      if (newer) {
        found->second = data;
      }
    }
  }
  std::unordered_set<std::string> seen;
  const bool rooted = std::filesystem::is_directory(scan);
  const std::filesystem::recursive_directory_iterator end;
  std::filesystem::recursive_directory_iterator entry = rooted
      ? std::filesystem::recursive_directory_iterator(scan, std::filesystem::directory_options::skip_permission_denied)
      : end;
  for (; entry != end; ++entry) {
    const std::filesystem::path path = entry->path();
    const bool file = entry->is_regular_file();
    const std::string extension = lowerExtension(path.filename().string());
    const bool image = file && suffixMatched(extension, media::IMAGE_SUFFIXES);
    const bool video = file && suffixMatched(extension, media::VIDEO_SUFFIXES);
    struct statx status;
    const int stated = file ? statx(AT_FDCWD, path.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BTIME | STATX_MTIME | STATX_SIZE, &status) : -1;
    const std::time_t now = std::time(nullptr);
    const bool writing = recording && stated == 0 && now >= status.stx_mtime.tv_sec &&
        now - static_cast<std::time_t>(status.stx_mtime.tv_sec) < media::RECORDING_SETTLE_SECONDS;
    const std::filesystem::path parent = path.parent_path().lexically_normal();
    const std::string key = parent.string() + "\n" + path.filename().string();
    const bool usable = file && stated == 0 && (image || video) && !writing;
    if (usable || writing) {
      seen.insert(key);
    }
    const Json::Value* current = usable && rowsByKey.count(key) != 0 ? &rowsByKey[key] : nullptr;
    const bool unchanged = current != nullptr && !(*current)[media::KEY_SIZE].isNull() &&
        (*current)[media::KEY_SIZE].asInt64() == static_cast<Json::Int64>(status.stx_size) &&
        (*current)[media::UPDATED_SEC].asInt64() == static_cast<Json::Int64>(status.stx_mtime.tv_sec);
    std::string md5;
    const bool hashed = usable && current != nullptr && !unchanged;
    if (hashed || (usable && current == nullptr)) {
      std::ifstream input(path, std::ios::binary);
      EVP_MD_CTX* ctx = input ? EVP_MD_CTX_new() : nullptr;
      const int inited = ctx == nullptr ? 0 : EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
      std::vector<char> chunk(media::MD5_CHUNK_SIZE);
      int updated = inited;
      while (updated == 1 && input) {
        input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        const std::streamsize got = input.gcount();
        updated = got > 0 ? EVP_DigestUpdate(ctx, chunk.data(), static_cast<std::size_t>(got)) : updated;
      }
      unsigned char digest[EVP_MAX_MD_SIZE];
      unsigned int length = 0;
      const int finalized = updated == 1 ? EVP_DigestFinal_ex(ctx, digest, &length) : 0;
      EVP_MD_CTX_free(ctx);
      if (inited != 1 || updated != 1 || finalized != 1 || input.bad()) {
        throw std::runtime_error(media::ERR_MD5);
      }
      md5 = digestHex(digest, length);
    }
    const bool sameMd5 = hashed && md5 == (*current)[media::KEY_MD5].asString();
    const bool changed = hashed && !sameMd5;
    const bool created = usable && current == nullptr;
    const std::filesystem::path relative = parent.lexically_relative(prefix);
    const std::string ownerDir = relative.empty() || relative == "." ? "" : relative.begin()->string();
    const auto userFound = userIds.find(ownerDir);
    const auto pipelineFound = pipelineIds.find(ownerDir);
    const auto streamFound = streamIds.find(ownerDir);
    const std::string userId = upload ? (ownerId.empty() ? (userFound == userIds.end() ? "" : userFound->second) : ownerId) : adminId;
    const std::string pipelineId = pipeline ? (ownerId.empty() ? (pipelineFound == pipelineIds.end() ? "" : pipelineFound->second) : ownerId) : "";
    const std::string streamId = recording ? (ownerId.empty() ? (streamFound == streamIds.end() ? "" : streamFound->second) : ownerId) : "";
    const bool owned = !userId.empty() && (!pipeline || !pipelineId.empty()) && (!recording || !streamId.empty());
    std::tm utc{};
    const std::time_t mtime = stated == 0 ? static_cast<std::time_t>(status.stx_mtime.tv_sec) : 0;
    const std::time_t born = stated == 0 && (status.stx_mask & STATX_BTIME) != 0
        ? static_cast<std::time_t>(status.stx_btime.tv_sec)
        : mtime;
    gmtime_r(&mtime, &utc);
    char updatedBuffer[40];
    std::strftime(updatedBuffer, sizeof(updatedBuffer), media::TIME_UTC_FORMAT, &utc);
    gmtime_r(&born, &utc);
    char createdBuffer[40];
    std::strftime(createdBuffer, sizeof(createdBuffer), media::TIME_UTC_FORMAT, &utc);
    std::optional<int> width;
    std::optional<int> height;
    std::optional<double> duration;
    std::optional<double> fps;
    std::optional<std::string> codec;
    if ((created && owned) || changed) {
      probeFile(path, image, width, height, duration, fps, codec);
    }
    const std::string type = image ? media::TYPE_IMAGE : media::TYPE_VIDEO;
    if (created && owned) {
      create(type,
             path.filename().string(),
             width ? Json::Value(*width) : Json::Value(),
             height ? Json::Value(*height) : Json::Value(),
             image || !duration ? Json::Value() : Json::Value(*duration),
             image ? Json::Value() : (fps ? Json::Value(*fps) : Json::Value()),
             image ? Json::Value() : (codec ? Json::Value(*codec) : Json::Value()),
             Json::Value(static_cast<Json::Int64>(status.stx_size)),
             md5,
             parent.string(),
             createdBuffer,
             updatedBuffer,
             userId,
             streamId.empty() ? Json::Value() : Json::Value(streamId),
             pipelineId.empty() ? Json::Value() : Json::Value(pipelineId));
    }
    if (sameMd5) {
      update((*current)[media::KEY_TYPE],
             (*current)[media::KEY_NAME],
             (*current)[media::KEY_WIDTH],
             (*current)[media::KEY_HEIGHT],
             (*current)[media::KEY_DURATION],
             (*current)[media::KEY_FPS],
             (*current)[media::KEY_CODEC],
             Json::Value(static_cast<Json::Int64>(status.stx_size)),
             (*current)[media::KEY_MD5],
             (*current)[media::KEY_FOLDER],
             (*current)[media::KEY_CREATED_AT],
             updatedBuffer,
             (*current)[media::KEY_USER_ID],
             (*current)[media::KEY_STREAM_ID],
             (*current)[media::KEY_PIPELINE_ID],
             (*current)[media::KEY_ID].asString());
    }
    if (changed) {
      update(type,
             path.filename().string(),
             width ? Json::Value(*width) : Json::Value(),
             height ? Json::Value(*height) : Json::Value(),
             image || !duration ? Json::Value() : Json::Value(*duration),
             image ? Json::Value() : (fps ? Json::Value(*fps) : Json::Value()),
             image ? Json::Value() : (codec ? Json::Value(*codec) : Json::Value()),
             Json::Value(static_cast<Json::Int64>(status.stx_size)),
             md5,
             parent.string(),
             (*current)[media::KEY_CREATED_AT],
             updatedBuffer,
             (*current)[media::KEY_USER_ID],
             (*current)[media::KEY_STREAM_ID],
             (*current)[media::KEY_PIPELINE_ID],
             (*current)[media::KEY_ID].asString());
    }
  }
  for (const auto& item : rowsByKey) {
    const bool present = seen.count(item.first) != 0;
    if (!present) {
      remove(item.second[media::KEY_ID].asString());
    }
  }
}

Json::Value MediaService::listFile(const std::string& folder,
                                 const std::string& ownerColumn,
                                 const std::string& ownerId,
                                 const std::string& type,
                                 int page,
                                 long pageSize,
                                 const std::string& search) {
  reloadFile(folder, ownerColumn, ownerId);
  const long offset = static_cast<long>(page - 1) * pageSize;
  const std::string prefix = (std::filesystem::path(root) / folder).lexically_normal().string();
  const std::string match = Searcher::clause({"name", "type", "codec", "folder", "md5"}, 4);
  const std::string from = "FROM media WHERE (folder = $1 OR folder LIKE $1 || '/%') AND (" + ownerColumn +
      "::text = $2 OR $2 = '') AND (type = $3 OR $3 = '') AND " + match + " ";
  const auto counted = Sql::sync({}, dbClient, "SELECT count(*) AS total " + from, prefix, ownerId, type, search);
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + SELECT_MEDIA + from + "ORDER BY created_at DESC LIMIT $5 OFFSET $6",
      prefix,
      ownerId,
      type,
      search,
      pageSize,
      offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(formatResult(mediaRow(row)));
  }
  Json::Value data;
  data[media::KEY_ITEMS] = items;
  data[media::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[media::KEY_PAGE] = page;
  data[media::KEY_PAGE_SIZE] = static_cast<Json::Int64>(pageSize);
  return data;
}

std::optional<std::string> MediaService::uuidOrEmpty(const Json::Value& value) {
  const std::optional<std::string> id =
      value.isNull() || value.asString().empty() ? std::nullopt : std::optional<std::string>(value.asString());
  return id;
}

Json::Value MediaService::get(const std::string& mediaId) {
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + SELECT_MEDIA + "FROM media WHERE id = $1::uuid", mediaId);
  if (rows.empty()) {
    throw std::runtime_error(media::ERR_NOT_FOUND);
  }
  const Json::Value data = formatResult(mediaRow(rows[0]));
  return data;
}

std::filesystem::path MediaService::mediaId2Path(const std::string& mediaId) {
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + SELECT_MEDIA + "FROM media WHERE id = $1::uuid", mediaId);
  const bool found = !rows.empty();
  const Json::Value data = found ? mediaRow(rows[0]) : Json::Value();
  const std::filesystem::path file = found
      ? std::filesystem::path(data[media::KEY_FOLDER].asString()) /
          std::filesystem::path(data[media::KEY_NAME].asString()).filename()
      : std::filesystem::path();
  const bool present = found && std::filesystem::is_regular_file(file);
  if (!present) {
    throw std::runtime_error(media::ERR_NOT_FOUND);
  }
  return file;
}

Json::Value MediaService::fileNode(const std::filesystem::path& path, const std::string& folder) {
  struct statx status;
  const int stated = statx(AT_FDCWD, path.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BTIME | STATX_MTIME | STATX_SIZE, &status);
  const bool born = stated == 0 && (status.stx_mask & STATX_BTIME) != 0;
  const struct statx_timestamp created = born ? status.stx_btime : status.stx_mtime;
  Json::Value node;
  node[media::KEY_BASENAME] = path.filename().string();
  node[media::KEY_CREATED_AT] = stated == 0 ? formatTime(static_cast<std::time_t>(created.tv_sec), media::TIME_FORMAT) : "";
  node[media::KEY_UPDATED_AT] = stated == 0 ? formatTime(static_cast<std::time_t>(status.stx_mtime.tv_sec), media::TIME_FORMAT) : "";
  node[media::KEY_SIZE] = stated == 0 ? static_cast<Json::UInt64>(status.stx_size) : Json::UInt64(0);
  node[media::KEY_FOLDER] = folder;
  return node;
}

Json::Value MediaService::create(const Json::Value& type,
                                 const Json::Value& name,
                                 const Json::Value& width,
                                 const Json::Value& height,
                                 const Json::Value& duration,
                                 const Json::Value& fps,
                                 const Json::Value& codec,
                                 const Json::Value& size,
                                 const Json::Value& md5,
                                 const Json::Value& folder,
                                 const Json::Value& createdAt,
                                 const Json::Value& updatedAt,
                                 const Json::Value& userId,
                                 const Json::Value& streamId,
                                 const Json::Value& pipelineId) {
  Json::Value data = media::MEDIA_ROW();
  data[media::KEY_TYPE] = type.isNull() ? data[media::KEY_TYPE] : type;
  data[media::KEY_NAME] = name.isNull() ? data[media::KEY_NAME] : name;
  data[media::KEY_WIDTH] = width.isNull() ? data[media::KEY_WIDTH] : width;
  data[media::KEY_HEIGHT] = height.isNull() ? data[media::KEY_HEIGHT] : height;
  const bool image = data[media::KEY_TYPE].asString() == media::TYPE_IMAGE;
  data[media::KEY_DURATION] = image || duration.isNull() ? Json::Value() : duration;
  data[media::KEY_FPS] = image || fps.isNull() ? Json::Value() : fps;
  data[media::KEY_CODEC] = image || codec.isNull() ? Json::Value() : codec;
  data[media::KEY_SIZE] = size.isNull() ? data[media::KEY_SIZE] : size;
  data[media::KEY_MD5] = md5.isNull() ? data[media::KEY_MD5] : md5;
  data[media::KEY_FOLDER] = folder.isNull() ? data[media::KEY_FOLDER] : folder;
  data[media::KEY_CREATED_AT] = createdAt.isNull() ? data[media::KEY_CREATED_AT] : createdAt;
  data[media::KEY_UPDATED_AT] = updatedAt.isNull() ? data[media::KEY_UPDATED_AT] : updatedAt;
  data[media::KEY_USER_ID] = userId.isNull() ? data[media::KEY_USER_ID] : userId;
  data[media::KEY_STREAM_ID] = streamId.isNull() ? data[media::KEY_STREAM_ID] : streamId;
  data[media::KEY_PIPELINE_ID] = pipelineId.isNull() ? data[media::KEY_PIPELINE_ID] : pipelineId;
  const std::optional<int> widthValue =
      data[media::KEY_WIDTH].isNull() ? std::nullopt : std::optional<int>(data[media::KEY_WIDTH].asInt());
  const std::optional<int> heightValue =
      data[media::KEY_HEIGHT].isNull() ? std::nullopt : std::optional<int>(data[media::KEY_HEIGHT].asInt());
  const std::optional<double> durationValue = data[media::KEY_DURATION].isNull()
      ? std::nullopt
      : std::optional<double>(data[media::KEY_DURATION].asDouble());
  const std::optional<double> fpsValue =
      data[media::KEY_FPS].isNull() ? std::nullopt : std::optional<double>(data[media::KEY_FPS].asDouble());
  const std::optional<std::string> codecValue = data[media::KEY_CODEC].isNull()
      ? std::nullopt
      : std::optional<std::string>(data[media::KEY_CODEC].asString());
  const std::optional<long> sizeValue = data[media::KEY_SIZE].isNull()
      ? std::nullopt
      : std::optional<long>(data[media::KEY_SIZE].asInt64());
  const auto rows = Sql::sync({}, dbClient,
      std::string("INSERT INTO media (id, type, name, width, height, duration, fps, codec, size, md5, folder, "
                  "created_at, updated_at, user_id, stream_id, pipeline_id) "
                  "VALUES (gen_random_uuid(), $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, "
                  "$11::timestamptz, $12::timestamptz, $13::uuid, $14::uuid, $15::uuid) RETURNING ") +
          SELECT_MEDIA,
      data[media::KEY_TYPE].asString(),
      data[media::KEY_NAME].asString(),
      widthValue,
      heightValue,
      durationValue,
      fpsValue,
      codecValue,
      sizeValue,
      data[media::KEY_MD5].asString(),
      data[media::KEY_FOLDER].asString(),
      data[media::KEY_CREATED_AT].asString(),
      data[media::KEY_UPDATED_AT].asString(),
      data[media::KEY_USER_ID].asString(),
      uuidOrEmpty(data[media::KEY_STREAM_ID]),
      uuidOrEmpty(data[media::KEY_PIPELINE_ID]));
  data = formatResult(mediaRow(rows[0]));
  return data;
}

Json::Value MediaService::removeFile(const std::string& mediaId) {
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + SELECT_MEDIA + "FROM media WHERE id = $1::uuid", mediaId);
  if (rows.empty()) {
    throw std::runtime_error(media::ERR_NOT_FOUND);
  }
  const Json::Value data = formatResult(mediaRow(rows[0]));
  const std::filesystem::path file = std::filesystem::path(data[media::KEY_FOLDER].asString()) /
      std::filesystem::path(data[media::KEY_NAME].asString()).filename();
  const bool present = std::filesystem::is_regular_file(file);
  if (present) {
    std::filesystem::remove(file);
  }
  return data;
}

Json::Value MediaService::remove(const std::string& mediaId) {
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + SELECT_MEDIA + "FROM media WHERE id = $1::uuid", mediaId);
  if (rows.empty()) {
    throw std::runtime_error(media::ERR_NOT_FOUND);
  }
  const Json::Value data = formatResult(mediaRow(rows[0]));
  Sql::sync({}, dbClient, "DELETE FROM media WHERE id = $1::uuid", mediaId);
  return data;
}

Json::Value MediaService::update(const Json::Value& type,
                                 const Json::Value& name,
                                 const Json::Value& width,
                                 const Json::Value& height,
                                 const Json::Value& duration,
                                 const Json::Value& fps,
                                 const Json::Value& codec,
                                 const Json::Value& size,
                                 const Json::Value& md5,
                                 const Json::Value& folder,
                                 const Json::Value& createdAt,
                                 const Json::Value& updatedAt,
                                 const Json::Value& userId,
                                 const Json::Value& streamId,
                                 const Json::Value& pipelineId,
                                 const std::string& mediaId) {
  Json::Value data = media::MEDIA_ROW();
  data[media::KEY_TYPE] = type.isNull() ? data[media::KEY_TYPE] : type;
  data[media::KEY_NAME] = name.isNull() ? data[media::KEY_NAME] : name;
  data[media::KEY_WIDTH] = width.isNull() ? data[media::KEY_WIDTH] : width;
  data[media::KEY_HEIGHT] = height.isNull() ? data[media::KEY_HEIGHT] : height;
  const bool image = data[media::KEY_TYPE].asString() == media::TYPE_IMAGE;
  data[media::KEY_DURATION] = image || duration.isNull() ? Json::Value() : duration;
  data[media::KEY_FPS] = image || fps.isNull() ? Json::Value() : fps;
  data[media::KEY_CODEC] = image || codec.isNull() ? Json::Value() : codec;
  data[media::KEY_SIZE] = size.isNull() ? data[media::KEY_SIZE] : size;
  data[media::KEY_MD5] = md5.isNull() ? data[media::KEY_MD5] : md5;
  data[media::KEY_FOLDER] = folder.isNull() ? data[media::KEY_FOLDER] : folder;
  data[media::KEY_CREATED_AT] = createdAt.isNull() ? data[media::KEY_CREATED_AT] : createdAt;
  data[media::KEY_UPDATED_AT] = updatedAt.isNull() ? data[media::KEY_UPDATED_AT] : updatedAt;
  data[media::KEY_USER_ID] = userId.isNull() ? data[media::KEY_USER_ID] : userId;
  data[media::KEY_STREAM_ID] = streamId.isNull() ? data[media::KEY_STREAM_ID] : streamId;
  data[media::KEY_PIPELINE_ID] = pipelineId.isNull() ? data[media::KEY_PIPELINE_ID] : pipelineId;
  const std::optional<int> widthValue =
      data[media::KEY_WIDTH].isNull() ? std::nullopt : std::optional<int>(data[media::KEY_WIDTH].asInt());
  const std::optional<int> heightValue =
      data[media::KEY_HEIGHT].isNull() ? std::nullopt : std::optional<int>(data[media::KEY_HEIGHT].asInt());
  const std::optional<double> durationValue = data[media::KEY_DURATION].isNull()
      ? std::nullopt
      : std::optional<double>(data[media::KEY_DURATION].asDouble());
  const std::optional<double> fpsValue =
      data[media::KEY_FPS].isNull() ? std::nullopt : std::optional<double>(data[media::KEY_FPS].asDouble());
  const std::optional<std::string> codecValue = data[media::KEY_CODEC].isNull()
      ? std::nullopt
      : std::optional<std::string>(data[media::KEY_CODEC].asString());
  const std::optional<long> sizeValue = data[media::KEY_SIZE].isNull()
      ? std::nullopt
      : std::optional<long>(data[media::KEY_SIZE].asInt64());
  const auto rows = Sql::sync({}, dbClient,
      std::string("UPDATE media SET type = $1, name = $2, width = $3, height = $4, duration = $5, fps = $6, "
                  "codec = $7, size = $8, md5 = $9, folder = $10, created_at = $11::timestamptz, "
                  "updated_at = $12::timestamptz, user_id = $13::uuid, stream_id = $14::uuid, "
                  "pipeline_id = $15::uuid WHERE id = $16::uuid RETURNING ") +
          SELECT_MEDIA,
      data[media::KEY_TYPE].asString(),
      data[media::KEY_NAME].asString(),
      widthValue,
      heightValue,
      durationValue,
      fpsValue,
      codecValue,
      sizeValue,
      data[media::KEY_MD5].asString(),
      data[media::KEY_FOLDER].asString(),
      data[media::KEY_CREATED_AT].asString(),
      data[media::KEY_UPDATED_AT].asString(),
      data[media::KEY_USER_ID].asString(),
      uuidOrEmpty(data[media::KEY_STREAM_ID]),
      uuidOrEmpty(data[media::KEY_PIPELINE_ID]),
      mediaId);
  if (rows.empty()) {
    throw std::runtime_error(media::ERR_NOT_FOUND);
  }
  data = formatResult(mediaRow(rows[0]));
  return data;
}

std::string MediaService::lowerExtension(const std::string& fileName) {
  std::string extension = std::filesystem::path(fileName).extension().string();
  for (char& character : extension) {
    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }
  return extension;
}

bool MediaService::suffixMatched(const std::string& extension, std::span<const char* const> suffixes) {
  bool matched = false;
  for (const char* suffix : suffixes) {
    matched = matched || extension == suffix;
  }
  return matched;
}

std::string MediaService::getType(const std::string& fileName) {
  const std::string extension = lowerExtension(fileName);
  const bool image = suffixMatched(extension, media::IMAGE_SUFFIXES);
  const bool video = suffixMatched(extension, media::VIDEO_SUFFIXES);
  if (!image && !video) {
    throw std::runtime_error(media::ERR_SUFFIX);
  }
  const std::string type = image ? media::TYPE_IMAGE : media::TYPE_VIDEO;
  return type;
}

Json::Value MediaService::mediaRow(const drogon::orm::Row& row) {
  Json::Value data = media::MEDIA_ROW();
  data[media::KEY_ID] = row["id"].as<std::string>();
  data[media::KEY_TYPE] = row["type"].as<std::string>();
  data[media::KEY_NAME] = row["name"].as<std::string>();
  data[media::KEY_WIDTH] = row["width"].isNull() ? Json::Value() : row["width"].as<int>();
  data[media::KEY_HEIGHT] = row["height"].isNull() ? Json::Value() : row["height"].as<int>();
  data[media::KEY_DURATION] = row["duration"].isNull() ? Json::Value() : row["duration"].as<double>();
  data[media::KEY_FPS] = row["fps"].isNull() ? Json::Value() : row["fps"].as<double>();
  data[media::KEY_CODEC] = row["codec"].isNull() ? Json::Value() : row["codec"].as<std::string>();
  data[media::KEY_SIZE] = row["size"].isNull() ? Json::Value() : Json::Value(static_cast<Json::Int64>(row["size"].as<long>()));
  data[media::KEY_MD5] = row["md5"].as<std::string>();
  data[media::KEY_FOLDER] = row["folder"].as<std::string>();
  data[media::KEY_CREATED_AT] = row["created_at"].as<std::string>();
  data[media::KEY_UPDATED_AT] = row["updated_at"].as<std::string>();
  data[media::KEY_USER_ID] = row["user_id"].as<std::string>();
  data[media::KEY_STREAM_ID] = row["stream_id"].isNull() ? Json::Value() : row["stream_id"].as<std::string>();
  data[media::KEY_PIPELINE_ID] = row["pipeline_id"].isNull() ? Json::Value() : row["pipeline_id"].as<std::string>();
  return data;
}

Json::Value MediaService::formatResult(Json::Value data) {
  const bool sized = !data[media::KEY_SIZE].isNull();
  const double megabytes = sized ? static_cast<double>(data[media::KEY_SIZE].asInt64()) / media::BYTES_PER_MB : 0;
  data[media::KEY_SIZE] = sized ? Json::Value(std::round(megabytes * 10.0) / 10.0) : data[media::KEY_SIZE];
  const bool timed = !data[media::KEY_DURATION].isNull();
  data[media::KEY_DURATION] = timed ? Json::Value(formatDuration(data[media::KEY_DURATION].asDouble())) : data[media::KEY_DURATION];
  return data;
}

std::string MediaService::formatDuration(double seconds) {
  const long total = static_cast<long>(seconds);
  const long safe = total < 0 ? 0 : total;
  const long hours = safe / 3600;
  const long minutes = (safe % 3600) / 60;
  const long secs = safe % 60;
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "%02ld:%02ld:%02ld", hours, minutes, secs);
  const std::string text(buffer);
  return text;
}

std::string MediaService::fileMd5(const std::string& bytes) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int length = 0;
  const int digested = EVP_Digest(bytes.data(), bytes.size(), digest, &length, EVP_md5(), nullptr);
  if (digested != 1) {
    throw std::runtime_error(media::ERR_MD5);
  }
  const std::string hex = digestHex(digest, length);
  return hex;
}

std::string MediaService::streamId2Name(const std::string& streamId) {
  const auto rows = Sql::sync({}, dbClient, "SELECT name FROM stream WHERE id = $1::uuid", streamId);
  const std::string name = rows[0]["name"].as<std::string>();
  return name;
}

std::string MediaService::pipelineId2Name(const std::string& pipelineId) {
  const auto rows = Sql::sync({}, dbClient, "SELECT name FROM pipeline WHERE id = $1::uuid", pipelineId);
  const std::string name = rows[0]["name"].as<std::string>();
  return name;
}

std::string MediaService::userId2Name(const std::string& userId) {
  const auto rows = Sql::sync({}, dbClient, "SELECT username FROM \"user\" WHERE id = $1::uuid", userId);
  const std::string name = rows[0]["username"].as<std::string>();
  return name;
}

std::filesystem::path MediaService::createUploadFolder(const std::string& name) {
  const std::filesystem::path folder = std::filesystem::path(root) / media::UPLOAD_DIR / name;
  std::filesystem::create_directories(folder);
  return folder;
}

std::string mediaInfoText(void* handle, MediaInfo_stream_C kind, const char* parameter) {
  const char* text = MediaInfo_Get(handle, kind, 0, parameter, MediaInfo_Info_Text, MediaInfo_Info_Name);
  const std::string value = text ? text : "";
  return value;
}

void MediaService::readProbe(void* handle,
                             bool image,
                             std::optional<int>& width,
                             std::optional<int>& height,
                             std::optional<double>& duration,
                             std::optional<double>& fps,
                             std::optional<std::string>& codec) {
  const std::string imageWidth = mediaInfoText(handle, MediaInfo_Stream_Image, media::MEDIAINFO_WIDTH);
  const std::string imageHeight = mediaInfoText(handle, MediaInfo_Stream_Image, media::MEDIAINFO_HEIGHT);
  const bool imageSized = image && !imageWidth.empty() && !imageHeight.empty();
  const MediaInfo_stream_C kind = imageSized ? MediaInfo_Stream_Image : MediaInfo_Stream_Video;
  const std::string widthText = mediaInfoText(handle, kind, media::MEDIAINFO_WIDTH);
  const std::string heightText = mediaInfoText(handle, kind, media::MEDIAINFO_HEIGHT);
  const std::string durationText = mediaInfoText(handle, kind, media::MEDIAINFO_DURATION);
  const std::string fpsText = mediaInfoText(handle, kind, media::MEDIAINFO_FPS);
  const std::string codecText = mediaInfoText(handle, kind, media::MEDIAINFO_CODEC);
  int widthValue = 0;
  int heightValue = 0;
  double durationValue = 0;
  double fpsValue = 0;
  const auto widthParsed = std::from_chars(widthText.data(), widthText.data() + widthText.size(), widthValue);
  const auto heightParsed = std::from_chars(heightText.data(), heightText.data() + heightText.size(), heightValue);
  const auto durationParsed = std::from_chars(durationText.data(), durationText.data() + durationText.size(), durationValue);
  const auto fpsParsed = std::from_chars(fpsText.data(), fpsText.data() + fpsText.size(), fpsValue);
  width = widthParsed.ec == std::errc() ? std::optional<int>(widthValue) : std::nullopt;
  height = heightParsed.ec == std::errc() ? std::optional<int>(heightValue) : std::nullopt;
  duration = durationParsed.ec == std::errc() ? std::optional<double>(durationValue / 1000.0) : std::nullopt;
  fps = fpsParsed.ec == std::errc() ? std::optional<double>(fpsValue) : std::nullopt;
  codec = codecText.empty() ? std::nullopt : std::optional<std::string>(codecText);
}

void MediaService::probeFile(const std::filesystem::path& path,
                             bool image,
                             std::optional<int>& width,
                             std::optional<int>& height,
                             std::optional<double>& duration,
                             std::optional<double>& fps,
                             std::optional<std::string>& codec) {
  MediaInfoBuffer info;
  void* handle = info.require();
  MediaInfo_Open(handle, path.c_str());
  readProbe(handle, image, width, height, duration, fps, codec);
}

void MediaService::writeUpload(const std::filesystem::path& folder, const std::string& fileName, const std::string& fileContent) {
  const std::filesystem::path name = std::filesystem::path(fileName).filename();
  std::ofstream output(folder / name, std::ios::binary);
  output.exceptions(std::ios::failbit | std::ios::badbit);
  output.write(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
}

Json::Value MediaService::upload(const std::string& userId, const std::string& fileName, const std::string& fileContent) {
  const std::string type = getType(fileName);
  const bool image = type == media::TYPE_IMAGE;
  const std::string name = std::filesystem::path(fileName).filename().string();
  const std::filesystem::path folder = createUploadFolder(userId2Name(userId)).lexically_normal();
  writeUpload(folder, name, fileContent);
  const std::filesystem::path path = folder / name;
  std::optional<int> width;
  std::optional<int> height;
  std::optional<double> duration;
  std::optional<double> fps;
  std::optional<std::string> codec;
  probeFile(path, image, width, height, duration, fps, codec);
  const std::string md5 = fileMd5(fileContent);
  struct statx status;
  const int stated = statx(AT_FDCWD, path.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BTIME | STATX_MTIME, &status);
  const std::time_t mtime = stated == 0 ? static_cast<std::time_t>(status.stx_mtime.tv_sec) : std::time(nullptr);
  const std::time_t born = stated == 0 && (status.stx_mask & STATX_BTIME) != 0
      ? static_cast<std::time_t>(status.stx_btime.tv_sec)
      : mtime;
  std::tm utc{};
  gmtime_r(&mtime, &utc);
  char updatedBuffer[40];
  std::strftime(updatedBuffer, sizeof(updatedBuffer), media::TIME_UTC_FORMAT, &utc);
  gmtime_r(&born, &utc);
  char createdBuffer[40];
  std::strftime(createdBuffer, sizeof(createdBuffer), media::TIME_UTC_FORMAT, &utc);
  const Json::Value widthValue = width ? Json::Value(*width) : Json::Value();
  const Json::Value heightValue = height ? Json::Value(*height) : Json::Value();
  const Json::Value durationValue = image || !duration ? Json::Value() : Json::Value(*duration);
  const Json::Value fpsValue = image ? Json::Value() : (fps ? Json::Value(*fps) : Json::Value());
  const Json::Value codecValue = image ? Json::Value() : (codec ? Json::Value(*codec) : Json::Value());
  const Json::Value sizeValue(static_cast<Json::Int64>(fileContent.size()));
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + SELECT_MEDIA +
          "FROM media WHERE folder = $1 AND name = $2 AND user_id = $3::uuid ORDER BY updated_at DESC LIMIT 1",
      folder.string(),
      name,
      userId);
  const bool found = !rows.empty();
  const Json::Value current = found ? mediaRow(rows[0]) : Json::Value();
  const Json::Value created = found
      ? update(type,
               name,
               widthValue,
               heightValue,
               durationValue,
               fpsValue,
               codecValue,
               sizeValue,
               md5,
               folder.string(),
               current[media::KEY_CREATED_AT],
               updatedBuffer,
               userId,
               Json::Value(),
               Json::Value(),
               current[media::KEY_ID].asString())
      : create(type,
               name,
               widthValue,
               heightValue,
               durationValue,
               fpsValue,
               codecValue,
               sizeValue,
               md5,
               folder.string(),
               createdBuffer,
               updatedBuffer,
               userId,
               Json::Value(),
               Json::Value());
  return created;
}

std::string MediaService::formatTime(std::time_t seconds, const char* format) {
  std::tm local{};
  localtime_r(&seconds, &local);
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), format, &local);
  const std::string text(buffer);
  return text;
}
