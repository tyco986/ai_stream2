#pragma once

#include <drogon/orm/DbClient.h>

#include <filesystem>
#include <string>
#include <vector>

class Database {
 public:
  void migrate();
  static drogon::orm::DbClientPtr client();

 private:
  void connect();
  void applyPending();
  void applyFile(const std::filesystem::path& path);
  std::vector<std::string> statements(const std::filesystem::path& path);

  drogon::orm::DbClientPtr dbClient;
  static drogon::orm::DbClientPtr sharedClient;
};
