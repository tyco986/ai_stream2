#include "services/pipeline_analyzer_service.hpp"

#include "constants/pipeline_analyzer_const.hpp"
#include "searcher/searcher.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

PipelineAnalyzerService::PipelineAnalyzerService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value PipelineAnalyzerService::list(const std::string& search, int page, int pageSize) {
  const int offset = (page - 1) * pageSize;
  const std::string match = Searcher::clause({"id", "name", "source_path", "annotations"}, 1);
  const std::string from = "FROM pipeline_analyzer WHERE " + match + " ";
  const auto counted = dbClient->execSqlSync("SELECT count(*) AS total " + from, search);
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, name, source_path, annotations::text AS annotations, updated_at " + from +
          "ORDER BY name LIMIT $2 OFFSET $3",
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
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, name, source_path, annotations::text AS annotations, updated_at "
      "FROM pipeline_analyzer WHERE id = $1::uuid",
      analyzerId);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value PipelineAnalyzerService::create(const Json::Value& name,
                                            const Json::Value& annotations,
                                            const std::string& fileName,
                                            const std::string& fileContent) {
  const bool hasFile = !fileContent.empty();
  Json::Value data = pipeline_analyzer::ROW();
  data[pipeline_analyzer::KEY_NAME] = name.isNull() ? data[pipeline_analyzer::KEY_NAME] : name;
  data[pipeline_analyzer::KEY_ANNOTATIONS] =
      annotations.isNull() ? data[pipeline_analyzer::KEY_ANNOTATIONS] : annotations;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const std::string annotationsText = Json::writeString(builder, data[pipeline_analyzer::KEY_ANNOTATIONS]);
  auto rows = dbClient->execSqlSync(
      "INSERT INTO pipeline_analyzer (id, name, source_path, annotations, updated_at) "
      "VALUES (gen_random_uuid(), $1, NULL, $2::jsonb, now()) "
      "RETURNING id::text AS id, name, source_path, annotations::text AS annotations, updated_at",
      data[pipeline_analyzer::KEY_NAME].asString(),
      annotationsText);
  const std::string id = rows[0]["id"].as<std::string>();
  if (hasFile) {
    rows = dbClient->execSqlSync(
        "UPDATE pipeline_analyzer SET source_path = $2 WHERE id = $1::uuid "
        "RETURNING id::text AS id, name, source_path, annotations::text AS annotations, updated_at",
        id,
        saveFile(id, fileName, fileContent));
  }
  data = row2json(rows[0]);
  return data;
}

Json::Value PipelineAnalyzerService::update(const std::string& analyzerId,
                                            const Json::Value& name,
                                            const Json::Value& annotations,
                                            const std::string& fileName,
                                            const std::string& fileContent) {
  const bool hasFile = !fileContent.empty();
  Json::Value data = pipeline_analyzer::ROW();
  data[pipeline_analyzer::KEY_NAME] = name.isNull() ? data[pipeline_analyzer::KEY_NAME] : name;
  data[pipeline_analyzer::KEY_ANNOTATIONS] =
      annotations.isNull() ? data[pipeline_analyzer::KEY_ANNOTATIONS] : annotations;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const std::string annotationsText = Json::writeString(builder, data[pipeline_analyzer::KEY_ANNOTATIONS]);
  auto rows = dbClient->execSqlSync(
      "UPDATE pipeline_analyzer SET name = $2, annotations = $3::jsonb, updated_at = now() "
      "WHERE id = $1::uuid "
      "RETURNING id::text AS id, name, source_path, annotations::text AS annotations, updated_at",
      analyzerId,
      data[pipeline_analyzer::KEY_NAME].asString(),
      annotationsText);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_NOT_FOUND);
  }
  const bool hasSource = !rows[0]["source_path"].isNull();
  const std::string oldPath = hasSource ? rows[0]["source_path"].as<std::string>() : "";
  if (hasFile) {
    const std::string sourcePath = saveFile(analyzerId, fileName, fileContent);
    if (hasSource && oldPath != sourcePath) {
      std::filesystem::remove(oldPath);
    }
    rows = dbClient->execSqlSync(
        "UPDATE pipeline_analyzer SET source_path = $2 WHERE id = $1::uuid "
        "RETURNING id::text AS id, name, source_path, annotations::text AS annotations, updated_at",
        analyzerId,
        sourcePath);
  }
  data = row2json(rows[0]);
  return data;
}

std::string PipelineAnalyzerService::saveFile(const std::string& analyzerId,
                                              const std::string& fileName,
                                              const std::string& fileContent) {
  const std::filesystem::path file(std::filesystem::path(fileName).filename());
  const std::string storedName = file.string();
  const bool plain = storedName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-") ==
      std::string::npos;
  const bool image = file.extension() == pipeline_analyzer::SUFFIX_JPG ||
      file.extension() == pipeline_analyzer::SUFFIX_JPEG || file.extension() == pipeline_analyzer::SUFFIX_PNG;
  if (!plain || !image) {
    throw std::runtime_error("invalid analyzer file");
  }
  const std::filesystem::path source =
      std::filesystem::path(pipeline_analyzer::SOURCE_DIR) / (analyzerId + file.extension().string());
  std::filesystem::create_directories(pipeline_analyzer::SOURCE_DIR);
  std::ofstream output(source, std::ios::binary);
  output.write(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
  output.close();
  if (!output) {
    throw std::runtime_error("analyzer file not saved");
  }
  return source.string();
}

Json::Value PipelineAnalyzerService::remove(const std::string& analyzerId) {
  const auto used = dbClient->execSqlSync(
      "SELECT id FROM pipeline WHERE $1::uuid = ANY(analyzer_ids) LIMIT 1", analyzerId);
  if (!used.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_OCCUPIED);
  }
  const auto rows = dbClient->execSqlSync(
      "DELETE FROM pipeline_analyzer WHERE id = $1::uuid "
      "RETURNING id::text AS id, name, source_path, annotations::text AS annotations, updated_at",
      analyzerId);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_analyzer::ERR_NOT_FOUND);
  }
  if (!rows[0]["source_path"].isNull()) {
    std::filesystem::remove(rows[0]["source_path"].as<std::string>());
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
  data[pipeline_analyzer::KEY_SOURCE_PATH] =
      row["source_path"].isNull() ? Json::Value() : row["source_path"].as<std::string>();
  data[pipeline_analyzer::KEY_ANNOTATIONS] = annotations;
  data[pipeline_analyzer::KEY_UPDATED_AT] = row["updated_at"].as<std::string>();
  return data;
}
