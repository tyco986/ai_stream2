#include "services/notice_service.hpp"

#include "constants/notice_const.hpp"

#include <stdexcept>
#include <string>

namespace {

const char* SELECT_ROW =
    "id::text AS id, created_at, read_at, user_id::text AS user_id, level, source, text, "
    "ref_id::text AS ref_id ";

const char* OWNED = "(user_id = $1::uuid OR user_id IS NULL) ";

}

NoticeService::NoticeService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value NoticeService::create(const Json::Value& createdAt,
                                  const Json::Value& userId,
                                  const Json::Value& level,
                                  const Json::Value& source,
                                  const Json::Value& text,
                                  const Json::Value& refId) {
  Json::Value data = notice::ROW();
  data[notice::KEY_CREATED_AT] = createdAt.isNull() ? data[notice::KEY_CREATED_AT] : createdAt;
  data[notice::KEY_USER_ID] = userId.isNull() ? data[notice::KEY_USER_ID] : userId;
  data[notice::KEY_LEVEL] = level.isNull() ? data[notice::KEY_LEVEL] : level;
  data[notice::KEY_SOURCE] = source.isNull() ? data[notice::KEY_SOURCE] : source;
  data[notice::KEY_TEXT] = text.isNull() ? data[notice::KEY_TEXT] : text;
  data[notice::KEY_REF_ID] = refId.isNull() ? data[notice::KEY_REF_ID] : refId;
  const auto rows = dbClient->execSqlSync(
      std::string("INSERT INTO notice (id, created_at, user_id, level, source, text, ref_id) "
                  "VALUES (gen_random_uuid(), $1::timestamptz, NULLIF($2, '')::uuid, $3, $4, $5, "
                  "NULLIF($6, '')::uuid) RETURNING ") +
          SELECT_ROW,
      data[notice::KEY_CREATED_AT].isNull() ? "" : data[notice::KEY_CREATED_AT].asString(),
      data[notice::KEY_USER_ID].isNull() ? "" : data[notice::KEY_USER_ID].asString(),
      data[notice::KEY_LEVEL].isNull() ? "" : data[notice::KEY_LEVEL].asString(),
      data[notice::KEY_SOURCE].isNull() ? "" : data[notice::KEY_SOURCE].asString(),
      data[notice::KEY_TEXT].isNull() ? "" : data[notice::KEY_TEXT].asString(),
      data[notice::KEY_REF_ID].isNull() ? "" : data[notice::KEY_REF_ID].asString());
  data = row2json(rows[0]);
  return data;
}

std::string NoticeService::token2UserId(const std::string& token) {
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id FROM \"user\" WHERE token = $1 AND active", token);
  if (rows.empty()) {
    throw std::runtime_error(notice::ERR_NOT_FOUND);
  }
  const std::string id = rows[0]["id"].as<std::string>();
  return id;
}

Json::Value NoticeService::list(const std::string& userId, int page, int pageSize) {
  const int offset = (page - 1) * pageSize;
  const std::string from = std::string("FROM notice WHERE ") + OWNED;
  const auto counted = dbClient->execSqlSync("SELECT count(*) AS total " + from, userId);
  const auto rows = dbClient->execSqlSync(
      std::string("SELECT ") + SELECT_ROW + from + "ORDER BY created_at DESC, id DESC LIMIT $2 OFFSET $3",
      userId,
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
  const auto rows = dbClient->execSqlSync(
      std::string("DELETE FROM notice WHERE id = $1::uuid RETURNING ") + SELECT_ROW, noticeId);
  if (rows.empty()) {
    throw std::runtime_error(notice::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

long NoticeService::countUnread(const std::string& userId) {
  const auto rows = dbClient->execSqlSync(
      std::string("SELECT count(*) AS total FROM notice WHERE read_at IS NULL AND ") + OWNED, userId);
  const long total = rows[0]["total"].as<long>();
  return total;
}

Json::Value NoticeService::setReadAt(const std::string& noticeId, const std::string& readAt) {
  const auto rows = dbClient->execSqlSync(
      std::string("UPDATE notice SET read_at = $2::timestamptz WHERE id = $1::uuid RETURNING ") + SELECT_ROW,
      noticeId,
      readAt);
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
  data[notice::KEY_USER_ID] = row["user_id"].isNull() ? Json::Value() : row["user_id"].as<std::string>();
  data[notice::KEY_LEVEL] = row["level"].as<std::string>();
  data[notice::KEY_SOURCE] = row["source"].as<std::string>();
  data[notice::KEY_TEXT] = row["text"].as<std::string>();
  data[notice::KEY_REF_ID] = row["ref_id"].isNull() ? Json::Value() : row["ref_id"].as<std::string>();
  return data;
}
