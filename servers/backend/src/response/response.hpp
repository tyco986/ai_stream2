#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace http {

class Response {
 public:
  static drogon::HttpResponsePtr ok(const Json::Value& data = Json::Value(Json::objectValue),
                                    const char* message = "");
  static drogon::HttpResponsePtr error(drogon::HttpStatusCode status, const char* message);

 private:
  static drogon::HttpResponsePtr envelope(bool success,
                                          drogon::HttpStatusCode status,
                                          const char* message,
                                          const Json::Value& data);
};

}
