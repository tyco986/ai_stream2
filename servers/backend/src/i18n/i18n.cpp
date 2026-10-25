#include "i18n/i18n.hpp"

#include "constants/i18n_const.hpp"

#include <fstream>
#include <stdexcept>

#include <json/json.h>

std::unordered_map<std::string, std::unordered_map<std::string, std::string>> I18n::catalog;
thread_local std::string I18n::current = i18n::LANG_EN;

void I18n::load(const std::string& directory) {
  for (const char* language : {i18n::LANG_EN, i18n::LANG_ZH}) {
    std::ifstream input(directory + "/" + language + ".json");
    Json::Value root;
    std::string errors;
    const bool parsed = Json::parseFromStream(Json::CharReaderBuilder(), input, &root, &errors);
    if (!parsed) {
      throw std::runtime_error(errors);
    }
    std::unordered_map<std::string, std::string> table;
    for (const auto& name : root.getMemberNames()) {
      table.emplace(name, root[name].asString());
    }
    catalog.insert_or_assign(language, std::move(table));
  }
}

void I18n::bind(const drogon::HttpRequestPtr& request) {
  language(request->getHeader(i18n::HEADER));
}

void I18n::language(const std::string& header) {
  const auto comma = header.find(',');
  const std::string first = header.substr(0, comma == std::string::npos ? header.size() : comma);
  const auto semi = first.find(';');
  const std::string tag = first.substr(0, semi == std::string::npos ? first.size() : semi);
  current = tag.rfind("zh", 0) == 0 ? i18n::LANG_ZH : i18n::LANG_EN;
}

const char* I18n::text(const char* key) {
  const auto selected = catalog.find(current);
  const auto& table = (selected != catalog.end() ? selected : catalog.find(i18n::LANG_EN))->second;
  const auto found = table.find(key);
  const char* message = found == table.end() ? key : found->second.c_str();
  return message;
}
