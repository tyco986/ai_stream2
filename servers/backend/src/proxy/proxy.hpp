#pragma once

#include "logger/proxy_logger.hpp"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <chrono>
#include <functional>
#include <string>

class Proxy {
 public:
  static void forward(const std::string& upstreamBase,
                      const drogon::HttpRequestPtr& request,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  static std::string upstreamPath(const std::string& path);
  static void complete(const drogon::HttpRequestPtr& request,
                       std::chrono::steady_clock::time_point started,
                       const std::function<void(const drogon::HttpResponsePtr&)>& callback,
                       drogon::ReqResult result,
                       const drogon::HttpResponsePtr& upstream);

  static ProxyLogger logger;
};
