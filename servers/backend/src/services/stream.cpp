#include "services/stream.hpp"

#include <optional>
#include <stdexcept>
#include <vector>

#include "constants/stream.hpp"
#include "constants/stream_group.hpp"
#include "searcher/searcher.hpp"

StreamService::StreamService(drogon::orm::DbClientPtr dbClient) {
  this->dbClient = std::move(dbClient);
}

Json::Value StreamService::list(const std::string& groupId,
                                const std::string& streamId,
                                const std::string& search,
                                int page,
                                int pageSize) {
  const int offset = (page - 1) * pageSize;
  const bool byStream = !streamId.empty();
  const std::string columns =
      "stream.id::text AS id, stream.name, stream.group_id::text AS group_id, "
      "stream_group.name AS group_name, stream.url, stream.width, stream.height, stream.fps, "
      "stream.status, stream.enabled, stream.recording, stream.probed_at ";
  const std::string match = Searcher::clause(
      {"stream.id",
       "stream.name",
       "stream.group_id",
       "stream_group.name",
       "stream.url",
       "stream.width",
       "stream.height",
       "stream.fps",
       "stream.status",
       "stream.enabled",
       "stream.recording",
       "stream.probed_at"},
      2);
  const std::string byStreamFrom =
      "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream.id = $1::uuid AND " +
      match + " ";
  const auto counted = byStream
      ? dbClient->execSqlSync("SELECT count(*) AS total " + byStreamFrom, streamId, search)
      : dbClient->execSqlSync(
            "WITH RECURSIVE subtree AS ("
            "SELECT id FROM stream_group WHERE id = $1::uuid "
            "UNION ALL "
            "SELECT child.id FROM stream_group child INNER JOIN subtree ON child.parent_id = subtree.id"
            ") SELECT count(*) AS total FROM stream "
            "INNER JOIN stream_group ON stream_group.id = stream.group_id "
            "WHERE stream.group_id IN (SELECT id FROM subtree) AND " +
                match,
            groupId.empty() ? stream_group::ALL_GROUP_ID : groupId,
            search);
  const auto rows = byStream
      ? dbClient->execSqlSync(
            "SELECT " + columns + byStreamFrom + "ORDER BY stream.name LIMIT $3 OFFSET $4",
            streamId,
            search,
            pageSize,
            offset)
      : dbClient->execSqlSync(
            "WITH RECURSIVE subtree AS ("
            "SELECT id FROM stream_group WHERE id = $1::uuid "
            "UNION ALL "
            "SELECT child.id FROM stream_group child INNER JOIN subtree ON child.parent_id = subtree.id"
            ") SELECT " +
                columns +
                "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
                "WHERE stream.group_id IN (SELECT id FROM subtree) AND " + match + " "
                "ORDER BY stream.name LIMIT $3 OFFSET $4",
            groupId.empty() ? stream_group::ALL_GROUP_ID : groupId,
            search,
            pageSize,
            offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(item(row));
  }
  Json::Value data;
  data[stream::KEY_ITEMS] = items;
  data[stream::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[stream::KEY_PAGE] = page;
  data[stream::KEY_PAGE_SIZE] = pageSize;
  return data;
}

Json::Value StreamService::create(drogon::orm::DbClient& tx,
                                  const std::string& name,
                                  const std::string& groupId,
                                  const std::string& url,
                                  bool enabled,
                                  bool recording) {
  const auto rows = tx.execSqlSync(
      "WITH inserted AS ("
      "INSERT INTO stream (id, name, group_id, url, status, enabled, recording) "
      "SELECT gen_random_uuid(), $1, stream_group.id, $2, $3, $4, $5 "
      "FROM stream_group WHERE stream_group.id = $6::uuid "
      "RETURNING id, name, group_id, url, width, height, fps, status, enabled, recording, probed_at"
      ") SELECT inserted.id::text AS id, inserted.name, inserted.group_id::text AS group_id, "
      "stream_group.name AS group_name, inserted.url, inserted.width, inserted.height, inserted.fps, "
      "inserted.status, inserted.enabled, inserted.recording, inserted.probed_at "
      "FROM inserted INNER JOIN stream_group ON stream_group.id = inserted.group_id",
      name,
      url,
      stream::STATUS_OFFLINE,
      enabled,
      recording,
      groupId);
  if (rows.empty()) {
    throw std::runtime_error("stream not created");
  }
  Json::Value data = item(rows[0]);
  return data;
}

Json::Value StreamService::get(const std::string& streamId) {
  const auto rows = dbClient->execSqlSync(
      "SELECT stream.id::text AS id, stream.name, stream.group_id::text AS group_id, "
      "stream_group.name AS group_name, stream.url, stream.width, stream.height, stream.fps, "
      "stream.status, stream.enabled, stream.recording, stream.probed_at "
      "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream.id = $1::uuid",
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not found");
  }
  Json::Value data = item(rows[0]);
  return data;
}

Json::Value StreamService::update(drogon::orm::DbClient& tx,
                                  const std::string& streamId,
                                  const std::string& name,
                                  const std::string& groupId,
                                  const std::string& url,
                                  bool enabled,
                                  bool recording) {
  const auto rows = tx.execSqlSync(
      "WITH updated AS ("
      "UPDATE stream SET name = $1, group_id = stream_group.id, url = $2, enabled = $3, recording = $4 "
      "FROM stream_group WHERE stream.id = $5::uuid AND stream_group.id = $6::uuid "
      "RETURNING stream.id, stream.name, stream.group_id, stream.url, stream.width, stream.height, "
      "stream.fps, stream.status, stream.enabled, stream.recording, stream.probed_at"
      ") SELECT updated.id::text AS id, updated.name, updated.group_id::text AS group_id, "
      "stream_group.name AS group_name, updated.url, updated.width, updated.height, updated.fps, "
      "updated.status, updated.enabled, updated.recording, updated.probed_at "
      "FROM updated INNER JOIN stream_group ON stream_group.id = updated.group_id",
      name,
      url,
      enabled,
      recording,
      streamId,
      groupId);
  if (rows.empty()) {
    throw std::runtime_error("stream not updated");
  }
  Json::Value data = item(rows[0]);
  return data;
}

Json::Value StreamService::probe(const std::string& streamId, const Json::Value& result) {
  const Json::Value widthNode = result[stream::KEY_WIDTH];
  const Json::Value heightNode = result[stream::KEY_HEIGHT];
  const Json::Value fpsNode = result[stream::KEY_FPS];
  const Json::Value probedAtNode = result[stream::KEY_PROBED_AT];
  const std::optional<int> width = widthNode.isNull() ? std::nullopt : std::optional<int>(widthNode.asInt());
  const std::optional<int> height = heightNode.isNull() ? std::nullopt : std::optional<int>(heightNode.asInt());
  const std::optional<double> fps = fpsNode.isNull() ? std::nullopt : std::optional<double>(fpsNode.asDouble());
  const std::optional<std::string> probedAt =
      probedAtNode.isNull() ? std::nullopt : std::optional<std::string>(probedAtNode.asString());
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream SET status = $1, width = $2, height = $3, fps = $4, probed_at = $5::timestamptz "
      "WHERE id = $6::uuid "
      "RETURNING id::text AS id, status, width, height, fps, probed_at",
      result[stream::KEY_STATUS].asString(),
      width,
      height,
      fps,
      probedAt,
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not probed");
  }
  Json::Value data = probeItem(rows[0]);
  return data;
}

void StreamService::enable(drogon::orm::DbClient& tx, const std::string& streamId) {
  const auto rows = tx.execSqlSync(
      "UPDATE stream SET enabled = true WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not enabled");
  }
}

void StreamService::disable(drogon::orm::DbClient& tx, const std::string& streamId) {
  const auto rows = tx.execSqlSync(
      "UPDATE stream SET enabled = false WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not disabled");
  }
}

void StreamService::record(drogon::orm::DbClient& tx, const std::string& streamId) {
  const auto rows = tx.execSqlSync(
      "UPDATE stream SET recording = true WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not recorded");
  }
}

void StreamService::unrecord(drogon::orm::DbClient& tx, const std::string& streamId) {
  const auto rows = tx.execSqlSync(
      "UPDATE stream SET recording = false WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not unrecorded");
  }
}

void StreamService::remove(const std::string& streamId) {
  const auto rows = dbClient->execSqlSync("DELETE FROM stream WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not deleted");
  }
}

void StreamService::batchRemove(const std::vector<std::string>& streamIds) {
  for (const auto& streamId : streamIds) {
    remove(streamId);
  }
}

Json::Value StreamService::probeItem(const drogon::orm::Row& row) {
  Json::Value node;
  node[stream::KEY_ID] = row["id"].as<std::string>();
  node[stream::KEY_STATUS] = row["status"].as<std::string>();
  node[stream::KEY_WIDTH] = row["width"].isNull() ? Json::Value() : row["width"].as<int>();
  node[stream::KEY_HEIGHT] = row["height"].isNull() ? Json::Value() : row["height"].as<int>();
  node[stream::KEY_FPS] = row["fps"].isNull() ? Json::Value() : row["fps"].as<double>();
  node[stream::KEY_PROBED_AT] =
      row["probed_at"].isNull() ? Json::Value() : row["probed_at"].as<std::string>();
  return node;
}

Json::Value StreamService::item(const drogon::orm::Row& row) {
  Json::Value node;
  node[stream::KEY_ID] = row["id"].as<std::string>();
  node[stream::KEY_NAME] = row["name"].as<std::string>();
  node[stream::KEY_GROUP_ID] = row["group_id"].as<std::string>();
  node[stream::KEY_GROUP_NAME] = row["group_name"].as<std::string>();
  node[stream::KEY_URL] = row["url"].as<std::string>();
  node[stream::KEY_WIDTH] = row["width"].isNull() ? Json::Value() : row["width"].as<int>();
  node[stream::KEY_HEIGHT] = row["height"].isNull() ? Json::Value() : row["height"].as<int>();
  node[stream::KEY_FPS] = row["fps"].isNull() ? Json::Value() : row["fps"].as<double>();
  node[stream::KEY_STATUS] = row["status"].as<std::string>();
  node[stream::KEY_ENABLED] = row["enabled"].as<bool>();
  node[stream::KEY_RECORDING] = row["recording"].as<bool>();
  node[stream::KEY_PROBED_AT] =
      row["probed_at"].isNull() ? Json::Value() : row["probed_at"].as<std::string>();
  return node;
}
