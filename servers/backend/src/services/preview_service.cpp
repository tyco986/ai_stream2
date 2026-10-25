#include "services/preview_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/preview_const.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

PreviewService::PreviewService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

std::string PreviewService::token2UserId(const std::string& token) {
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id FROM \"user\" WHERE token = $1 AND active", token);
  if (rows.empty()) {
    throw std::runtime_error(preview::ERR_NOT_FOUND);
  }
  const std::string id = rows[0]["id"].as<std::string>();
  return id;
}

Json::Value PreviewService::get(const std::string& presetId, const std::string& userId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + preview::SQL_COLUMNS +
          " FROM preview_preset WHERE id = $1::uuid AND user_id = $2::uuid",
      presetId,
      userId);
  if (rows.empty()) {
    throw std::runtime_error(preview::ERR_NOT_FOUND);
  }
  Json::Value data = parseSlots(row2json(rows[0]));
  return data;
}

Json::Value PreviewService::map(const std::string& userId) {
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name FROM preview_preset WHERE user_id = $1::uuid ORDER BY name", userId);
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value PreviewService::tree() {
  const auto groupRows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name, parent_id::text AS parent_id FROM stream_group ORDER BY name");
  const auto streamRows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name, group_id::text AS group_id, mount, status, record FROM stream ORDER BY name");
  Json::Value root;
  std::unordered_map<std::string, std::vector<Json::Value>> children;
  for (const auto& row : groupRows) {
    Json::Value group;
    group[preview::KEY_ID] = row["id"].as<std::string>();
    group[preview::KEY_NAME] = row["name"].as<std::string>();
    group[preview::KEY_TYPE] = preview::TYPE_GROUP;
    const std::string parentId = row["parent_id"].isNull() ? "" : row["parent_id"].as<std::string>();
    if (parentId.empty()) {
      root = group;
    } else {
      children[parentId].push_back(group);
    }
  }
  std::unordered_map<std::string, std::vector<Json::Value>> streams;
  for (const auto& row : streamRows) {
    Json::Value stream;
    stream[preview::KEY_ID] = row["id"].as<std::string>();
    stream[preview::KEY_NAME] = row["name"].as<std::string>();
    stream[preview::KEY_TYPE] = preview::TYPE_STREAM;
    stream[preview::KEY_MOUNT] = row["mount"].as<bool>();
    stream[preview::KEY_STATUS] = row["status"].as<std::string>();
    stream[preview::KEY_RECORD] = row["record"].as<bool>();
    streams[row["group_id"].as<std::string>()].push_back(stream);
  }
  Json::Value data = groupNode(root, children, streams);
  return data;
}

Json::Value PreviewService::create(const std::string& userId,
                                   const Json::Value& name,
                                   const Json::Value& layout,
                                   const Json::Value& mode,
                                   const Json::Value& slots,
                                   const Json::Value& active) {
  const std::string text = slots2Text(layout.asString(), slots);
  auto transaction = dbClient->newTransaction();
  if (active.asBool()) {
    Sql::sync({}, transaction, 
        "UPDATE preview_preset SET active = false WHERE user_id = $1::uuid AND active", userId);
  }
  const auto rows = Sql::sync({}, transaction, 
      std::string("INSERT INTO preview_preset (id, user_id, name, layout, mode, slots, active) "
                  "VALUES (gen_random_uuid(), $1::uuid, $2, $3, $4, $5::jsonb, $6) RETURNING ") +
          preview::SQL_COLUMNS,
      userId,
      name.asString(),
      layout.asString(),
      mode.asString(),
      text,
      active.asBool());
  Json::Value data = parseSlots(row2json(rows[0]));
  return data;
}

Json::Value PreviewService::update(const std::string& presetId,
                                   const std::string& userId,
                                   const Json::Value& name,
                                   const Json::Value& layout,
                                   const Json::Value& mode,
                                   const Json::Value& slots,
                                   const Json::Value& active) {
  const std::string text = slots2Text(layout.asString(), slots);
  auto transaction = dbClient->newTransaction();
  if (active.asBool()) {
    Sql::sync({}, transaction, 
        "UPDATE preview_preset SET active = false WHERE user_id = $1::uuid AND active", userId);
  }
  const auto rows = Sql::sync({}, transaction, 
      std::string("UPDATE preview_preset SET name = $3, layout = $4, mode = $5, slots = $6::jsonb, active = $7 "
                  "WHERE id = $1::uuid AND user_id = $2::uuid RETURNING ") +
          preview::SQL_COLUMNS,
      presetId,
      userId,
      name.asString(),
      layout.asString(),
      mode.asString(),
      text,
      active.asBool());
  if (rows.empty()) {
    throw std::runtime_error(preview::ERR_NOT_FOUND);
  }
  Json::Value data = parseSlots(row2json(rows[0]));
  return data;
}

Json::Value PreviewService::remove(const std::string& presetId, const std::string& userId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + preview::SQL_COLUMNS +
          " FROM preview_preset WHERE id = $1::uuid AND user_id = $2::uuid",
      presetId,
      userId);
  if (rows.empty()) {
    throw std::runtime_error(preview::ERR_NOT_FOUND);
  }
  if (rows[0]["name"].as<std::string>() == preview::NAME_DEFAULT) {
    throw std::runtime_error(preview::ERR_DEFAULT);
  }
  Sql::sync({}, dbClient, "DELETE FROM preview_preset WHERE id = $1::uuid AND user_id = $2::uuid", presetId, userId);
  Json::Value data = parseSlots(row2json(rows[0]));
  return data;
}

Json::Value PreviewService::getActive(const std::string& userId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT ") + preview::SQL_COLUMNS +
          " FROM preview_preset WHERE user_id = $1::uuid AND active",
      userId);
  if (rows.empty()) {
    throw std::runtime_error(preview::ERR_NOT_FOUND);
  }
  Json::Value data = parseSlots(row2json(rows[0]));
  return data;
}

std::string PreviewService::slots2Text(const std::string& layout, const Json::Value& slots) {
  const bool listed =
      std::find(preview::AVAILABLE_LAYOUT.begin(), preview::AVAILABLE_LAYOUT.end(), layout) !=
      preview::AVAILABLE_LAYOUT.end();
  if (!listed) {
    throw std::runtime_error(preview::ERR_LAYOUT);
  }
  const int count = preview::LAYOUT_SLOT_COUNT.at(layout);
  const bool shape = slots.isObject() && static_cast<int>(slots.size()) == count;
  if (!shape) {
    throw std::runtime_error(preview::ERR_SLOTS);
  }
  for (int index = 1; index <= count; ++index) {
    const std::string key = std::to_string(index);
    const bool slot = slots.isMember(key) && (slots[key].isNull() || slots[key].isString());
    if (!slot) {
      throw std::runtime_error(preview::ERR_SLOTS);
    }
  }
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const std::string text = Json::writeString(builder, slots);
  return text;
}

Json::Value PreviewService::parseSlots(Json::Value data) {
  const Json::Value slots = data[preview::KEY_SLOTS];
  std::vector<std::string> ids;
  for (const auto& key : slots.getMemberNames()) {
    if (slots[key].isString()) {
      ids.push_back(slots[key].asString());
    }
  }
  Json::Value streams(Json::objectValue);
  if (!ids.empty()) {
    std::string list = "{";
    for (const auto& id : ids) {
      list += list.size() == 1 ? id : "," + id;
    }
    list += "}";
    const auto rows = Sql::sync({}, dbClient, 
        std::string("SELECT ") + preview::SQL_STREAM +
            "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
            "WHERE stream.id = ANY($1::uuid[])",
        list);
    for (const auto& row : rows) {
      Json::Value node;
      node[preview::KEY_ID] = row["id"].as<std::string>();
      node[preview::KEY_NAME] = row["name"].as<std::string>();
      node[preview::KEY_GROUP_ID] = row["group_id"].as<std::string>();
      node[preview::KEY_GROUP_NAME] = row["group_name"].as<std::string>();
      node[preview::KEY_URL] = row["url"].as<std::string>();
      node[preview::KEY_WIDTH] = row["width"].isNull() ? Json::Value() : row["width"].as<int>();
      node[preview::KEY_HEIGHT] = row["height"].isNull() ? Json::Value() : row["height"].as<int>();
      node[preview::KEY_FPS] = row["fps"].isNull() ? Json::Value() : row["fps"].as<double>();
      node[preview::KEY_STATUS] = row["status"].as<std::string>();
      node[preview::KEY_MOUNT] = row["mount"].as<bool>();
      node[preview::KEY_RECORD] = row["record"].as<bool>();
      node[preview::KEY_PROBED_AT] = row["probed_at"].isNull() ? Json::Value() : row["probed_at"].as<std::string>();
      streams[row["id"].as<std::string>()] = node;
    }
  }
  Json::Value expanded(Json::objectValue);
  for (const auto& key : slots.getMemberNames()) {
    const bool bound = slots[key].isString() && streams.isMember(slots[key].asString());
    expanded[key] = bound ? streams[slots[key].asString()] : Json::Value();
  }
  data[preview::KEY_SLOTS] = expanded;
  return data;
}

Json::Value PreviewService::row2json(const drogon::orm::Row& row) {
  Json::Value slots;
  Json::Reader().parse(row["slots"].as<std::string>(), slots);
  Json::Value data = preview::ROW();
  data[preview::KEY_ID] = row["id"].as<std::string>();
  data[preview::KEY_USER_ID] = row["user_id"].as<std::string>();
  data[preview::KEY_NAME] = row["name"].as<std::string>();
  data[preview::KEY_LAYOUT] = row["layout"].as<std::string>();
  data[preview::KEY_MODE] = row["mode"].as<std::string>();
  data[preview::KEY_SLOTS] = slots;
  data[preview::KEY_ACTIVE] = row["active"].as<bool>();
  return data;
}

Json::Value PreviewService::groupNode(const Json::Value& group,
                                      const std::unordered_map<std::string, std::vector<Json::Value>>& children,
                                      const std::unordered_map<std::string, std::vector<Json::Value>>& streams) {
  Json::Value node = group;
  Json::Value kids(Json::arrayValue);
  const auto streamIt = streams.find(group[preview::KEY_ID].asString());
  if (streamIt != streams.end()) {
    for (const auto& row : streamIt->second) {
      kids.append(row);
    }
  }
  const auto childIt = children.find(group[preview::KEY_ID].asString());
  if (childIt != children.end()) {
    for (const auto& child : childIt->second) {
      kids.append(groupNode(child, children, streams));
    }
  }
  node[preview::KEY_CHILDREN] = kids;
  return node;
}
