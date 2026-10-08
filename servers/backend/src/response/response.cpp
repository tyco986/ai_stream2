#include "response/response.hpp"

#include "constants/response.hpp"

drogon::HttpResponsePtr http::Response::ok(const Json::Value& data, const char* message) {
  auto response = envelope(true, drogon::k200OK, message, data);
  return response;
}

drogon::HttpResponsePtr http::Response::error(drogon::HttpStatusCode status,
                                              const char* message) {
  auto response = envelope(false, status, message, Json::Value(Json::nullValue));
  return response;
}

drogon::HttpResponsePtr http::Response::envelope(bool success,
                                                 drogon::HttpStatusCode status,
                                                 const char* message,
                                                 const Json::Value& data) {
  Json::Value body;
  body[KEY_SUCCESS] = success;
  body[KEY_MESSAGE] = message;
  body[KEY_DATA] = data;
  auto response = drogon::HttpResponse::newHttpJsonResponse(body);
  response->setStatusCode(status);
  return response;
}
