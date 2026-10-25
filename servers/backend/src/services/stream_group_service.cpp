#include "services/stream_group_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/stream_group_const.hpp"

#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

StreamGroupService::StreamGroupService(drogon::orm::DbClientPtr dbClient) {
  this->dbClient = std::move(dbClient);
}

Json::Value StreamGroupService::tree() {
  const auto groupRows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name, parent_id::text AS parent_id FROM stream_group ORDER BY name");
  const auto streamRows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name, group_id::text AS group_id, mount, status, record "
      "FROM stream ORDER BY name");

  Json::Value root;
  std::unordered_map<std::string, std::vector<Json::Value>> children;
  for (const auto& row : groupRows) {
    Json::Value group;
    group[stream_group::KEY_ID] = row["id"].as<std::string>();
    group[stream_group::KEY_NAME] = row["name"].as<std::string>();
    group[stream_group::KEY_TYPE] = stream_group::TYPE_GROUP;
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
    stream[stream_group::KEY_ID] = row["id"].as<std::string>();
    stream[stream_group::KEY_NAME] = row["name"].as<std::string>();
    stream[stream_group::KEY_TYPE] = stream_group::TYPE_STREAM;
    stream[stream_group::KEY_MOUNT] = row["mount"].as<bool>();
    stream[stream_group::KEY_STATUS] = row["status"].as<std::string>();
    stream[stream_group::KEY_RECORD] = row["record"].as<bool>();
    streams[row["group_id"].as<std::string>()].push_back(stream);
  }

  Json::Value data = groupNode(root, children, streams);
  return data;
}

Json::Value StreamGroupService::map() {
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name FROM stream_group ORDER BY name");
  Json::Value data;
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value StreamGroupService::importCSV(const std::string& csv) {
  std::istringstream input(csv);
  std::string line;
  const bool headed = static_cast<bool>(std::getline(input, line));
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  if (!headed || line != stream_group::CSV_HEADER) {
    throw std::runtime_error(stream_group::ERR_CSV);
  }
  std::vector<std::string> names;
  std::vector<std::string> parents;
  std::unordered_set<std::string> seen;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const bool blank = line.empty();
    const std::size_t comma = line.find(',');
    const bool columns = comma != std::string::npos && line.find(',', comma + 1) == std::string::npos;
    if (!blank && !columns) {
      throw std::runtime_error(stream_group::ERR_CSV);
    }
    if (!blank) {
      const std::string name = line.substr(0, comma);
      const std::string parent = line.substr(comma + 1);
      if (!seen.insert(name).second) {
        throw std::runtime_error(stream_group::ERR_GROUP_EXISTS);
      }
      names.push_back(name);
      parents.push_back(parent);
    }
  }
  std::unordered_map<std::string, std::string> idByName;
  std::unordered_map<std::string, std::string> parentById;
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name, parent_id::text AS parent_id FROM stream_group");
  for (const auto& row : rows) {
    const std::string id = row["id"].as<std::string>();
    idByName[row["name"].as<std::string>()] = id;
    parentById[id] = row["parent_id"].isNull() ? "" : row["parent_id"].as<std::string>();
  }
  std::vector<std::pair<std::string, std::string>> updates;
  std::vector<std::pair<std::string, std::string>> creates;
  for (std::size_t index = 0; index < names.size(); index += 1) {
    const bool root = names[index] == stream_group::ALL_GROUP_NAME;
    const bool parentEmpty = parents[index].empty();
    if (root && !parentEmpty) {
      throw std::runtime_error(stream_group::ERR_ALL);
    }
    if (!root && parentEmpty) {
      throw std::runtime_error(stream_group::ERR_PARENT_MISSING);
    }
    const auto parent = idByName.find(parents[index]);
    if (!root && parent == idByName.end()) {
      throw std::runtime_error(stream_group::ERR_PARENT_MISSING);
    }
    const auto current = idByName.find(names[index]);
    if (!root && current != idByName.end()) {
      std::string cursor = parent->second;
      while (!cursor.empty()) {
        if (cursor == current->second) {
          throw std::runtime_error(stream_group::ERR_PARENT_MISSING);
        }
        cursor = parentById[cursor];
      }
      updates.emplace_back(current->second, parent->second);
    }
    if (!root && current == idByName.end()) {
      creates.emplace_back(names[index], parent->second);
    }
  }
  {
    auto transaction = dbClient->newTransaction();
    for (const auto& created : creates) {
      Sql::sync({}, transaction, 
          "INSERT INTO stream_group (id, name, parent_id) VALUES (gen_random_uuid(), $1, $2::uuid)",
          created.first,
          created.second);
    }
    for (const auto& updated : updates) {
      Sql::sync({}, transaction, 
          "UPDATE stream_group SET parent_id = $1::uuid WHERE id = $2::uuid", updated.second, updated.first);
    }
  }
  Json::Value data = tree();
  return data;
}

std::string StreamGroupService::exportCSV() {
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT child.name, parent.name AS parent_name "
      "FROM stream_group child LEFT JOIN stream_group parent ON parent.id = child.parent_id "
      "ORDER BY child.name");
  std::ostringstream csv;
  csv << stream_group::CSV_HEADER << "\n";
  for (const auto& row : rows) {
    const std::string parent = row["parent_name"].isNull() ? "" : row["parent_name"].as<std::string>();
    csv << row["name"].as<std::string>() << "," << parent << "\n";
  }
  return csv.str();
}

Json::Value StreamGroupService::groupNode(
    const Json::Value& group,
    const std::unordered_map<std::string, std::vector<Json::Value>>& children,
    const std::unordered_map<std::string, std::vector<Json::Value>>& streams) {
  Json::Value node = group;
  Json::Value kids(Json::arrayValue);
  const auto streamIt = streams.find(group[stream_group::KEY_ID].asString());
  if (streamIt != streams.end()) {
    for (const auto& row : streamIt->second) {
      kids.append(row);
    }
  }
  const auto childIt = children.find(group[stream_group::KEY_ID].asString());
  if (childIt != children.end()) {
    for (const auto& child : childIt->second) {
      kids.append(groupNode(child, children, streams));
    }
  }
  node[stream_group::KEY_CHILDREN] = kids;
  return node;
}

Json::Value StreamGroupService::create(const std::string& parentGroupId, const Json::Value& name) {
  const auto rows = Sql::sync({}, dbClient, 
      "INSERT INTO stream_group (id, name, parent_id) "
      "SELECT gen_random_uuid(), $1, id FROM stream_group WHERE id = $2::uuid "
      "RETURNING id::text AS id, name, parent_id::text AS parent_id",
      name.asString(),
      parentGroupId);
  if (rows.empty()) {
    throw std::runtime_error("group not created");
  }
  Json::Value data;
  data[stream_group::KEY_ID] = rows[0]["id"].as<std::string>();
  data[stream_group::KEY_NAME] = rows[0]["name"].as<std::string>();
  data[stream_group::KEY_PARENT_ID] = rows[0]["parent_id"].as<std::string>();
  return data;
}

Json::Value StreamGroupService::members(const std::string& groupId) {
  const auto groups = Sql::sync({}, dbClient, 
      "SELECT id FROM stream_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error("group not found");
  }
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT stream.id::text AS id, stream.name, stream.group_id::text AS group_id, "
      "stream_group.name AS group_name FROM stream "
      "INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream.group_id = $1::uuid ORDER BY stream.name",
      groupId);
  Json::Value data(Json::arrayValue);
  for (const auto& row : rows) {
    Json::Value item;
    item[stream_group::KEY_ID] = row["id"].as<std::string>();
    item[stream_group::KEY_NAME] = row["name"].as<std::string>();
    item[stream_group::KEY_GROUP_ID] = row["group_id"].as<std::string>();
    item[stream_group::KEY_GROUP_NAME] = row["group_name"].as<std::string>();
    data.append(item);
  }
  return data;
}

Json::Value StreamGroupService::candidates(const std::string& groupId) {
  const auto groups = Sql::sync({}, dbClient, 
      "SELECT id FROM stream_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error("group not found");
  }
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT stream.id::text AS id, stream.name, stream.group_id::text AS group_id, "
      "stream_group.name AS group_name FROM stream "
      "INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream.group_id <> $1::uuid ORDER BY stream.name",
      groupId);
  Json::Value data(Json::arrayValue);
  for (const auto& row : rows) {
    Json::Value item;
    item[stream_group::KEY_ID] = row["id"].as<std::string>();
    item[stream_group::KEY_NAME] = row["name"].as<std::string>();
    item[stream_group::KEY_GROUP_ID] = row["group_id"].as<std::string>();
    item[stream_group::KEY_GROUP_NAME] = row["group_name"].as<std::string>();
    data.append(item);
  }
  return data;
}

Json::Value StreamGroupService::setMembers(const std::string& groupId, const Json::Value& streamIds) {
  const auto groups = Sql::sync({}, dbClient, 
      "SELECT id FROM stream_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error("group not found");
  }
  std::unordered_set<std::string> selected;
  for (const auto& id : streamIds) {
    selected.insert(id.asString());
  }
  {
    auto transaction = dbClient->newTransaction();
    for (const auto& id : selected) {
      Sql::sync({}, transaction, 
          "UPDATE stream SET group_id = $1::uuid WHERE id = $2::uuid AND group_id <> $1::uuid",
          groupId,
          id);
    }
    const auto current = Sql::sync({}, transaction, 
        "SELECT id::text AS id FROM stream WHERE group_id = $1::uuid", groupId);
    for (const auto& row : current) {
      const auto id = row["id"].as<std::string>();
      if (selected.count(id) == 0) {
        Sql::sync({}, transaction, 
            "UPDATE stream SET group_id = $1::uuid WHERE id = $2::uuid",
            stream_group::ALL_GROUP_ID,
            id);
      }
    }
  }
  Json::Value data = tree();
  return data;
}

Json::Value StreamGroupService::update(
    const std::string& groupId, const Json::Value& name, const Json::Value& parentId) {
  if (groupId == stream_group::ALL_GROUP_ID) {
    throw std::runtime_error(stream_group::ERR_ALL);
  }
  const std::string parent = parentId.asString();
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, parent_id::text AS parent_id FROM stream_group");
  std::unordered_map<std::string, std::string> parentById;
  bool found = false;
  bool parentFound = false;
  for (const auto& row : rows) {
    const auto id = row["id"].as<std::string>();
    parentById[id] = row["parent_id"].isNull() ? "" : row["parent_id"].as<std::string>();
    found = found || id == groupId;
    parentFound = parentFound || id == parent;
  }
  std::string cursor = parentFound ? parent : "";
  bool cycle = false;
  while (!cursor.empty()) {
    cycle = cycle || cursor == groupId;
    cursor = cycle ? "" : parentById[cursor];
  }
  if (!parentFound || cycle) {
    throw std::runtime_error(stream_group::ERR_PARENT_MISSING);
  }
  if (!found) {
    throw std::runtime_error("group not updated");
  }
  const auto updated = Sql::sync({}, dbClient, 
      "UPDATE stream_group SET name = $1, parent_id = $2::uuid "
      "WHERE id = $3::uuid AND parent_id IS NOT NULL "
      "RETURNING id::text AS id, name, parent_id::text AS parent_id",
      name.asString(),
      parent,
      groupId);
  if (updated.empty()) {
    throw std::runtime_error("group not updated");
  }
  Json::Value data;
  data[stream_group::KEY_ID] = updated[0]["id"].as<std::string>();
  data[stream_group::KEY_NAME] = updated[0]["name"].as<std::string>();
  data[stream_group::KEY_PARENT_ID] = updated[0]["parent_id"].as<std::string>();
  return data;
}

std::vector<std::string> StreamGroupService::subtreeIds(const std::string& groupId) {
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, parent_id::text AS parent_id FROM stream_group");
  std::unordered_map<std::string, std::vector<std::string>> children;
  bool found = false;
  for (const auto& row : rows) {
    const auto id = row["id"].as<std::string>();
    const auto parentId = row["parent_id"].isNull() ? "" : row["parent_id"].as<std::string>();
    found = found || (id == groupId && !parentId.empty());
    if (!parentId.empty()) {
      children[parentId].push_back(id);
    }
  }
  std::vector<std::string> ids;
  if (found) {
    ids.push_back(groupId);
  }
  for (size_t index = 0; index < ids.size(); ++index) {
    const auto childIt = children.find(ids[index]);
    if (childIt != children.end()) {
      ids.insert(ids.end(), childIt->second.begin(), childIt->second.end());
    }
  }
  return ids;
}

Json::Value StreamGroupService::remove(const std::string& groupId) {
  if (groupId == stream_group::ALL_GROUP_ID) {
    throw std::runtime_error(stream_group::ERR_ALL_DELETE);
  }
  const auto ids = subtreeIds(groupId);
  if (ids.empty()) {
    throw std::runtime_error("group not deleted");
  }
  {
    auto transaction = dbClient->newTransaction();
    for (const auto& id : ids) {
      Sql::sync({}, transaction, 
          "UPDATE stream SET group_id = $1::uuid WHERE group_id = $2::uuid",
          stream_group::ALL_GROUP_ID,
          id);
    }
    for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
      Sql::sync({}, transaction, "DELETE FROM stream_group WHERE id = $1::uuid", *it);
    }
  }
  Json::Value data = tree();
  return data;
}

