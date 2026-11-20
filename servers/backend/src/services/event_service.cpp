#include "services/event_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/event_const.hpp"
#include "constants/redis_const.hpp"
#include "constants/pipeline_const.hpp"
#include "constants/response_const.hpp"
#include "searcher/searcher.hpp"

#include <stdexcept>
#include <string>

namespace {

const char* SELECT_ROW =
    "event.id::text AS id, event.occurred_at, event.stream_id::text AS stream_id, stream.name AS stream_name, "
    "event.media_id::text AS media_id, media.name AS media_name, event.pipeline_id::text AS pipeline_id, "
    "pipeline.name AS pipeline_name, event.event_code, event.event_label, event.read_at, "
    "event.raw_path, event.visualization_path, event.annotations::text AS annotations ";

const char* FROM_EVENT =
    "FROM event "
    "LEFT JOIN stream ON stream.id = event.stream_id "
    "LEFT JOIN media ON media.id = event.media_id "
    "LEFT JOIN pipeline ON pipeline.id = event.pipeline_id ";

}

EventService::EventService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

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
                               eventNode[event::KEY_EVENT_LABEL],
                               Json::Value(),
                               Json::Value(),
                               annotations);
  return created;
}

Json::Value EventService::create(const Json::Value& occurredAt,
                                 const Json::Value& streamId,
                                 const Json::Value& mediaId,
                                 const Json::Value& pipelineId,
                                 const Json::Value& eventCode,
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
                  "event_code, event_label, raw_path, visualization_path, annotations) "
                  "VALUES (gen_random_uuid(), $1::timestamptz, NULLIF($2, '')::uuid, NULLIF($3, '')::uuid, "
                  "$4::uuid, $5, $6, NULLIF($7, ''), NULLIF($8, ''), "
                  "CASE WHEN $9 = '' THEN NULL ELSE $9::jsonb END) "
                  "RETURNING id, occurred_at, stream_id, media_id, pipeline_id, event_code, event_label, read_at, "
                  "raw_path, visualization_path, annotations) "
                  "SELECT ") +
          SELECT_ROW + FROM_EVENT,
      data[event::KEY_OCCURRED_AT].isNull() ? "" : data[event::KEY_OCCURRED_AT].asString(),
      data[event::KEY_STREAM_ID].isNull() ? "" : data[event::KEY_STREAM_ID].asString(),
      data[event::KEY_MEDIA_ID].isNull() ? "" : data[event::KEY_MEDIA_ID].asString(),
      data[event::KEY_PIPELINE_ID].isNull() ? "" : data[event::KEY_PIPELINE_ID].asString(),
      data[event::KEY_EVENT_CODE].isNull() ? "" : data[event::KEY_EVENT_CODE].asString(),
      data[event::KEY_EVENT_LABEL].isNull() ? "" : data[event::KEY_EVENT_LABEL].asString(),
      data[event::KEY_RAW_PATH].isNull() ? "" : data[event::KEY_RAW_PATH].asString(),
      data[event::KEY_VISUALIZATION_PATH].isNull() ? "" : data[event::KEY_VISUALIZATION_PATH].asString(),
      annotationsText);
  data = row2json(rows[0]);
  return data;
}

Json::Value EventService::setRead(const std::string& eventId, bool read) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("WITH event AS ("
                  "UPDATE event SET read_at = CASE WHEN $2::boolean THEN now() ELSE NULL END WHERE id = $1::uuid "
                  "RETURNING id, occurred_at, stream_id, media_id, pipeline_id, event_code, event_label, read_at, "
                  "raw_path, visualization_path, annotations) "
                  "SELECT ") +
          SELECT_ROW + FROM_EVENT,
      eventId,
      read);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  const char* noticeSql = read
      ? "UPDATE notice SET read_at = now() WHERE source = $2 AND ref_id = $1::uuid AND read_at IS NULL"
      : "UPDATE notice SET read_at = NULL WHERE source = $2 AND ref_id = $1::uuid";
  Sql::sync({}, dbClient, noticeSql, eventId, event::NOTICE_SOURCE);
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value EventService::remove(const std::string& eventId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("WITH event AS ("
                  "DELETE FROM event WHERE id = $1::uuid "
                  "RETURNING id, occurred_at, stream_id, media_id, pipeline_id, event_code, event_label, read_at, "
                  "raw_path, visualization_path, annotations) "
                  "SELECT ") +
          SELECT_ROW + FROM_EVENT,
      eventId);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value EventService::list(const std::string& search, int page, long pageSize) {
  const long offset = static_cast<long>(page - 1) * pageSize;
  const std::string match = Searcher::clause(
      {"event.id", "event.stream_id", "event.media_id", "event.pipeline_id", "event.event_code", "event.event_label"}, 1);
  const std::string from = std::string(FROM_EVENT) + "WHERE " + match + " ";
  const auto counted = Sql::sync({}, dbClient, "SELECT count(*) AS total " + from, search);
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + SELECT_ROW + from + "ORDER BY event.occurred_at DESC, event.id DESC LIMIT $2 OFFSET $3",
      search,
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
  data[event::KEY_EVENT_LABEL] = row["event_label"].as<std::string>();
  data[event::KEY_READ_AT] = row["read_at"].isNull() ? Json::Value() : row["read_at"].as<std::string>();
  data[event::KEY_RAW_PATH] = row["raw_path"].isNull() ? Json::Value() : row["raw_path"].as<std::string>();
  data[event::KEY_VISUALIZATION_PATH] =
      row["visualization_path"].isNull() ? Json::Value() : row["visualization_path"].as<std::string>();
  data[event::KEY_ANNOTATIONS] = annotations.isNull() ? Json::Value() : annotations;
  return data;
}
