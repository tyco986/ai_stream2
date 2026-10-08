#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>
#include <string>

class Proxy {
 public:
  static void forward(const std::string& upstreamBase,
                      const drogon::HttpRequestPtr& request,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  static void complete(const std::function<void(const drogon::HttpResponsePtr&)>& callback,
                       drogon::ReqResult result,
                       const drogon::HttpResponsePtr& upstream);
};
