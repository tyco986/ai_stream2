#include "services/stream_group.hpp"

#include "constants/stream_group.hpp"

#include <stdexcept>

StreamGroupService::StreamGroupService(drogon::orm::DbClientPtr dbClient) {
  this->dbClient = std::move(dbClient);
}

Json::Value StreamGroupService::tree() {
  const auto groupRows = dbClient->execSqlSync(
      "SELECT id::text AS id, name, parent_id::text AS parent_id FROM stream_group ORDER BY name");
  const auto streamRows = dbClient->execSqlSync(
      "SELECT id::text AS id, name, group_id::text AS group_id, enabled, status, recording "
      "FROM stream ORDER BY name");

  GroupRow root;
  std::unordered_map<std::string, std::vector<GroupRow>> children;
  for (const auto& row : groupRows) {
    GroupRow group;
    group.id = row["id"].as<std::string>();
    group.name = row["name"].as<std::string>();
    group.parentId = row["parent_id"].isNull() ? "" : row["parent_id"].as<std::string>();
    if (group.parentId.empty()) {
      root = group;
    } else {
      children[group.parentId].push_back(group);
    }
  }

  std::unordered_map<std::string, std::vector<StreamRow>> streams;
  for (const auto& row : streamRows) {
    StreamRow stream;
    stream.id = row["id"].as<std::string>();
    stream.name = row["name"].as<std::string>();
    stream.enabled = row["enabled"].as<bool>();
    stream.status = row["status"].as<std::string>();
    stream.recording = row["recording"].as<bool>();
    streams[row["group_id"].as<std::string>()].push_back(stream);
  }

  Json::Value data = groupNode(root, children, streams);
  return data;
}

Json::Value StreamGroupService::map() {
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, name FROM stream_group ORDER BY name");
  Json::Value data;
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value StreamGroupService::groupNode(
    const GroupRow& group,
    const std::unordered_map<std::string, std::vector<GroupRow>>& children,
    const std::unordered_map<std::string, std::vector<StreamRow>>& streams) {
  Json::Value node;
  node[stream_group::KEY_ID] = group.id;
  node[stream_group::KEY_NAME] = group.name;
  node[stream_group::KEY_TYPE] = stream_group::TYPE_GROUP;
  Json::Value kids(Json::arrayValue);
  const auto streamIt = streams.find(group.id);
  if (streamIt != streams.end()) {
    for (const auto& row : streamIt->second) {
      kids.append(streamNode(row));
    }
  }
  const auto childIt = children.find(group.id);
  if (childIt != children.end()) {
    for (const auto& child : childIt->second) {
      kids.append(groupNode(child, children, streams));
    }
  }
  node[stream_group::KEY_CHILDREN] = kids;
  return node;
}

Json::Value StreamGroupService::create(const std::string& parentGroupId, const std::string& name) {
  const auto rows = dbClient->execSqlSync(
      "INSERT INTO stream_group (id, name, parent_id) "
      "SELECT gen_random_uuid(), $1, id FROM stream_group WHERE id = $2::uuid "
      "RETURNING id::text AS id, name, parent_id::text AS parent_id",
      name,
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
  const auto rows = dbClient->execSqlSync(
      "SELECT stream.name, stream.id::text AS id FROM stream "
      "INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream_group.id = $1::uuid ORDER BY stream.name",
      groupId);
  Json::Value data;
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  const auto groups = dbClient->execSqlSync(
      "SELECT id FROM stream_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error("group not found");
  }
  return data;
}

Json::Value StreamGroupService::candidates(const std::string& groupId) {
  const auto groups = dbClient->execSqlSync(
      "SELECT id FROM stream_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error("group not found");
  }
  const auto rows = dbClient->execSqlSync(
      "SELECT name, id::text AS id FROM stream WHERE group_id <> $1::uuid ORDER BY name",
      groupId);
  Json::Value data;
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value StreamGroupService::setMembers(const std::string& groupId,
                                           const std::vector<std::string>& streamIds) {
  const auto groups = dbClient->execSqlSync(
      "SELECT id FROM stream_group WHERE id = $1::uuid", groupId);
  if (groups.empty()) {
    throw std::runtime_error("group not found");
  }
  std::unordered_set<std::string> selected(streamIds.begin(), streamIds.end());
  auto transaction = dbClient->newTransaction();
  int updated = 0;
  for (const auto& id : selected) {
    const auto result = transaction->execSqlSync(
        "UPDATE stream SET group_id = $1::uuid WHERE id = $2::uuid AND group_id <> $1::uuid",
        groupId,
        id);
    updated += result.affectedRows();
  }
  const auto current = transaction->execSqlSync(
      "SELECT id::text AS id FROM stream WHERE group_id = $1::uuid", groupId);
  int removed = 0;
  for (const auto& row : current) {
    const auto id = row["id"].as<std::string>();
    if (selected.count(id) == 0) {
      transaction->execSqlSync(
          "UPDATE stream SET group_id = $1::uuid WHERE id = $2::uuid",
          stream_group::ALL_GROUP_ID,
          id);
      ++removed;
    }
  }
  Json::Value data;
  data[stream_group::KEY_UPDATED_COUNT] = updated;
  data[stream_group::KEY_REMOVED_COUNT] = removed;
  return data;
}

Json::Value StreamGroupService::rename(const std::string& groupId, const std::string& name) {
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream_group SET name = $1 WHERE id = $2::uuid AND parent_id IS NOT NULL "
      "RETURNING id::text AS id, name, parent_id::text AS parent_id",
      name,
      groupId);
  if (rows.empty()) {
    throw std::runtime_error("group not renamed");
  }
  Json::Value data;
  data[stream_group::KEY_ID] = rows[0]["id"].as<std::string>();
  data[stream_group::KEY_NAME] = rows[0]["name"].as<std::string>();
  data[stream_group::KEY_PARENT_ID] = rows[0]["parent_id"].as<std::string>();
  return data;
}

std::vector<std::string> StreamGroupService::subtreeIds(const std::string& groupId) {
  const auto rows = dbClient->execSqlSync(
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
  const auto ids = subtreeIds(groupId);
  if (ids.empty()) {
    throw std::runtime_error("group not deleted");
  }
  auto transaction = dbClient->newTransaction();
  int moved = 0;
  for (const auto& id : ids) {
    const auto updated = transaction->execSqlSync(
        "UPDATE stream SET group_id = $1::uuid WHERE group_id = $2::uuid",
        stream_group::ALL_GROUP_ID,
        id);
    moved += updated.affectedRows();
  }
  std::unordered_set<std::string> remaining(ids.begin(), ids.end());
  std::unordered_map<std::string, std::vector<std::string>> children;
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, parent_id::text AS parent_id FROM stream_group");
  for (const auto& row : rows) {
    if (!row["parent_id"].isNull()) {
      children[row["parent_id"].as<std::string>()].push_back(row["id"].as<std::string>());
    }
  }
  Json::Value deleted(Json::arrayValue);
  while (!remaining.empty()) {
    std::string leaf;
    for (const auto& id : remaining) {
      const auto childIt = children.find(id);
      bool hasChild = false;
      if (childIt != children.end()) {
        for (const auto& child : childIt->second) {
          hasChild = hasChild || remaining.count(child) > 0;
        }
      }
      if (!hasChild) {
        leaf = id;
      }
    }
    transaction->execSqlSync("DELETE FROM stream_group WHERE id = $1::uuid", leaf);
    deleted.append(leaf);
    remaining.erase(leaf);
  }
  Json::Value data;
  data[stream_group::KEY_DELETED_GROUP_IDS] = deleted;
  data[stream_group::KEY_MOVED_STREAM_COUNT] = moved;
  return data;
}

Json::Value StreamGroupService::streamNode(const StreamRow& row) {
  Json::Value node;
  node[stream_group::KEY_ID] = row.id;
  node[stream_group::KEY_NAME] = row.name;
  node[stream_group::KEY_TYPE] = stream_group::TYPE_STREAM;
  node[stream_group::KEY_ENABLED] = row.enabled;
  node[stream_group::KEY_STATUS] = row.status;
  node[stream_group::KEY_RECORDING] = row.recording;
  return node;
}
