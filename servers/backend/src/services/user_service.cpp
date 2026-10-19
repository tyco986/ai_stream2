#include "services/user_service.hpp"

#include "constants/user_const.hpp"
#include "searcher/searcher.hpp"

#include <stdexcept>
#include <utility>

UserService::UserService(drogon::orm::DbClientPtr dbClient) {
  this->dbClient = std::move(dbClient);
}

Json::Value UserService::row2json(const drogon::orm::Row& row) {
  Json::Value data;
  data[user::KEY_ID] = row["id"].as<std::string>();
  data[user::KEY_USERNAME] = row["username"].as<std::string>();
  data[user::KEY_PASSWD] = row["passwd"].as<std::string>();
  data[user::KEY_ACTIVE] = row["active"].as<bool>();
  data[user::KEY_GROUP_ID] = row["group_id"].isNull() ? Json::Value() : Json::Value(row["group_id"].as<std::string>());
  data[user::KEY_GROUP_NAME] =
      row["group_name"].isNull() ? Json::Value() : Json::Value(row["group_name"].as<std::string>());
  data[user::KEY_LAST_LOGIN] =
      row["last_login"].isNull() ? Json::Value() : Json::Value(row["last_login"].as<std::string>());
  return data;
}

Json::Value UserService::login(const Json::Value& username, const Json::Value& passwd) {
  const auto rows = dbClient->execSqlSync(
      "UPDATE \"user\" SET token = gen_random_uuid()::text, last_login = now() "
      "WHERE username = $1 AND passwd = $2 AND active RETURNING token",
      username.asString(),
      passwd.asString());
  if (rows.empty()) {
    throw std::runtime_error(user::ERR_LOGIN);
  }
  Json::Value data;
  data[user::KEY_TOKEN] = rows[0]["token"].as<std::string>();
  return data;
}

Json::Value UserService::list(const std::string& search, int page, int pageSize) {
  const int offset = (page - 1) * pageSize;
  const std::string match = Searcher::clause(
      {"\"user\".id", "\"user\".username", "\"user\".active", "\"user\".group_id", "user_group.name"}, 1);
  const std::string from = std::string(user::SQL_USER_FROM) + " WHERE " + match + " ";
  const auto counted = dbClient->execSqlSync("SELECT count(*) AS total " + from, search);
  const auto rows = dbClient->execSqlSync(
      "SELECT " + std::string(user::SQL_USER) + " " + from + "ORDER BY \"user\".username LIMIT $2 OFFSET $3",
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

Json::Value UserService::get(const std::string& userId) {
  const auto rows = dbClient->execSqlSync(
      std::string("SELECT ") + user::SQL_USER + " " + user::SQL_USER_FROM + " WHERE \"user\".id = $1::uuid",
      userId);
  if (rows.empty()) {
    throw std::runtime_error(user::ERR_USER_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value UserService::map() {
  const auto rows = dbClient->execSqlSync("SELECT id::text AS id, username FROM \"user\" ORDER BY username");
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["username"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value UserService::create(const Json::Value& username,
                                const Json::Value& passwd,
                                const Json::Value& active,
                                const Json::Value& groupId) {
  const std::string group = groupId.isString() ? groupId.asString() : "";
  std::string id;
  {
    auto transaction = dbClient->newTransaction();
    const auto rows = transaction->execSqlSync(
        "INSERT INTO \"user\" (id, username, passwd, active, group_id) "
        "VALUES (gen_random_uuid(), $1, $2, $3, NULLIF($4, '')::uuid) RETURNING id::text AS id",
        username.asString(),
        passwd.asString(),
        active.asBool(),
        group);
    id = rows[0]["id"].as<std::string>();
    transaction->execSqlSync(
        "INSERT INTO preview_preset (id, user_id, name, layout, mode, slots, active) "
        "VALUES (gen_random_uuid(), $1::uuid, $2, '2x2', 'grid', "
        "'{\"1\":null,\"2\":null,\"3\":null,\"4\":null}'::jsonb, TRUE)",
        id,
        std::string(user::PRESET_NAME_DEFAULT));
  }
  Json::Value data = get(id);
  return data;
}

Json::Value UserService::update(const std::string& userId,
                                const Json::Value& username,
                                const Json::Value& passwd,
                                const Json::Value& active,
                                const Json::Value& groupId) {
  const Json::Value current = get(userId);
  const bool admin = userId == user::ADMIN_USER_ID;
  const std::string nextGroup = groupId.isString() ? groupId.asString() : "";
  const std::string currentGroup = current[user::KEY_GROUP_ID].isString() ? current[user::KEY_GROUP_ID].asString() : "";
  const bool locked = admin && (username.asString() != current[user::KEY_USERNAME].asString() ||
                                active.asBool() != current[user::KEY_ACTIVE].asBool() || nextGroup != currentGroup);
  if (locked) {
    throw std::runtime_error(user::ERR_USER_LOCKED);
  }
  dbClient->execSqlSync(
      "UPDATE \"user\" SET username = $2, passwd = $3, active = $4, group_id = NULLIF($5, '')::uuid "
      "WHERE id = $1::uuid",
      userId,
      admin ? current[user::KEY_USERNAME].asString() : username.asString(),
      passwd.asString(),
      admin ? current[user::KEY_ACTIVE].asBool() : active.asBool(),
      admin ? currentGroup : nextGroup);
  Json::Value data = get(userId);
  return data;
}

Json::Value UserService::remove(const std::string& userId) {
  if (userId == user::ADMIN_USER_ID) {
    throw std::runtime_error(user::ERR_USER_LOCKED);
  }
  const Json::Value data = get(userId);
  auto transaction = dbClient->newTransaction();
  transaction->execSqlSync("DELETE FROM preview_preset WHERE user_id = $1::uuid", userId);
  transaction->execSqlSync("DELETE FROM \"user\" WHERE id = $1::uuid", userId);
  return data;
}
