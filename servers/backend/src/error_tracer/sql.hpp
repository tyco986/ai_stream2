#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

class SqlCall {
 public:
  std::source_location location = std::source_location::current();
};

class Sql {
 public:
  template <typename Client, typename... Args>
  static drogon::orm::Result sync(SqlCall call,
                                  const Client& client,
                                  const std::string& statement,
                                  Args&&... args) {
    std::optional<drogon::orm::Result> result;
    try {
      result = client->execSqlSync(statement, std::forward<Args>(args)...);
    } catch (const std::exception& error) {
      throw std::runtime_error(Sql::locate(call.location, error.what()));
    }
    return std::move(*result);
  }

  static std::string locate(const std::source_location& location, const char* message);
};
