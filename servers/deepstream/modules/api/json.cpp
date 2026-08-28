#include "json.hpp"

#include <cctype>
#include <string>

std::string YamlJson::escape(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (char ch : text) {
    if (ch == '\\') {
      out += "\\\\";
    } else if (ch == '"') {
      out += "\\\"";
    } else if (ch == '\n') {
      out += "\\n";
    } else if (ch == '\r') {
      out += "\\r";
    } else if (ch == '\t') {
      out += "\\t";
    } else {
      out += ch;
    }
  }
  return out;
}

bool YamlJson::isJsonNumber(const std::string& text) {
  bool ok = !text.empty();
  size_t i = 0;
  bool dot = false;
  bool digit = false;
  if (ok && (text[0] == '-' || text[0] == '+')) {
    i = 1;
    ok = text.size() > 1;
  }
  for (; ok && i < text.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    if (text[i] == '.' && !dot) {
      dot = true;
    } else if (std::isdigit(ch) != 0) {
      digit = true;
    } else {
      ok = false;
    }
  }
  ok = ok && digit;
  return ok;
}

std::string YamlJson::dumpScalar(const YAML::Node& node) {
  const std::string text = node.Scalar();
  std::string result;
  if (text == "true" || text == "false") {
    result = text;
  } else if (text == "null" || text == "~") {
    result = "null";
  } else if (isJsonNumber(text)) {
    result = text;
  } else {
    result = "\"" + escape(text) + "\"";
  }
  return result;
}

std::string YamlJson::dumpSequence(const YAML::Node& node) {
  std::string result = "[";
  bool first = true;
  for (const YAML::Node& item : node) {
    if (!first) {
      result += ",";
    }
    first = false;
    result += dump(item);
  }
  result += "]";
  return result;
}

std::string YamlJson::dumpMap(const YAML::Node& node) {
  std::string result = "{";
  bool first = true;
  for (auto it = node.begin(); it != node.end(); ++it) {
    if (!first) {
      result += ",";
    }
    first = false;
    result += dump(it->first);
    result += ":";
    result += dump(it->second);
  }
  result += "}";
  return result;
}

std::string YamlJson::dump(const YAML::Node& node) {
  std::string result = "null";
  if (node && !node.IsNull()) {
    if (node.IsScalar()) {
      result = dumpScalar(node);
    } else if (node.IsSequence()) {
      result = dumpSequence(node);
    } else if (node.IsMap()) {
      result = dumpMap(node);
    }
  }
  return result;
}

std::string YamlJson::envelope(bool success, const std::string& message,
                               const std::string& data_json) {
  std::string body = "{\"success\":";
  body += success ? "true" : "false";
  body += ",\"message\":\"";
  body += escape(message);
  body += "\",\"data\":";
  body += data_json.empty() ? "null" : data_json;
  body += "}";
  return body;
}
