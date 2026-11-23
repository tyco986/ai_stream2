#include "services/notice_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/event_const.hpp"
#include "constants/notice_const.hpp"

#include <stdexcept>
#include <string>

namespace {

const char* SELECT_ROW =
    "id::text AS id, created_at, read_at, read_by::text AS read_by, level, source, text, "
    "ref_id::text AS ref_id ";

}

NoticeService::NoticeService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

std::string NoticeService::token2UserId(const std::string& token) {
  const auto rows = Sql::sync({}, dbClient,
      "SELECT id::text AS id FROM \"user\" WHERE token = $1 AND active", token);
  if (rows.empty()) {
    throw std::runtime_error(notice::ERR_NOT_FOUND);
  }
  const std::string id = rows[0]["id"].as<std::string>();
  return id;
}

Json::Value NoticeService::create(const Json::Value& createdAt,
                                  const Json::Value& level,
                                  const Json::Value& source,
                                  const Json::Value& text,
                                  const Json::Value& refId) {
  const std::string levelText = level.isNull() ? "" : level.asString();
  if (levelText != notice::LEVEL_SYSTEM) {
    throw std::runtime_error(notice::ERR_LEVEL);
  }
  Json::Value data = notice::ROW();
  data[notice::KEY_CREATED_AT] = createdAt.isNull() ? data[notice::KEY_CREATED_AT] : createdAt;
  data[notice::KEY_LEVEL] = levelText;
  data[notice::KEY_SOURCE] = source.isNull() ? data[notice::KEY_SOURCE] : source;
  data[notice::KEY_TEXT] = text.isNull() ? data[notice::KEY_TEXT] : text;
  data[notice::KEY_REF_ID] = refId.isNull() ? data[notice::KEY_REF_ID] : refId;
  const auto rows = Sql::sync({}, dbClient, 
      std::string("INSERT INTO notice (id, created_at, level, source, text, ref_id) "
                  "VALUES (gen_random_uuid(), COALESCE(NULLIF($1, '')::timestamptz, now()), $2, $3, $4, "
                  "NULLIF($5, '')::uuid) RETURNING ") +
          SELECT_ROW,
      data[notice::KEY_CREATED_AT].isNull() ? "" : data[notice::KEY_CREATED_AT].asString(),
      data[notice::KEY_LEVEL].isNull() ? "" : data[notice::KEY_LEVEL].asString(),
      data[notice::KEY_SOURCE].isNull() ? "" : data[notice::KEY_SOURCE].asString(),
      data[notice::KEY_TEXT].isNull() ? "" : data[notice::KEY_TEXT].asString(),
      data[notice::KEY_REF_ID].isNull() ? "" : data[notice::KEY_REF_ID].asString());
  data = row2json(rows[0]);
  return data;
}

Json::Value NoticeService::list(const std::string& level, int page, long pageSize) {
  if (!level.empty() && level != notice::LEVEL_SYSTEM) {
    throw std::runtime_error(notice::ERR_LEVEL);
  }
  const long offset = static_cast<long>(page - 1) * pageSize;
  const auto counted = Sql::sync({}, dbClient, 
      "SELECT count(*) AS total FROM notice WHERE ($1 = '' OR level = $1)", level);
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + SELECT_ROW +
          "FROM notice WHERE ($1 = '' OR level = $1) ORDER BY created_at DESC, id DESC LIMIT $2 OFFSET $3",
      level,
      pageSize,
      offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(row2json(row));
  }
  Json::Value data;
  data[notice::KEY_ITEMS] = items;
  data[notice::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[notice::KEY_PAGE] = page;
  data[notice::KEY_PAGE_SIZE] = pageSize;
  return data;
}

Json::Value NoticeService::remove(const std::string& noticeId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("DELETE FROM notice WHERE id = $1::uuid RETURNING ") + SELECT_ROW, noticeId);
  if (rows.empty()) {
    throw std::runtime_error(notice::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value NoticeService::count() {
  const auto systemRows = Sql::sync({}, dbClient, 
      "SELECT count(*) AS total FROM notice WHERE read_at IS NULL AND level = $1", notice::LEVEL_SYSTEM);
  const auto eventRows = Sql::sync({}, dbClient, 
      "SELECT count(*) AS total FROM event WHERE read_at IS NULL AND level = $1", event::LEVEL_ALERT);
  const long system = systemRows[0]["total"].as<long>();
  const long event = eventRows[0]["total"].as<long>();
  Json::Value data;
  data[notice::KEY_TOTAL] = static_cast<Json::Int64>(system + event);
  data[notice::KEY_EVENT] = static_cast<Json::Int64>(event);
  data[notice::KEY_SYSTEM] = static_cast<Json::Int64>(system);
  return data;
}

Json::Value NoticeService::setRead(const std::string& noticeId, bool read, const std::string& userId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("UPDATE notice SET read_at = CASE WHEN $2::boolean THEN now() ELSE NULL END, "
                  "read_by = CASE WHEN $2::boolean THEN NULLIF($3, '')::uuid ELSE NULL END "
                  "WHERE id = $1::uuid RETURNING ") +
          SELECT_ROW,
      noticeId,
      read,
      userId);
  if (rows.empty()) {
    throw std::runtime_error(notice::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value NoticeService::row2json(const drogon::orm::Row& row) {
  Json::Value data = notice::ROW();
  data[notice::KEY_ID] = row["id"].as<std::string>();
  data[notice::KEY_CREATED_AT] = row["created_at"].as<std::string>();
  data[notice::KEY_READ_AT] = row["read_at"].isNull() ? Json::Value() : row["read_at"].as<std::string>();
  data[notice::KEY_READ_BY] = row["read_by"].isNull() ? Json::Value() : row["read_by"].as<std::string>();
  data[notice::KEY_LEVEL] = row["level"].as<std::string>();
  data[notice::KEY_SOURCE] = row["source"].as<std::string>();
  data[notice::KEY_TEXT] = row["text"].as<std::string>();
  data[notice::KEY_REF_ID] = row["ref_id"].isNull() ? Json::Value() : row["ref_id"].as<std::string>();
  return data;
}
