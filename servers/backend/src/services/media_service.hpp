#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <ctime>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

class MediaService {
 public:
  explicit MediaService(drogon::orm::DbClientPtr dbClient);
  Json::Value listUploadFile(int page, long pageSize, const std::string& userId, const std::string& type, const std::string& search);
  Json::Value listPipelineFile(int page, long pageSize, const std::string& type, const std::string& search, const std::string& pipelineId);
  Json::Value listRecordingFile(int page, long pageSize, const std::string& type, const std::string& search, const std::string& streamId);
  Json::Value listUploadFolder();
  Json::Value listPipelineFolder();
  Json::Value listRecordingFolder();
  Json::Value get(const std::string& mediaId);
  std::filesystem::path mediaId2Path(const std::string& mediaId);
  Json::Value create(const Json::Value& type,
                     const Json::Value& name,
                     const Json::Value& width,
                     const Json::Value& height,
                     const Json::Value& duration,
                     const Json::Value& fps,
                     const Json::Value& codec,
                     const Json::Value& size,
                     const Json::Value& md5,
                     const Json::Value& folder,
                     const Json::Value& createdAt,
                     const Json::Value& updatedAt,
                     const Json::Value& userId,
                     const Json::Value& streamId,
                     const Json::Value& pipelineId);
  Json::Value removeFile(const std::string& mediaId);
  Json::Value remove(const std::string& mediaId);
  Json::Value update(const Json::Value& type,
                     const Json::Value& name,
                     const Json::Value& width,
                     const Json::Value& height,
                     const Json::Value& duration,
                     const Json::Value& fps,
                     const Json::Value& codec,
                     const Json::Value& size,
                     const Json::Value& md5,
                     const Json::Value& folder,
                     const Json::Value& createdAt,
                     const Json::Value& updatedAt,
                     const Json::Value& userId,
                     const Json::Value& streamId,
                     const Json::Value& pipelineId,
                     const std::string& mediaId);
  Json::Value upload(const std::string& userId, const std::string& fileName, const std::string& fileContent);

 private:
  std::string formatTime(std::time_t seconds, const char* format);
  Json::Value fileNode(const std::filesystem::path& path, const std::string& folder);
  std::string fileMd5(const std::string& bytes);
  std::string userId2Name(const std::string& userId);
  std::string streamId2Name(const std::string& streamId);
  std::string pipelineId2Name(const std::string& pipelineId);
  void reloadFile(const std::string& folder, const std::string& ownerColumn, const std::string& ownerId);
  Json::Value listFolder(const std::string& folder, const std::string& sql);
  Json::Value listFile(const std::string& folder,
                     const std::string& ownerColumn,
                     const std::string& ownerId,
                     const std::string& type,
                     int page,
                     long pageSize,
                     const std::string& search);
  std::optional<std::string> uuidOrEmpty(const Json::Value& value);
  std::filesystem::path createUploadFolder(const std::string& userName);
  void probeFile(const std::filesystem::path& path,
                 bool image,
                 std::optional<int>& width,
                 std::optional<int>& height,
                 std::optional<double>& duration,
                 std::optional<double>& fps,
                 std::optional<std::string>& codec);
  void readProbe(void* handle,
                 bool image,
                 std::optional<int>& width,
                 std::optional<int>& height,
                 std::optional<double>& duration,
                 std::optional<double>& fps,
                 std::optional<std::string>& codec);
  std::string lowerExtension(const std::string& fileName);
  bool suffixMatched(const std::string& extension, std::span<const char* const> suffixes);
  void writeUpload(const std::filesystem::path& folder, const std::string& fileName, const std::string& fileContent);
  std::string getType(const std::string& fileName);
  Json::Value mediaRow(const drogon::orm::Row& row);
  Json::Value formatResult(Json::Value data);
  std::string formatDuration(double seconds);

  drogon::orm::DbClientPtr dbClient;
  std::string root;
};
