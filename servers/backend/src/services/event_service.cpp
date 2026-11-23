#include "services/event_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/event_const.hpp"
#include "constants/redis_const.hpp"
#include "constants/pipeline_const.hpp"
#include "constants/response_const.hpp"
#include "searcher/searcher.hpp"
#include "services/event_hub.hpp"

#include <MediaInfoDLL/MediaInfoDLL.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <charconv>
#include <exception>
#include <clocale>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

const char* SELECT_ROW =
    "event.id::text AS id, event.occurred_at, event.stream_id::text AS stream_id, stream.name AS stream_name, "
    "event.media_id::text AS media_id, media.name AS media_name, event.pipeline_id::text AS pipeline_id, "
    "pipeline.name AS pipeline_name, event.event_code, event.level, event.event_label, event.read_at, "
    "event.reader_id::text AS reader_id, reader.username AS reader_name, "
    "event.raw_path, event.visualization_path, event.annotations::text AS annotations ";

const char* FROM_EVENT =
    "FROM event "
    "LEFT JOIN stream ON stream.id = event.stream_id "
    "LEFT JOIN media ON media.id = event.media_id "
    "LEFT JOIN pipeline ON pipeline.id = event.pipeline_id "
    "LEFT JOIN \"user\" AS reader ON reader.id = event.reader_id ";

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
      throw std::runtime_error(event::ERR_CONTENT);
    }
    return handle;
  }
  void* handle;
};

}

EventService::EventService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

std::string EventService::token2UserId(const std::string& token) {
  const auto rows = Sql::sync({}, dbClient,
      "SELECT id::text AS id FROM \"user\" WHERE token = $1 AND active", token);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  const std::string id = rows[0]["id"].as<std::string>();
  return id;
}

Json::Value EventService::get(const std::string& eventId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + SELECT_ROW + FROM_EVENT + "WHERE event.id = $1::uuid", eventId);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value EventService::createFromRedis(const Json::Value& message, const std::string& topic) {
  const std::string prefix = redis::TOPIC_PREFIX;
  const std::string name = topic.rfind(prefix, 0) == 0 ? topic.substr(prefix.size()) : topic;
  const auto rows = Sql::sync({}, dbClient,
      "SELECT id::text AS id, input::text AS input FROM pipeline WHERE name = $1",
      name);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_PIPELINE);
  }
  Json::Value input;
  Json::Reader().parse(rows[0]["input"].as<std::string>(), input);
  const Json::Value streams = input.isMember(pipeline::KEY_STREAMS) ? input[pipeline::KEY_STREAMS] : Json::Value(Json::arrayValue);
  const Json::Value videos = input.isMember(pipeline::KEY_VIDEOS) ? input[pipeline::KEY_VIDEOS] : Json::Value(Json::arrayValue);
  const Json::Value images = input.isMember(pipeline::KEY_IMAGES) ? input[pipeline::KEY_IMAGES] : Json::Value(Json::arrayValue);
  const bool streamSource = streams.isArray() && !streams.empty();
  const Json::Value& sources = streamSource ? streams : (videos.isArray() && !videos.empty() ? videos : images);
  const int pad = message[redis::KEY_PAD].asInt();
  if (!sources.isArray() || pad < 0 || static_cast<Json::ArrayIndex>(pad) >= sources.size() ||
      !sources[static_cast<Json::ArrayIndex>(pad)].isObject()) {
    throw std::runtime_error(event::ERR_PAD);
  }
  const Json::Value sourceId = sources[static_cast<Json::ArrayIndex>(pad)][pipeline::KEY_ID];
  Json::Value annotations(Json::objectValue);
  annotations[redis::KEY_NUM] = message[redis::KEY_NUM];
  annotations[redis::KEY_OBJECT] = message[redis::KEY_OBJECT];
  const Json::Value& eventNode = message[redis::KEY_EVENT];
  Json::Value created = create(message[redis::KEY_TS],
                               streamSource ? sourceId : Json::Value(),
                               streamSource ? Json::Value() : sourceId,
                               Json::Value(rows[0]["id"].as<std::string>()),
                               eventNode[event::KEY_EVENT_CODE],
                               eventNode[event::KEY_LEVEL],
                               eventNode[event::KEY_EVENT_LABEL],
                               eventNode[event::KEY_RAW_PATH],
                               eventNode[event::KEY_VISUALIZATION_PATH],
                               annotations);
  return created;
}

Json::Value EventService::create(const Json::Value& occurredAt,
                                 const Json::Value& streamId,
                                 const Json::Value& mediaId,
                                 const Json::Value& pipelineId,
                                 const Json::Value& eventCode,
                                 const Json::Value& level,
                                 const Json::Value& eventLabel,
                                 const Json::Value& rawPath,
                                 const Json::Value& visualizationPath,
                                 const Json::Value& annotations) {
  Json::Value data = event::ROW();
  data[event::KEY_OCCURRED_AT] = occurredAt.isNull() ? data[event::KEY_OCCURRED_AT] : occurredAt;
  data[event::KEY_STREAM_ID] = streamId.isNull() ? data[event::KEY_STREAM_ID] : streamId;
  data[event::KEY_MEDIA_ID] = mediaId.isNull() ? data[event::KEY_MEDIA_ID] : mediaId;
  data[event::KEY_PIPELINE_ID] = pipelineId.isNull() ? data[event::KEY_PIPELINE_ID] : pipelineId;
  data[event::KEY_EVENT_CODE] = eventCode.isNull() ? data[event::KEY_EVENT_CODE] : eventCode;
  data[event::KEY_LEVEL] = level.isNull() ? data[event::KEY_LEVEL] : level;
  data[event::KEY_EVENT_LABEL] = eventLabel.isNull() ? data[event::KEY_EVENT_LABEL] : eventLabel;
  data[event::KEY_RAW_PATH] = rawPath.isNull() ? data[event::KEY_RAW_PATH] : rawPath;
  data[event::KEY_VISUALIZATION_PATH] =
      visualizationPath.isNull() ? data[event::KEY_VISUALIZATION_PATH] : visualizationPath;
  data[event::KEY_ANNOTATIONS] = annotations.isNull() ? data[event::KEY_ANNOTATIONS] : annotations;
  Json::StreamWriterBuilder builder;
  builder[http::JSON_INDENTATION] = "";
  builder[http::JSON_PRECISION_TYPE] = http::JSON_PRECISION_DECIMAL;
  builder[http::JSON_PRECISION] = http::JSON_PRECISION_DIGITS;
  const std::string annotationsText =
      data[event::KEY_ANNOTATIONS].isNull() ? "" : Json::writeString(builder, data[event::KEY_ANNOTATIONS]);
  const auto rows = Sql::sync({}, dbClient, 
      std::string("WITH event AS ("
                  "INSERT INTO event (id, occurred_at, stream_id, media_id, pipeline_id, "
                  "event_code, level, event_label, raw_path, visualization_path, annotations) "
                  "VALUES (gen_random_uuid(), $1::timestamptz, NULLIF($2, '')::uuid, NULLIF($3, '')::uuid, "
                  "$4::uuid, $5, $6, $7, NULLIF($8, ''), NULLIF($9, ''), "
                  "CASE WHEN $10 = '' THEN NULL ELSE $10::jsonb END) "
                  "RETURNING id, occurred_at, stream_id, media_id, pipeline_id, event_code, level, event_label, read_at, "
                  "reader_id, raw_path, visualization_path, annotations) "
                  "SELECT ") +
          SELECT_ROW + FROM_EVENT,
      data[event::KEY_OCCURRED_AT].isNull() ? "" : data[event::KEY_OCCURRED_AT].asString(),
      data[event::KEY_STREAM_ID].isNull() ? "" : data[event::KEY_STREAM_ID].asString(),
      data[event::KEY_MEDIA_ID].isNull() ? "" : data[event::KEY_MEDIA_ID].asString(),
      data[event::KEY_PIPELINE_ID].isNull() ? "" : data[event::KEY_PIPELINE_ID].asString(),
      data[event::KEY_EVENT_CODE].isNull() ? "" : data[event::KEY_EVENT_CODE].asString(),
      data[event::KEY_LEVEL].isNull() ? "" : data[event::KEY_LEVEL].asString(),
      data[event::KEY_EVENT_LABEL].isNull() ? "" : data[event::KEY_EVENT_LABEL].asString(),
      data[event::KEY_RAW_PATH].isNull() ? "" : data[event::KEY_RAW_PATH].asString(),
      data[event::KEY_VISUALIZATION_PATH].isNull() ? "" : data[event::KEY_VISUALIZATION_PATH].asString(),
      annotationsText);
  data = row2json(rows[0]);
  notify(event::ACTION_INSERT, event::BANNER_EVENT(data));
  return data;
}

Json::Value EventService::readRow(const std::string& eventId, bool read, const std::string& userId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("WITH event AS ("
                  "UPDATE event SET read_at = CASE WHEN $2::boolean THEN now() ELSE NULL END, "
                  "reader_id = CASE WHEN $2::boolean THEN NULLIF($3, '')::uuid ELSE NULL END "
                  "WHERE id = $1::uuid "
                  "RETURNING id, occurred_at, stream_id, media_id, pipeline_id, event_code, level, event_label, read_at, "
                  "reader_id, raw_path, visualization_path, annotations) "
                  "SELECT ") +
          SELECT_ROW + FROM_EVENT,
      eventId,
      read,
      userId);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value EventService::removeRow(const std::string& eventId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("WITH event AS ("
                  "DELETE FROM event WHERE id = $1::uuid "
                  "RETURNING id, occurred_at, stream_id, media_id, pipeline_id, event_code, level, event_label, read_at, "
                  "reader_id, raw_path, visualization_path, annotations) "
                  "SELECT ") +
          SELECT_ROW + FROM_EVENT,
      eventId);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

void EventService::notify(const char* action, const Json::Value& event) {
  EventHub::shared().banner(action, event);
  EventHub::shared().refresh();
}

Json::Value EventService::setRead(const std::string& eventId, bool read, const std::string& userId) {
  Json::Value data = readRow(eventId, read, userId);
  notify(read ? event::ACTION_READ : event::ACTION_UNREAD, event::BANNER_EVENT(data));
  return data;
}

Json::Value EventService::batchSetRead(const Json::Value& ids, bool read, const std::string& userId) {
  Json::Value data = event::BATCH_DATA();
  Json::Value events(Json::arrayValue);
  if (ids.isArray()) {
    for (const auto& node : ids) {
      try {
        const Json::Value row = readRow(node.asString(), read, userId);
        data[event::KEY_UPDATED_ROWS].append(row);
        events.append(event::BANNER_EVENT(row));
      } catch (const std::exception&) {
        Json::Value failed;
        failed[event::KEY_ID] = node.asString();
        data[event::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[event::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[event::KEY_UPDATED_ROWS].size());
  data[event::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[event::KEY_FAILED_ROWS].size());
  if (!events.empty()) {
    notify(read ? event::ACTION_BATCH_READ : event::ACTION_BATCH_UNREAD, events);
  }
  return data;
}

Json::Value EventService::remove(const std::string& eventId) {
  Json::Value data = removeRow(eventId);
  notify(event::ACTION_REMOVE, event::BANNER_EVENT(data));
  return data;
}

Json::Value EventService::batchRemove(const Json::Value& ids) {
  Json::Value data = event::BATCH_DATA();
  Json::Value events(Json::arrayValue);
  if (ids.isArray()) {
    for (const auto& node : ids) {
      try {
        const Json::Value row = removeRow(node.asString());
        data[event::KEY_UPDATED_ROWS].append(row);
        events.append(event::BANNER_EVENT(row));
      } catch (const std::exception&) {
        Json::Value failed;
        failed[event::KEY_ID] = node.asString();
        data[event::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[event::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[event::KEY_UPDATED_ROWS].size());
  data[event::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[event::KEY_FAILED_ROWS].size());
  if (!events.empty()) {
    notify(event::ACTION_BATCH_REMOVE, events);
  }
  return data;
}

Json::Value EventService::list(const std::string& search, const std::string& level, int page, long pageSize) {
  const long offset = static_cast<long>(page - 1) * pageSize;
  const std::string match = Searcher::clause(
      {"event.id", "event.stream_id", "event.media_id", "event.pipeline_id", "event.event_code", "event.event_label"}, 1);
  const std::string levelMatch = std::string("($2 = '' OR $2 = '") + event::LEVEL_ALL + "' OR event.level = $2)";
  const std::string from = std::string(FROM_EVENT) + "WHERE " + match + " AND " + levelMatch + " ";
  const auto counted = Sql::sync({}, dbClient, "SELECT count(*) AS total " + from, search, level);
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + SELECT_ROW + from + "ORDER BY event.occurred_at DESC, event.id DESC LIMIT $3 OFFSET $4",
      search,
      level,
      pageSize,
      offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(row2json(row));
  }
  Json::Value data;
  data[event::KEY_ITEMS] = items;
  data[event::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[event::KEY_PAGE] = page;
  data[event::KEY_PAGE_SIZE] = pageSize;
  return data;
}

Json::Value EventService::row2json(const drogon::orm::Row& row) {
  Json::Value annotations;
  if (!row["annotations"].isNull()) {
    Json::Reader().parse(row["annotations"].as<std::string>(), annotations);
  }
  Json::Value data = event::ROW();
  data[event::KEY_ID] = row["id"].as<std::string>();
  data[event::KEY_OCCURRED_AT] = row["occurred_at"].as<std::string>();
  data[event::KEY_STREAM_ID] = row["stream_id"].isNull() ? Json::Value() : row["stream_id"].as<std::string>();
  data[event::KEY_STREAM_NAME] = row["stream_name"].isNull() ? Json::Value() : row["stream_name"].as<std::string>();
  data[event::KEY_MEDIA_ID] = row["media_id"].isNull() ? Json::Value() : row["media_id"].as<std::string>();
  data[event::KEY_MEDIA_NAME] = row["media_name"].isNull() ? Json::Value() : row["media_name"].as<std::string>();
  data[event::KEY_PIPELINE_ID] = row["pipeline_id"].as<std::string>();
  data[event::KEY_PIPELINE_NAME] = row["pipeline_name"].isNull() ? Json::Value() : row["pipeline_name"].as<std::string>();
  data[event::KEY_EVENT_CODE] = row["event_code"].as<std::string>();
  data[event::KEY_LEVEL] = row["level"].as<std::string>();
  data[event::KEY_EVENT_LABEL] = row["event_label"].as<std::string>();
  data[event::KEY_READ_AT] = row["read_at"].isNull() ? Json::Value() : row["read_at"].as<std::string>();
  data[event::KEY_READER_ID] = row["reader_id"].isNull() ? Json::Value() : row["reader_id"].as<std::string>();
  data[event::KEY_READER_NAME] = row["reader_name"].isNull() ? Json::Value() : row["reader_name"].as<std::string>();
  data[event::KEY_RAW_PATH] = row["raw_path"].isNull() ? Json::Value() : row["raw_path"].as<std::string>();
  data[event::KEY_VISUALIZATION_PATH] =
      row["visualization_path"].isNull() ? Json::Value() : row["visualization_path"].as<std::string>();
  data[event::KEY_ANNOTATIONS] = annotations.isNull() ? Json::Value() : annotations;
  return data;
}

std::string EventService::contentPath(const std::string& eventId, const std::string& kind) {
  const bool raw = kind == event::KIND_RAW;
  const bool visualization = kind == event::KIND_VISUALIZATION;
  if (!raw && !visualization) {
    throw std::runtime_error(event::ERR_KIND);
  }
  const Json::Value row = get(eventId);
  const Json::Value stored = raw ? row[event::KEY_RAW_PATH] : row[event::KEY_VISUALIZATION_PATH];
  const std::string path = stored.isString() ? stored.asString() : "";
  if (path.empty()) {
    throw std::runtime_error(event::ERR_CONTENT);
  }
  return path;
}

Json::Value EventService::contentInfo(const std::string& eventId, const std::string& kind) {
  const std::filesystem::path path(contentPath(eventId, kind));
  struct statx status;
  const int stated = statx(AT_FDCWD, path.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BTIME | STATX_MTIME | STATX_SIZE, &status);
  const bool localeReady = std::setlocale(LC_CTYPE, event::LOCALE_CTYPE) != nullptr;
  MediaInfoBuffer mediaInfo;
  void* handle = mediaInfo.require();
  const std::size_t opened = stated == 0 && localeReady ? MediaInfo_Open(handle, path.c_str()) : 0;
  if (stated != 0 || !localeReady || opened == 0) {
    throw std::runtime_error(event::ERR_CONTENT);
  }
  const char* widthText = MediaInfo_Get(handle, MediaInfo_Stream_Image, 0, event::MEDIAINFO_WIDTH, MediaInfo_Info_Text, MediaInfo_Info_Name);
  const char* heightText = MediaInfo_Get(handle, MediaInfo_Stream_Image, 0, event::MEDIAINFO_HEIGHT, MediaInfo_Info_Text, MediaInfo_Info_Name);
  const bool imageSized = widthText != nullptr && heightText != nullptr && widthText[0] != '\0' && heightText[0] != '\0';
  const MediaInfo_stream_C stream = imageSized ? MediaInfo_Stream_Image : MediaInfo_Stream_Video;
  const char* widthSource = imageSized ? widthText : MediaInfo_Get(handle, stream, 0, event::MEDIAINFO_WIDTH, MediaInfo_Info_Text, MediaInfo_Info_Name);
  const char* heightSource = imageSized ? heightText : MediaInfo_Get(handle, stream, 0, event::MEDIAINFO_HEIGHT, MediaInfo_Info_Text, MediaInfo_Info_Name);
  const std::string widthValue = widthSource ? widthSource : "";
  const std::string heightValue = heightSource ? heightSource : "";
  int width = 0;
  int height = 0;
  const auto widthParsed = std::from_chars(widthValue.data(), widthValue.data() + widthValue.size(), width);
  const auto heightParsed = std::from_chars(heightValue.data(), heightValue.data() + heightValue.size(), height);
  const std::time_t mtime = static_cast<std::time_t>(status.stx_mtime.tv_sec);
  const std::time_t born = (status.stx_mask & STATX_BTIME) != 0 ? static_cast<std::time_t>(status.stx_btime.tv_sec) : mtime;
  std::tm utc{};
  gmtime_r(&born, &utc);
  char createdBuffer[40];
  std::strftime(createdBuffer, sizeof(createdBuffer), event::TIME_UTC_FORMAT, &utc);
  gmtime_r(&mtime, &utc);
  char updatedBuffer[40];
  std::strftime(updatedBuffer, sizeof(updatedBuffer), event::TIME_UTC_FORMAT, &utc);
  const double megabytes = static_cast<double>(status.stx_size) / event::BYTES_PER_MB;
  Json::Value info;
  info[event::KEY_WIDTH] = widthParsed.ec == std::errc() ? Json::Value(width) : Json::Value();
  info[event::KEY_HEIGHT] = heightParsed.ec == std::errc() ? Json::Value(height) : Json::Value();
  info[event::KEY_SIZE] = Json::Value(std::round(megabytes * 10.0) / 10.0);
  info[event::KEY_CREATED_AT] = createdBuffer;
  info[event::KEY_UPDATED_AT] = updatedBuffer;
  return info;
}
