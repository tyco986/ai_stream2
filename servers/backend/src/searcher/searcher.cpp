#include "searcher/searcher.hpp"

std::string Searcher::clause(const std::vector<const char*>& columns, int parameter) {
  const std::string holder = "$" + std::to_string(parameter);
  std::string sql = "(" + holder + " = ''";
  for (const auto* column : columns) {
    sql += " OR " + std::string(column) + "::text ILIKE '%' || " + holder + " || '%'";
  }
  sql += ")";
  return sql;
}
