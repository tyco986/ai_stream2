#include "services/pipeline_analyzer_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/pipeline_analyzer_const.hpp"
#include "searcher/searcher.hpp"

#include <stdexcept>

PipelineAnalyzerService::PipelineAnalyzerService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value PipelineAnalyzerService::list(const std::string& search, int page, long pageSize) {
  const long offset = static_cast<long>(page - 1) * pageSize;
  const std::string match = Searcher::clause(
      {"pipeline_analyzer.id", "pipeline_analyzer.name", "pipeline_analyzer.media_id", "pipeline_analyzer.annotations"},
      1);
  const std::string from = std::string(pipeline_analyzer::FROM_MEDIA) + "WHERE " + match + " ";
  const auto counted = Sql::sync({}, dbClient, "SELECT count(*) AS total " + from, search);
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + pipeline_analyzer::COLUMNS + " " + from +
          "ORDER BY pipeline_analyzer.name LIMIT $2 OFFSET $3",
      search,
      pageSize,
      offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(row2json(row));
  }
  Json::Value data;
  data[pipeline_analyzer::KEY_ITEMS] = items;
  data[pipeline_analyzer::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[pipeline_analyzer::KEY_PAGE] = page;
  data[pipeline_analyzer::KEY_PAGE_SIZE] = pageSize;
  return data;
}

Json::Value PipelineAnalyzerService::get(const std::string& analyzerId) {
  const auto rows = Sql::sync({}, dbClient,
      std::string("SELECT ") + pipeline_analyzer::COLUMNS + " " + pipeline_analyzer::FROM_MEDIA +
          "WHERE pipeline_analyzer.id = $1::uuid",
      analyzerId);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value PipelineAnalyzerService::create(const Json::Value& name,
                                            const Json::Value& annotations,
                                            const Json::Value& mediaId) {
  Json::Value data = pipeline_analyzer::ROW();
  data[pipeline_analyzer::KEY_NAME] = name.isNull() ? data[pipeline_analyzer::KEY_NAME] : name;
  data[pipeline_analyzer::KEY_ANNOTATIONS] =
      annotations.isNull() ? data[pipeline_analyzer::KEY_ANNOTATIONS] : annotations;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const std::string annotationsText = Json::writeString(builder, data[pipeline_analyzer::KEY_ANNOTATIONS]);
  const std::string media = mediaId.isString() ? mediaId.asString() : "";
  const auto rows = Sql::sync({}, dbClient,
      "WITH saved AS ("
      "INSERT INTO pipeline_analyzer (id, name, media_id, annotations, updated_at) "
      "VALUES (gen_random_uuid(), $1, NULLIF($2, '')::uuid, $3::jsonb, now()) "
      "RETURNING id, name, media_id, annotations, updated_at) " +
          std::string(pipeline_analyzer::SELECT_SAVED),
      data[pipeline_analyzer::KEY_NAME].asString(),
      media,
      annotationsText);
  data = row2json(rows[0]);
  return data;
}

Json::Value PipelineAnalyzerService::update(const std::string& analyzerId,
                                            const Json::Value& name,
                                            const Json::Value& annotations,
                                            const Json::Value& mediaId) {
  Json::Value data = pipeline_analyzer::ROW();
  data[pipeline_analyzer::KEY_NAME] = name.isNull() ? data[pipeline_analyzer::KEY_NAME] : name;
  data[pipeline_analyzer::KEY_ANNOTATIONS] =
      annotations.isNull() ? data[pipeline_analyzer::KEY_ANNOTATIONS] : annotations;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const std::string annotationsText = Json::writeString(builder, data[pipeline_analyzer::KEY_ANNOTATIONS]);
  const std::string media = mediaId.isString() ? mediaId.asString() : "";
  const auto rows = Sql::sync({}, dbClient,
      "WITH saved AS ("
      "UPDATE pipeline_analyzer SET name = $2, annotations = $3::jsonb, "
      "media_id = COALESCE(NULLIF($4, '')::uuid, media_id), updated_at = now() "
      "WHERE id = $1::uuid RETURNING id, name, media_id, annotations, updated_at) " +
          std::string(pipeline_analyzer::SELECT_SAVED),
      analyzerId,
      data[pipeline_analyzer::KEY_NAME].asString(),
      annotationsText,
      media);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_NOT_FOUND);
  }
  data = row2json(rows[0]);
  return data;
}

Json::Value PipelineAnalyzerService::remove(const std::string& analyzerId) {
  const auto used = Sql::sync({}, dbClient,
      "SELECT id FROM pipeline WHERE $1::uuid = ANY(analyzer_ids) LIMIT 1",
      analyzerId);
  if (!used.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_OCCUPIED);
  }
  const auto rows = Sql::sync({}, dbClient,
      "WITH saved AS ("
      "DELETE FROM pipeline_analyzer WHERE id = $1::uuid "
      "RETURNING id, name, media_id, annotations, updated_at) " +
          std::string(pipeline_analyzer::SELECT_SAVED),
      analyzerId);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value PipelineAnalyzerService::row2json(const drogon::orm::Row& row) {
  Json::Value annotations;
  Json::Reader().parse(row["annotations"].as<std::string>(), annotations);
  Json::Value data = pipeline_analyzer::ROW();
  data[pipeline_analyzer::KEY_ID] = row["id"].as<std::string>();
  data[pipeline_analyzer::KEY_NAME] = row["name"].as<std::string>();
  data[pipeline_analyzer::KEY_MEDIA_ID] = row["media_id"].isNull() ? Json::Value() : row["media_id"].as<std::string>();
  data[pipeline_analyzer::KEY_MEDIA_NAME] =
      row["media_name"].isNull() ? Json::Value() : row["media_name"].as<std::string>();
  data[pipeline_analyzer::KEY_WIDTH] = row["width"].isNull() ? Json::Value() : row["width"].as<int>();
  data[pipeline_analyzer::KEY_HEIGHT] = row["height"].isNull() ? Json::Value() : row["height"].as<int>();
  data[pipeline_analyzer::KEY_ANNOTATIONS] = annotations;
  data[pipeline_analyzer::KEY_UPDATED_AT] = row["updated_at"].as<std::string>();
  return data;
}
