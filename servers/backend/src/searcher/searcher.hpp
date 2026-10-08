#pragma once

#include <string>
#include <vector>

class Searcher {
 public:
  static std::string clause(const std::vector<const char*>& columns, int parameter);
};
