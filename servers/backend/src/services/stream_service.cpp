#include "services/stream_service.hpp"

#include "error_tracer/sql.hpp"

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
                   long pageSize) {
  const long offset = static_cast<long>(page - 1) * pageSize;
  const bool byStream = !streamId.empty();
  const std::string columns =
      "stream.id::text AS id, stream.name, stream.group_id::text AS group_id, "
      "stream_group.name AS group_name, stream.url, stream.width, stream.height, stream.fps, "
      "stream.status, stream.mount, stream.record, stream.probed_at ";
  const std::string match = Searcher::clause(stream::SEARCH_COLUMNS, 2);
  const std::string byStreamFrom =
      "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream.id = $1::uuid AND " +
      match + " ";
  const std::string scopeId = groupId.empty() ? stream_group::ALL_GROUP_ID : groupId;
  const auto counted = byStream
      ? Sql::sync({}, dbClient, "SELECT count(*) AS total " + byStreamFrom, streamId, search)
      : Sql::sync({}, dbClient, 
            "WITH RECURSIVE subtree AS ("
            "SELECT id FROM stream_group WHERE id = $1::uuid "
            "UNION ALL "
            "SELECT child.id FROM stream_group child INNER JOIN subtree ON child.parent_id = subtree.id"
            ") SELECT count(*) AS total FROM stream "
            "INNER JOIN stream_group ON stream_group.id = stream.group_id "
            "WHERE stream.group_id IN (SELECT id FROM subtree) AND " +
                match,
            scopeId,
            search);
  const auto rows = byStream
      ? Sql::sync({}, dbClient, 
            "SELECT " + columns + byStreamFrom + "ORDER BY stream.name LIMIT $3 OFFSET $4",
            streamId,
            search,
            pageSize,
            offset)
      : Sql::sync({}, dbClient, 
            "WITH RECURSIVE subtree AS ("
            "SELECT id FROM stream_group WHERE id = $1::uuid "
            "UNION ALL "
            "SELECT child.id FROM stream_group child INNER JOIN subtree ON child.parent_id = subtree.id"
            ") SELECT " +
                columns +
                "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
                "WHERE stream.group_id IN (SELECT id FROM subtree) AND " + match + " "
                "ORDER BY stream.name LIMIT $3 OFFSET $4",
            scopeId,
            search,
            pageSize,
            offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(row2json(row));
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
                                  const Json::Value& mount,
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
  data[stream::KEY_MOUNT] = mount.isNull() ? data[stream::KEY_MOUNT] : mount;
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
  const auto rows = Sql::sync({}, dbClient, 
      "INSERT INTO stream (id, name, group_id, url, width, height, fps, status, mount, record, probed_at) "
      "VALUES (gen_random_uuid(), $1, $2::uuid, $3, $4, $5, $6, $7, $8, $9, $10::timestamptz) "
      "RETURNING id::text AS id, name, group_id::text AS group_id, "
      "(SELECT name FROM stream_group WHERE id = group_id) AS group_name, "
      "url, width, height, fps, status, mount, record, probed_at",
      data[stream::KEY_NAME].asString(),
      data[stream::KEY_GROUP_ID].asString(),
      data[stream::KEY_URL].asString(),
      widthValue,
      heightValue,
      fpsValue,
      data[stream::KEY_STATUS].asString(),
      data[stream::KEY_MOUNT].asBool(),
      data[stream::KEY_RECORD].asBool(),
      probedAtValue);
  if (rows.empty()) {
    throw std::runtime_error("stream not created");
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

Json::Value StreamService::get(const std::string& streamId) {
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT stream.id::text AS id, stream.name, stream.group_id::text AS group_id, "
      "stream_group.name AS group_name, stream.url, stream.width, stream.height, stream.fps, "
      "stream.status, stream.mount, stream.record, stream.probed_at "
      "FROM stream INNER JOIN stream_group ON stream_group.id = stream.group_id "
      "WHERE stream.id = $1::uuid",
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not found");
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value StreamService::update(const std::string& streamId,
                                  const Json::Value& name,
                                  const Json::Value& groupId,
                                  const Json::Value& url,
                                  const Json::Value& mount,
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
  data[stream::KEY_MOUNT] = mount.isNull() ? data[stream::KEY_MOUNT] : mount;
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
  const auto rows = Sql::sync({}, dbClient, 
      "UPDATE stream SET name = $1, group_id = $2::uuid, url = $3, width = $4, height = $5, fps = $6, "
      "status = $7, mount = $8, record = $9, probed_at = $10::timestamptz "
      "WHERE id = $11::uuid "
      "RETURNING id::text AS id, name, group_id::text AS group_id, "
      "(SELECT name FROM stream_group WHERE id = group_id) AS group_name, "
      "url, width, height, fps, status, mount, record, probed_at",
      data[stream::KEY_NAME].asString(),
      data[stream::KEY_GROUP_ID].asString(),
      data[stream::KEY_URL].asString(),
      widthValue,
      heightValue,
      fpsValue,
      data[stream::KEY_STATUS].asString(),
      data[stream::KEY_MOUNT].asBool(),
      data[stream::KEY_RECORD].asBool(),
      probedAtValue,
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not updated");
  }
  Json::Value result = row2json(rows[0]);
  return result;
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
  const auto rows = Sql::sync({}, dbClient, 
      "UPDATE stream SET status = $1, width = $2, height = $3, fps = $4, probed_at = $5::timestamptz "
      "WHERE id = $6::uuid "
      "RETURNING id::text AS id, name, group_id::text AS group_id, "
      "(SELECT name FROM stream_group WHERE id = group_id) AS group_name, "
      "url, width, height, fps, status, mount, record, probed_at",
      status.asString(),
      widthValue,
      heightValue,
      fpsValue,
      probedAtValue,
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not probed");
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

void StreamService::setMount(const std::string& streamId, bool mount) {
  const auto rows = Sql::sync({}, dbClient,
      "UPDATE stream SET mount = $1 WHERE id = $2::uuid RETURNING id", mount, streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream mount not set");
  }
}

void StreamService::setRecord(const std::string& streamId, bool recording) {
  const auto rows = Sql::sync({}, dbClient, 
      "UPDATE stream SET record = $1 WHERE id = $2::uuid RETURNING id", recording, streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream record not set");
  }
}

Json::Value StreamService::remove(const std::string& streamId) {
  const auto rows = Sql::sync({}, dbClient, 
      "DELETE FROM stream WHERE id = $1::uuid "
      "RETURNING id::text AS id, name, group_id::text AS group_id, "
      "(SELECT name FROM stream_group WHERE id = group_id) AS group_name, "
      "url, width, height, fps, status, mount, record, probed_at",
      streamId);
  if (rows.empty()) {
    throw std::runtime_error("stream not deleted");
  }
  Json::Value data = row2json(rows[0]);
  return data;
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
  Json::Value data = stream::BATCH_DATA();
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
      Json::Value failed;
      failed[stream::KEY_NAME] = line;
      data[stream::KEY_FAILED_ROWS].append(failed);
    }
    if (!blank && columns) {
      const std::string name = line.substr(0, first);
      const std::string url = line.substr(first + 1, second - first - 1);
      const std::string group = line.substr(second + 1);
      const bool nameSeen = !seenNames.insert(name).second;
      const bool urlSeen = !seenUrls.insert(url).second;
      if (nameSeen || urlSeen) {
        Json::Value failed;
        failed[stream::KEY_NAME] = name;
        failed[stream::KEY_URL] = url;
        failed[stream::KEY_GROUP_NAME] = group;
        data[stream::KEY_FAILED_ROWS].append(failed);
      } else {
        names.push_back(name);
        urls.push_back(url);
        groups.push_back(group);
      }
    }
  }
  std::unordered_map<std::string, std::string> groupIds;
  const auto groupRows = Sql::sync({}, dbClient, "SELECT id::text AS id, name FROM stream_group");
  for (const auto& row : groupRows) {
    groupIds[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  std::unordered_map<std::string, std::string> idByName;
  std::unordered_map<std::string, std::string> idByUrl;
  std::unordered_map<std::string, std::string> groupByStream;
  const auto streamRows = Sql::sync({}, dbClient, 
      "SELECT id::text AS id, name, url, group_id::text AS group_id FROM stream");
  for (const auto& row : streamRows) {
    const std::string id = row["id"].as<std::string>();
    idByName[row["name"].as<std::string>()] = id;
    idByUrl[row["url"].as<std::string>()] = id;
    groupByStream[id] = row["group_id"].as<std::string>();
  }
  std::vector<std::string> skipped;
  std::vector<std::pair<std::string, std::string>> updates;
  std::vector<std::tuple<std::string, std::string, std::string>> creates;
  for (std::size_t index = 0; index < names.size(); index += 1) {
    const auto group = groupIds.find(groups[index]);
    const auto named = idByName.find(names[index]);
    const auto addressed = idByUrl.find(urls[index]);
    const bool hasGroup = group != groupIds.end();
    const bool hasName = named != idByName.end();
    const bool hasUrl = addressed != idByUrl.end();
    const bool same = hasName && hasUrl && named->second == addressed->second;
    const bool unchanged = same && hasGroup && groupByStream[named->second] == group->second;
    if (!hasGroup || (hasName && !same) || (hasUrl && !same)) {
      Json::Value failed;
      failed[stream::KEY_NAME] = names[index];
      failed[stream::KEY_URL] = urls[index];
      failed[stream::KEY_GROUP_NAME] = groups[index];
      data[stream::KEY_FAILED_ROWS].append(failed);
    }
    if (unchanged) {
      skipped.push_back(named->second);
    }
    if (same && hasGroup && !unchanged) {
      updates.emplace_back(named->second, group->second);
    }
    if (!hasName && !hasUrl && hasGroup) {
      creates.emplace_back(names[index], urls[index], group->second);
    }
  }
  const char* returning =
      "id::text AS id, name, group_id::text AS group_id, "
      "(SELECT name FROM stream_group WHERE id = group_id) AS group_name, "
      "url, width, height, fps, status, mount, record, probed_at";
  {
    auto transaction = dbClient->newTransaction();
    for (const auto& created : creates) {
      const auto rows = Sql::sync({}, transaction, 
          std::string("INSERT INTO stream (id, name, group_id, url, status, mount, record) "
                      "VALUES (gen_random_uuid(), $1, $2::uuid, $3, $4, false, false) RETURNING ") +
              returning,
          std::get<0>(created),
          std::get<2>(created),
          std::get<1>(created),
          stream::STATUS_OFFLINE);
      if (!rows.empty()) {
        data[stream::KEY_UPDATED_ROWS].append(row2json(rows[0]));
      }
    }
    for (const auto& updated : updates) {
      const auto rows = Sql::sync({}, transaction, 
          std::string("UPDATE stream SET group_id = $1::uuid WHERE id = $2::uuid RETURNING ") + returning,
          updated.second,
          updated.first);
      if (!rows.empty()) {
        data[stream::KEY_UPDATED_ROWS].append(row2json(rows[0]));
      }
    }
  }
  for (const auto& id : skipped) {
    data[stream::KEY_SKIPPED_ROWS].append(get(id));
  }
  data[stream::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_UPDATED_ROWS].size());
  data[stream::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_FAILED_ROWS].size());
  data[stream::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[stream::KEY_SKIPPED_ROWS].size());
  return data;
}

std::string StreamService::exportCSV() {
  const auto rows = Sql::sync({}, dbClient, 
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

Json::Value StreamService::row2json(const drogon::orm::Row& row) {
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
  node[stream::KEY_MOUNT] = row["mount"].as<bool>();
  node[stream::KEY_RECORD] = row["record"].as<bool>();
  node[stream::KEY_PROBED_AT] =
      row["probed_at"].isNull() ? Json::Value() : row["probed_at"].as<std::string>();
  return node;
}
