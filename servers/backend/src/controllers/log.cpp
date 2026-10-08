#include "controllers/log.hpp"

#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

bool LogController::dateValid(const std::string& date) {
  bool valid = date.size() == log::DATE_LENGTH;
  for (std::size_t index = 0; valid && index < date.size(); ++index) {
    const bool separator = index == 4 || index == 7;
    const unsigned char character = static_cast<unsigned char>(date[index]);
    valid = separator ? date[index] == '-' : std::isdigit(character) != 0;
  }
  return valid;
}

void LogController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const std::string date = request->getParameter(log::QUERY_DATE);
  const std::string page = request->getParameter(log::QUERY_PAGE);
  const bool knownPage = page == log::STREAM_DIR || page == log::STREAM_GROUP_DIR;
  const bool accepted = dateValid(date) && knownPage;
  std::string content;
  bool found = false;
  if (accepted) {
    const std::string path = std::string(log::DIR) + "/" + page + "/" + page + "_" + date + log::FILE_SUFFIX;
    std::ifstream input(path);
    found = static_cast<bool>(input);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    content = buffer.str();
  }
  Json::Value data;
  data[log::KEY_CONTENT] = content;
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k400BadRequest, log::MSG_GET_ERROR);
  if (accepted && found) {
    response = http::Response::ok(data, log::MSG_GET_OK);
  }
  if (accepted && !found) {
    response = http::Response::error(drogon::k404NotFound, log::MSG_GET_ERROR);
  }
  callback(std::move(response));
}

void LogController::enable(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  BaseLogger::setEnabled(true);
  callback(http::Response::ok(Json::Value(Json::objectValue), log::MSG_ENABLE_OK));
}
