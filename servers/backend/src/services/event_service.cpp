#include "services/event_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/event_const.hpp"
#include "constants/kafka_const.hpp"
#include "searcher/searcher.hpp"

#include <stdexcept>
#include <string>

namespace {

const char* SELECT_ROW =
    "id::text AS id, occurred_at, stream_id::text AS stream_id, "
    "pipeline_id::text AS pipeline_id, event_code, event_label, status, "
    "raw_path, visualization_path, annotations::text AS annotations ";

}

EventService::EventService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value EventService::get(const std::string& eventId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + SELECT_ROW + "FROM event WHERE id = $1::uuid", eventId);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value EventService::createFromKafka(const Json::Value& message, const std::string& topic) {
  const std::string prefix = kafka::TOPIC_PREFIX;
  const std::string name = topic.rfind(prefix, 0) == 0 ? topic.substr(prefix.size()) : topic;
  const auto rows = Sql::sync({}, dbClient,
      "SELECT id::text AS id, array_to_json(stream_ids)::text AS stream_ids FROM pipeline WHERE name = $1",
      name);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_PIPELINE);
  }
  Json::Value streamIds;
  Json::Reader().parse(rows[0]["stream_ids"].as<std::string>(), streamIds);
  const int pad = message[kafka::KEY_PAD].asInt();
  if (!streamIds.isArray() || pad < 0 || static_cast<Json::ArrayIndex>(pad) >= streamIds.size()) {
    throw std::runtime_error(event::ERR_PAD);
  }
  Json::Value annotations(Json::objectValue);
  annotations[kafka::KEY_NUM] = message[kafka::KEY_NUM];
  annotations[kafka::KEY_OBJECT] = message[kafka::KEY_OBJECT];
  const Json::Value& eventNode = message[kafka::KEY_EVENT];
  Json::Value created = create(message[kafka::KEY_TS],
                               streamIds[static_cast<Json::ArrayIndex>(pad)],
                               Json::Value(rows[0]["id"].as<std::string>()),
                               eventNode[event::KEY_EVENT_CODE],
                               eventNode[event::KEY_EVENT_LABEL],
                               Json::Value(event::STATUS_NEW),
                               Json::Value(),
                               Json::Value(),
                               annotations);
  return created;
}

Json::Value EventService::create(const Json::Value& occurredAt,
                                 const Json::Value& streamId,
                                 const Json::Value& pipelineId,
                                 const Json::Value& eventCode,
                                 const Json::Value& eventLabel,
                                 const Json::Value& status,
                                 const Json::Value& rawPath,
                                 const Json::Value& visualizationPath,
                                 const Json::Value& annotations) {
  Json::Value data = event::ROW();
  data[event::KEY_OCCURRED_AT] = occurredAt.isNull() ? data[event::KEY_OCCURRED_AT] : occurredAt;
  data[event::KEY_STREAM_ID] = streamId.isNull() ? data[event::KEY_STREAM_ID] : streamId;
  data[event::KEY_PIPELINE_ID] = pipelineId.isNull() ? data[event::KEY_PIPELINE_ID] : pipelineId;
  data[event::KEY_EVENT_CODE] = eventCode.isNull() ? data[event::KEY_EVENT_CODE] : eventCode;
  data[event::KEY_EVENT_LABEL] = eventLabel.isNull() ? data[event::KEY_EVENT_LABEL] : eventLabel;
  data[event::KEY_STATUS] = status.isNull() ? data[event::KEY_STATUS] : status;
  data[event::KEY_RAW_PATH] = rawPath.isNull() ? data[event::KEY_RAW_PATH] : rawPath;
  data[event::KEY_VISUALIZATION_PATH] =
      visualizationPath.isNull() ? data[event::KEY_VISUALIZATION_PATH] : visualizationPath;
  data[event::KEY_ANNOTATIONS] = annotations.isNull() ? data[event::KEY_ANNOTATIONS] : annotations;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const std::string annotationsText =
      data[event::KEY_ANNOTATIONS].isNull() ? "" : Json::writeString(builder, data[event::KEY_ANNOTATIONS]);
  const auto rows = Sql::sync({}, dbClient, 
      std::string("INSERT INTO event (id, occurred_at, stream_id, pipeline_id, "
                  "event_code, event_label, status, raw_path, visualization_path, annotations) "
                  "VALUES (gen_random_uuid(), $1::timestamptz, $2::uuid, $3::uuid, $4, $5, "
                  "COALESCE(NULLIF($6, ''), 'new'), NULLIF($7, ''), NULLIF($8, ''), "
                  "CASE WHEN $9 = '' THEN NULL ELSE $9::jsonb END) RETURNING ") +
          SELECT_ROW,
      data[event::KEY_OCCURRED_AT].isNull() ? "" : data[event::KEY_OCCURRED_AT].asString(),
      data[event::KEY_STREAM_ID].isNull() ? "" : data[event::KEY_STREAM_ID].asString(),
      data[event::KEY_PIPELINE_ID].isNull() ? "" : data[event::KEY_PIPELINE_ID].asString(),
      data[event::KEY_EVENT_CODE].isNull() ? "" : data[event::KEY_EVENT_CODE].asString(),
      data[event::KEY_EVENT_LABEL].isNull() ? "" : data[event::KEY_EVENT_LABEL].asString(),
      data[event::KEY_STATUS].isNull() ? "" : data[event::KEY_STATUS].asString(),
      data[event::KEY_RAW_PATH].isNull() ? "" : data[event::KEY_RAW_PATH].asString(),
      data[event::KEY_VISUALIZATION_PATH].isNull() ? "" : data[event::KEY_VISUALIZATION_PATH].asString(),
      annotationsText);
  data = row2json(rows[0]);
  return data;
}

Json::Value EventService::setStatus(const std::string& eventId, const std::string& status) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("UPDATE event SET status = $2 WHERE id = $1::uuid RETURNING ") + SELECT_ROW, eventId, status);
  if (rows.empty()) {
    throw std::runtime_error(event::ERR_NOT_FOUND);
  }
  if (status == event::STATUS_HANDLED) {
    Sql::sync({}, dbClient, 
        "UPDATE notice SET read_at = now() WHERE source = $2 AND ref_id = $1::uuid AND read_at IS NULL",
        eventId,
        event::NOTICE_SOURCE);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value EventService::list(const std::string& search, int page, long pageSize) {
  const long offset = static_cast<long>(page - 1) * pageSize;
  const std::string match = Searcher::clause(
      {"id", "stream_id", "pipeline_id", "event_code", "event_label", "status"}, 1);
  const std::string from = "FROM event WHERE " + match + " ";
  const auto counted = Sql::sync({}, dbClient, "SELECT count(*) AS total " + from, search);
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + SELECT_ROW + from + "ORDER BY occurred_at DESC, id DESC LIMIT $2 OFFSET $3",
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
  data[event::KEY_STREAM_ID] = row["stream_id"].as<std::string>();
  data[event::KEY_PIPELINE_ID] = row["pipeline_id"].as<std::string>();
  data[event::KEY_EVENT_CODE] = row["event_code"].as<std::string>();
  data[event::KEY_EVENT_LABEL] = row["event_label"].as<std::string>();
  data[event::KEY_STATUS] = row["status"].as<std::string>();
  data[event::KEY_RAW_PATH] = row["raw_path"].isNull() ? Json::Value() : row["raw_path"].as<std::string>();
  data[event::KEY_VISUALIZATION_PATH] =
      row["visualization_path"].isNull() ? Json::Value() : row["visualization_path"].as<std::string>();
  data[event::KEY_ANNOTATIONS] = annotations.isNull() ? Json::Value() : annotations;
  return data;
}
