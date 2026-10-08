#include "constants/proxy.hpp"
#include "proxy/proxy.hpp"

#include <drogon/drogon.h>

#include <string>

void registerUpstream(const std::string& prefix, const std::string& baseUrl) {
  drogon::app().registerHandler(
      prefix + "/.*",
      [baseUrl](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Proxy::forward(baseUrl, request, std::move(callback));
      },
      {drogon::Get, drogon::Post},
      {"AuthFilter"});
}

class ProxyRoutes {
 public:
  ProxyRoutes() {
    registerUpstream(deepstream::PREFIX, deepstream::BASE_URL);
    registerUpstream(generator::PREFIX, generator::BASE_URL);
    registerUpstream(ffmpeg::PREFIX, ffmpeg::BASE_URL);
    registerUpstream(export_trt::PREFIX, export_trt::BASE_URL);
    registerUpstream(export_onnx::PREFIX, export_onnx::BASE_URL);
    registerUpstream(mediamtx::PREFIX, mediamtx::BASE_URL);
  }
};

static ProxyRoutes proxyRoutes;
