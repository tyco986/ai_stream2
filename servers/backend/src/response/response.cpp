#include "response/response.hpp"

#include "constants/response_const.hpp"
#include "i18n/i18n.hpp"

Json::StreamWriterBuilder http::Response::makeWriter() {
  Json::StreamWriterBuilder builder;
  builder[JSON_EMIT_UTF8] = true;
  builder[JSON_INDENTATION] = "";
  return builder;
}

Json::StreamWriterBuilder http::Response::writer = http::Response::makeWriter();

drogon::HttpResponsePtr http::Response::ok(const Json::Value& data, const char* message) {
  auto response = envelope(true, drogon::k200OK, message, data);
  return response;
}

drogon::HttpResponsePtr http::Response::error(drogon::HttpStatusCode status,
                                              const char* message,
                                              const Json::Value& data) {
  auto response = envelope(false, status, message, data);
  return response;
}

drogon::HttpResponsePtr http::Response::envelope(bool success,
                                                 drogon::HttpStatusCode status,
                                                 const char* message,
                                                 const Json::Value& data) {
  Json::Value body;
  body[KEY_SUCCESS] = success;
  body[KEY_MESSAGE] = message == nullptr || message[0] == '\0' ? "" : I18n::text(message);
  body[KEY_DATA] = data;
  auto response = drogon::HttpResponse::newHttpResponse();
  response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  response->setBody(Json::writeString(writer, body));
  response->setStatusCode(status);
  return response;
}
