#include "db/database.hpp"

#include "constants/database.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

drogon::orm::DbClientPtr Database::sharedClient;

drogon::orm::DbClientPtr Database::client() {
  return sharedClient;
}

void Database::migrate() {
  connect();
  applyPending();
}

void Database::connect() {
  for (int attempt = 0; attempt < database::CONNECT_ATTEMPTS && !dbClient; ++attempt) {
    try {
      auto client = drogon::orm::DbClient::newPgClient(database::CONNECTION_INFO, 1);
      client->execSqlSync("SELECT 1");
      dbClient = std::move(client);
      sharedClient = dbClient;
    } catch (const std::exception&) {
      std::this_thread::sleep_for(std::chrono::seconds(database::CONNECT_DELAY_SECONDS));
    }
  }
  if (!dbClient) {
    throw std::runtime_error("postgresql unreachable");
  }
}

void Database::applyPending() {
  dbClient->execSqlSync(
      "CREATE TABLE IF NOT EXISTS schema_migrations (version VARCHAR PRIMARY KEY)");
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(database::MIGRATIONS_DIR)) {
    if (entry.path().extension() == ".sql") {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  for (const auto& file : files) {
    applyFile(file);
  }
}

void Database::applyFile(const std::filesystem::path& path) {
  const auto version = path.filename().string();
  const auto existing = dbClient->execSqlSync(
      "SELECT version FROM schema_migrations WHERE version = $1", version);
  if (existing.empty()) {
    auto transaction = dbClient->newTransaction();
    try {
      for (const auto& statement : statements(path)) {
        transaction->execSqlSync(statement);
      }
      transaction->execSqlSync("INSERT INTO schema_migrations (version) VALUES ($1)", version);
    } catch (...) {
      transaction->rollback();
      throw;
    }
  }
}

std::vector<std::string> Database::statements(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("migration file unreadable");
  }
  std::stringstream buffer;
  buffer << input.rdbuf();
  std::vector<std::string> result;
  std::string statement;
  std::stringstream source(buffer.str());
  while (std::getline(source, statement, ';')) {
    const auto begin = statement.find_first_not_of(" \t\r\n");
    if (begin != std::string::npos) {
      result.push_back(statement.substr(begin));
    }
  }
  return result;
}
