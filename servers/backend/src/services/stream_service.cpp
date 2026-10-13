#include "services/stream_service.hpp"

#include <optional>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "constants/stream_const.hpp"
#include "constants/stream_group_const.hpp"
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
      "stream.status, stream.enabled, stream.record, stream.probed_at ";
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
       "stream.record",
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

Json::Value StreamService::create(const Json::Value& name,
                                  const Json::Value& groupId,
                                  const Json::Value& url,
                                  const Json::Value& enabled,
                                  const Json::Value& recording,
                                  const Json::Value& status,
                                  const Json::Value& width,
                                  const Json::Value& height,
                                  const Json::Value& fps,
                                  const Json::Value& probedAt) {
  Json::Value data = stream::ROW();
  data[stream::KEY_NAME] = name.isNull() ? data[stream::KEY_NAME] : name;
  data[stream::KEY_GROUP_ID] = groupId.isNull() ? data[stream::KEY_GROUP_ID] : groupId;
  data[stream::KEY_URL] = url.isNull() ? data[stream::KEY_URL] : url;
  data[stream::KEY_ENABLED] = enabled.isNull() ? data[stream::KEY_ENABLED] : enabled;
  data[stream::KEY_RECORD] = recording.isNull() ? data[stream::KEY_RECORD] : recording;
  data[stream::KEY_STATUS] = status.isNull() ? data[stream::KEY_STATUS] : status;
  data[stream::KEY_WIDTH] = width.isNull() ? data[stream::KEY_WIDTH] : width;
  data[stream::KEY_HEIGHT] = height.isNull() ? data[stream::KEY_HEIGHT] : height;
  data[stream::KEY_FPS] = fps.isNull() ? data[stream::KEY_FPS] : fps;
  data[stream::KEY_PROBED_AT] = probedAt.isNull() ? data[stream::KEY_PROBED_AT] : probedAt;
  const std::optional<int> widthValue =
      data[stream::KEY_WIDTH].isNull() ? std::nullopt : std::optional<int>(data[stream::KEY_WIDTH].asInt());
  const std::optional<int> heightValue =
      data[stream::KEY_HEIGHT].isNull() ? std::nullopt : std::optional<int>(data[stream::KEY_HEIGHT].asInt());
  const std::optional<double> fpsValue =
      data[stream::KEY_FPS].isNull() ? std::nullopt : std::optional<double>(data[stream::KEY_FPS].asDouble());
  const std::optional<std::string> probedAtValue = data[stream::KEY_PROBED_AT].isNull()
      ? std::nullopt
      : std::optional<std::string>(data[stream::KEY_PROBED_AT].asString());
  const auto rows = dbClient->execSqlSync(
      "INSERT INTO stream (id, name, group_id, url, width, height, fps, status, enabled, record, probed_at) "
      "VALUES (gen_random_uuid(), $1, $2::uuid, $3, $4, $5, $6, $7, $8, $9, $10::timestamptz) "
      "RETURNING id::text AS id, name, group_id::text AS group_id, "
      "(SELECT name FROM stream_group WHERE id = group_id) AS group_name, "
      "url, width, height, fps, status, enabled, record, probed_at",
      data[stream::KEY_NAME].asString(),
      data[stream::KEY_GROUP_ID].asString(),
      data[stream::KEY_URL].asString(),
      widthValue,
      heightValue,
      fpsValue,
      data[stream::KEY_STATUS].asString(),
      data[stream::KEY_ENABLED].asBool(),
      data[stream::KEY_RECORD].asBool(),
      probedAtValue);
  if (rows.empty()) {
    throw std::runtime_error("stream not created");
  }
  Json::Value result = item(rows[0]);
  return result;
}

Json::Value StreamService::get(const std::string& streamId) {
  const auto rows = dbClient->execSqlSync(
      "SELECT stream.id::text AS id, stream.name, stream.group_id::text AS group_id, "
      "stream_group.name AS group_name, stream.url, stream.width, stream.height, stream.fps, "
      "stream.status, stream.enabled, stream.record, stream.probed_at "
      "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream.id = $1::uuid",
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not found");
  }
  Json::Value data = item(rows[0]);
  return data;
}

Json::Value StreamService::update(const std::string& streamId,
                                  const Json::Value& name,
                                  const Json::Value& groupId,
                                  const Json::Value& url,
                                  const Json::Value& enabled,
                                  const Json::Value& recording,
                                  const Json::Value& status,
                                  const Json::Value& width,
                                  const Json::Value& height,
                                  const Json::Value& fps,
                                  const Json::Value& probedAt) {
  Json::Value data = get(streamId);
  data[stream::KEY_NAME] = name.isNull() ? data[stream::KEY_NAME] : name;
  data[stream::KEY_GROUP_ID] = groupId.isNull() ? data[stream::KEY_GROUP_ID] : groupId;
  data[stream::KEY_URL] = url.isNull() ? data[stream::KEY_URL] : url;
  data[stream::KEY_ENABLED] = enabled.isNull() ? data[stream::KEY_ENABLED] : enabled;
  data[stream::KEY_RECORD] = recording.isNull() ? data[stream::KEY_RECORD] : recording;
  data[stream::KEY_STATUS] = status.isNull() ? data[stream::KEY_STATUS] : status;
  data[stream::KEY_WIDTH] = width.isNull() ? data[stream::KEY_WIDTH] : width;
  data[stream::KEY_HEIGHT] = height.isNull() ? data[stream::KEY_HEIGHT] : height;
  data[stream::KEY_FPS] = fps.isNull() ? data[stream::KEY_FPS] : fps;
  data[stream::KEY_PROBED_AT] = probedAt.isNull() ? data[stream::KEY_PROBED_AT] : probedAt;
  const std::optional<int> widthValue =
      data[stream::KEY_WIDTH].isNull() ? std::nullopt : std::optional<int>(data[stream::KEY_WIDTH].asInt());
  const std::optional<int> heightValue =
      data[stream::KEY_HEIGHT].isNull() ? std::nullopt : std::optional<int>(data[stream::KEY_HEIGHT].asInt());
  const std::optional<double> fpsValue =
      data[stream::KEY_FPS].isNull() ? std::nullopt : std::optional<double>(data[stream::KEY_FPS].asDouble());
  const std::optional<std::string> probedAtValue = data[stream::KEY_PROBED_AT].isNull()
      ? std::nullopt
      : std::optional<std::string>(data[stream::KEY_PROBED_AT].asString());
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream SET name = $1, group_id = $2::uuid, url = $3, width = $4, height = $5, fps = $6, "
      "status = $7, enabled = $8, record = $9, probed_at = $10::timestamptz "
      "WHERE id = $11::uuid "
      "RETURNING id::text AS id, name, group_id::text AS group_id, "
      "(SELECT name FROM stream_group WHERE id = group_id) AS group_name, "
      "url, width, height, fps, status, enabled, record, probed_at",
      data[stream::KEY_NAME].asString(),
      data[stream::KEY_GROUP_ID].asString(),
      data[stream::KEY_URL].asString(),
      widthValue,
      heightValue,
      fpsValue,
      data[stream::KEY_STATUS].asString(),
      data[stream::KEY_ENABLED].asBool(),
      data[stream::KEY_RECORD].asBool(),
      probedAtValue,
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not updated");
  }
  Json::Value data = item(rows[0]);
  return data;
}

Json::Value StreamService::probe(const std::string& streamId,
                                 const Json::Value& status,
                                 const Json::Value& width,
                                 const Json::Value& height,
                                 const Json::Value& fps,
                                 const Json::Value& probedAt) {
  const std::optional<int> widthValue = width.isNull() ? std::nullopt : std::optional<int>(width.asInt());
  const std::optional<int> heightValue = height.isNull() ? std::nullopt : std::optional<int>(height.asInt());
  const std::optional<double> fpsValue = fps.isNull() ? std::nullopt : std::optional<double>(fps.asDouble());
  const std::optional<std::string> probedAtValue =
      probedAt.isNull() ? std::nullopt : std::optional<std::string>(probedAt.asString());
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream SET status = $1, width = $2, height = $3, fps = $4, probed_at = $5::timestamptz "
      "WHERE id = $6::uuid "
      "RETURNING id::text AS id, status, width, height, fps, probed_at",
      status.asString(),
      widthValue,
      heightValue,
      fpsValue,
      probedAtValue,
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not probed");
  }
  Json::Value data = probeItem(rows[0]);
  return data;
}

void StreamService::enable(const std::string& streamId) {
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream SET enabled = true WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not enabled");
  }
}

void StreamService::disable(const std::string& streamId) {
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream SET enabled = false WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not disabled");
  }
}

void StreamService::record(const std::string& streamId) {
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream SET record = true WHERE id = $1::uuid RETURNING id", streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not recorded");
  }
}

void StreamService::unrecord(const std::string& streamId) {
  const auto rows = dbClient->execSqlSync(
      "UPDATE stream SET record = false WHERE id = $1::uuid RETURNING id", streamId);
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

Json::Value StreamService::importCSV(const std::string& csv) {
  std::istringstream input(csv);
  std::string line;
  const bool headed = static_cast<bool>(std::getline(input, line));
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  if (!headed || line != stream::CSV_HEADER) {
    throw std::runtime_error(stream::ERR_CSV);
  }
  std::vector<std::string> names;
  std::vector<std::string> urls;
  std::vector<std::string> groups;
  std::unordered_set<std::string> seenNames;
  std::unordered_set<std::string> seenUrls;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const bool blank = line.empty();
    const std::size_t first = line.find(',');
    const std::size_t second = first == std::string::npos ? std::string::npos : line.find(',', first + 1);
    const bool columns = first != std::string::npos && second != std::string::npos &&
        line.find(',', second + 1) == std::string::npos;
    if (!blank && !columns) {
      throw std::runtime_error(stream::ERR_CSV);
    }
    if (!blank) {
      const std::string name = line.substr(0, first);
      const std::string url = line.substr(first + 1, second - first - 1);
      const std::string group = line.substr(second + 1);
      const bool nameSeen = !seenNames.insert(name).second;
      const bool urlSeen = !seenUrls.insert(url).second;
      if (nameSeen) {
        throw std::runtime_error(stream::ERR_NAME_EXISTS);
      }
      if (urlSeen) {
        throw std::runtime_error(stream::ERR_URL_EXISTS);
      }
      names.push_back(name);
      urls.push_back(url);
      groups.push_back(group);
    }
  }
  std::unordered_map<std::string, std::string> groupIds;
  const auto groupRows = dbClient->execSqlSync("SELECT id::text AS id, name FROM stream_group");
  for (const auto& row : groupRows) {
    groupIds[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  std::unordered_map<std::string, std::string> idByName;
  std::unordered_map<std::string, std::string> idByUrl;
  const auto streamRows = dbClient->execSqlSync("SELECT id::text AS id, name, url FROM stream");
  for (const auto& row : streamRows) {
    idByName[row["name"].as<std::string>()] = row["id"].as<std::string>();
    idByUrl[row["url"].as<std::string>()] = row["id"].as<std::string>();
  }
  std::vector<std::pair<std::string, std::string>> updates;
  std::vector<std::tuple<std::string, std::string, std::string>> creates;
  for (std::size_t index = 0; index < names.size(); index += 1) {
    const auto group = groupIds.find(groups[index]);
    if (group == groupIds.end()) {
      throw std::runtime_error(stream::ERR_GROUP_MISSING);
    }
    const auto named = idByName.find(names[index]);
    const auto addressed = idByUrl.find(urls[index]);
    const bool hasName = named != idByName.end();
    const bool hasUrl = addressed != idByUrl.end();
    const bool same = hasName && hasUrl && named->second == addressed->second;
    if (hasName && !same) {
      throw std::runtime_error(stream::ERR_NAME_EXISTS);
    }
    if (hasUrl && !same) {
      throw std::runtime_error(stream::ERR_URL_EXISTS);
    }
    if (same) {
      updates.emplace_back(named->second, group->second);
    }
    if (!hasName && !hasUrl) {
      creates.emplace_back(names[index], urls[index], group->second);
    }
  }
  auto transaction = dbClient->newTransaction();
  for (const auto& created : creates) {
    transaction->execSqlSync(
        "INSERT INTO stream (id, name, group_id, url, status, enabled, record) "
        "VALUES (gen_random_uuid(), $1, $2::uuid, $3, $4, false, false)",
        std::get<0>(created),
        std::get<2>(created),
        std::get<1>(created),
        stream::STATUS_OFFLINE);
  }
  for (const auto& updated : updates) {
    transaction->execSqlSync(
        "UPDATE stream SET group_id = $1::uuid WHERE id = $2::uuid", updated.second, updated.first);
  }
  Json::Value data;
  data[stream::KEY_CREATED_COUNT] = static_cast<Json::UInt64>(creates.size());
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(updates.size());
  return data;
}

std::string StreamService::exportCSV() {
  const auto rows = dbClient->execSqlSync(
      "SELECT stream.name, stream.url, stream_group.name AS group_name "
      "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "ORDER BY stream.name");
  std::ostringstream csv;
  csv << stream::CSV_HEADER << "\n";
  for (const auto& row : rows) {
    csv << row["name"].as<std::string>() << "," << row["url"].as<std::string>() << ","
        << row["group_name"].as<std::string>() << "\n";
  }
  return csv.str();
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
  node[stream::KEY_RECORD] = row["record"].as<bool>();
  node[stream::KEY_PROBED_AT] =
      row["probed_at"].isNull() ? Json::Value() : row["probed_at"].as<std::string>();
  return node;
}
