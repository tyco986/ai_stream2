#include "services/user_group_service.hpp"

#include "constants/user_const.hpp"
#include "searcher/searcher.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

UserGroupService::UserGroupService(drogon::orm::DbClientPtr dbClient) {
  this->dbClient = std::move(dbClient);
}

Json::Value UserGroupService::row2json(const drogon::orm::Row& row) {
  Json::Value data = user::ROW();
  data[user::KEY_ID] = row["id"].as<std::string>();
  data[user::KEY_NAME] = row["name"].as<std::string>();
  data[user::KEY_STREAM_CRUD] = row["stream_crud"].as<std::string>();
  data[user::KEY_MODEL_CRUD] = row["model_crud"].as<std::string>();
  data[user::KEY_PREVIEW_CRUD] = row["preview_crud"].as<std::string>();
  data[user::KEY_PIPELINE_CRUD] = row["pipeline_crud"].as<std::string>();
  data[user::KEY_RECORDING_CRUD] = row["recording_crud"].as<std::string>();
  data[user::KEY_SERVER_CRUD] = row["server_crud"].as<std::string>();
  data[user::KEY_USER_CRUD] = row["user_crud"].as<std::string>();
  data[user::KEY_EVENT_CRUD] = row["event_crud"].as<std::string>();
  return data;
}

Json::Value UserGroupService::list(const std::string& search, int page, int pageSize) {
  const int offset = (page - 1) * pageSize;
  const std::string match = Searcher::clause(
      {"id", "name", "stream_crud", "model_crud", "preview_crud", "pipeline_crud", "recording_crud", "server_crud",
       "user_crud", "event_crud"},
      1);
  const std::string from = "FROM user_group WHERE " + match + " ";
  const auto counted = dbClient->execSqlSync("SELECT count(*) AS total " + from, search);
  const auto rows = dbClient->execSqlSync(
      "SELECT " + std::string(user::SQL_ROW) + " " + from + "ORDER BY name LIMIT $2 OFFSET $3",
      search,
      pageSize,
      offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(row2json(row));
  }
  Json::Value data;
  data[user::KEY_ITEMS] = items;
  data[user::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[user::KEY_PAGE] = page;
  data[user::KEY_PAGE_SIZE] = pageSize;
  return data;
}

Json::Value UserGroupService::get(const std::string& groupId) {
  const auto rows = dbClient->execSqlSync(
      std::string("SELECT ") + user::SQL_ROW + " FROM user_group WHERE id = $1::uuid",
      groupId);
  if (rows.empty()) {
    throw std::runtime_error(user::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value UserGroupService::map() {
  const auto rows = dbClient->execSqlSync("SELECT id::text AS id, name FROM user_group ORDER BY name");
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value UserGroupService::create(const Json::Value& name,
                                     const Json::Value& streamCrud,
                                     const Json::Value& modelCrud,
                                     const Json::Value& previewCrud,
                                     const Json::Value& pipelineCrud,
                                     const Json::Value& recordingCrud,
                                     const Json::Value& serverCrud,
                                     const Json::Value& userCrud,
                                     const Json::Value& eventCrud) {
  const auto rows = dbClient->execSqlSync(
      std::string("INSERT INTO user_group (id, name, stream_crud, model_crud, preview_crud, pipeline_crud, "
                  "recording_crud, server_crud, user_crud, event_crud) "
                  "VALUES (gen_random_uuid(), $1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING ") +
          user::SQL_ROW,
      name.asString(),
      streamCrud.asString(),
      modelCrud.asString(),
      previewCrud.asString(),
      pipelineCrud.asString(),
      recordingCrud.asString(),
      serverCrud.asString(),
      userCrud.asString(),
      eventCrud.asString());
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value UserGroupService::update(const std::string& groupId,
                                     const Json::Value& name,
                                     const Json::Value& streamCrud,
                                     const Json::Value& modelCrud,
                                     const Json::Value& previewCrud,
                                     const Json::Value& pipelineCrud,
                                     const Json::Value& recordingCrud,
                                     const Json::Value& serverCrud,
                                     const Json::Value& userCrud,
                                     const Json::Value& eventCrud) {
  if (groupId == user::ADMIN_GROUP_ID) {
    throw std::runtime_error(user::ERR_GROUP_LOCKED);
  }
  const auto rows = dbClient->execSqlSync(
      std::string("UPDATE user_group SET name = $1, stream_crud = $2, model_crud = $3, preview_crud = $4, "
                  "pipeline_crud = $5, recording_crud = $6, server_crud = $7, user_crud = $8, event_crud = $9 "
                  "WHERE id = $10::uuid RETURNING ") +
          user::SQL_ROW,
      name.asString(),
      streamCrud.asString(),
      modelCrud.asString(),
      previewCrud.asString(),
      pipelineCrud.asString(),
      recordingCrud.asString(),
      serverCrud.asString(),
      userCrud.asString(),
      eventCrud.asString(),
      groupId);
  if (rows.empty()) {
    throw std::runtime_error(user::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value UserGroupService::members(const std::string& groupId) {
  const auto groups = dbClient->execSqlSync("SELECT id FROM user_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error(user::ERR_NOT_FOUND);
  }
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, username AS name FROM \"user\" WHERE group_id = $1::uuid ORDER BY username",
      groupId);
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value UserGroupService::candidates(const std::string& groupId) {
  const auto groups = dbClient->execSqlSync("SELECT id FROM user_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error(user::ERR_NOT_FOUND);
  }
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, username AS name FROM \"user\" "
      "WHERE group_id IS DISTINCT FROM $1::uuid ORDER BY username",
      groupId);
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value UserGroupService::setMembers(const std::string& groupId, const Json::Value& userIds) {
  const auto groups = dbClient->execSqlSync(
      "SELECT id FROM user_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error(user::ERR_NOT_FOUND);
  }
  std::unordered_set<std::string> selected;
  for (const auto& id : userIds) {
    selected.insert(id.asString());
  }
  const bool locksAdmin = (groupId == user::ADMIN_GROUP_ID) != (selected.count(user::ADMIN_USER_ID) > 0);
  if (locksAdmin) {
    throw std::runtime_error(user::ERR_USER_LOCKED);
  }
  {
    auto transaction = dbClient->newTransaction();
    for (const auto& id : selected) {
      transaction->execSqlSync(
          "UPDATE \"user\" SET group_id = $1::uuid WHERE id = $2::uuid",
          groupId,
          id);
    }
    const auto current = transaction->execSqlSync(
        "SELECT id::text AS id FROM \"user\" WHERE group_id = $1::uuid", groupId);
    for (const auto& row : current) {
      const auto id = row["id"].as<std::string>();
      if (selected.count(id) == 0) {
        transaction->execSqlSync(
            "UPDATE \"user\" SET group_id = NULL WHERE id = $1::uuid",
            id);
      }
    }
  }
  Json::Value data = members(groupId);
  return data;
}

Json::Value UserGroupService::remove(const std::string& groupId) {
  if (groupId == user::ADMIN_GROUP_ID) {
    throw std::runtime_error(user::ERR_GROUP_LOCKED);
  }
  const auto rows = dbClient->execSqlSync(
      std::string("SELECT ") + user::SQL_ROW + " FROM user_group WHERE id = $1::uuid",
      groupId);
  if (rows.empty()) {
    throw std::runtime_error(user::ERR_NOT_FOUND);
  }
  {
    auto transaction = dbClient->newTransaction();
    transaction->execSqlSync(
        "UPDATE \"user\" SET group_id = NULL WHERE group_id = $1::uuid",
        groupId);
    transaction->execSqlSync("DELETE FROM user_group WHERE id = $1::uuid", groupId);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}
