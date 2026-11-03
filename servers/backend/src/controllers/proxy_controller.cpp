#include "constants/proxy_const.hpp"
#include "proxy/proxy.hpp"

#include <drogon/drogon.h>

#include <string>

void registerUpstream(const std::string& srcPrefix, const std::string& desBaseUrl) {
  drogon::app().registerHandlerViaRegex(
      srcPrefix + "/.*",
      [desBaseUrl](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Proxy::forward(desBaseUrl, request, std::move(callback));
      },
      {drogon::Get, drogon::Post, drogon::Patch, drogon::Delete},
      {"AuthFilter"});
}

class ProxyRoutes {
 public:
  ProxyRoutes() {
    registerUpstream(deepstream::SRC_PREFIX, deepstream::DES_BASE_URL);
    registerUpstream(generator::SRC_PREFIX, generator::DES_BASE_URL);
    registerUpstream(ffmpeg::SRC_PREFIX, ffmpeg::DES_BASE_URL);
    registerUpstream(export_trt::SRC_PREFIX, export_trt::DES_BASE_URL);
    registerUpstream(export_onnx::SRC_PREFIX, export_onnx::DES_BASE_URL);
    registerUpstream(mediamtx::SRC_PREFIX, mediamtx::DES_BASE_URL);
  }
};

static ProxyRoutes proxyRoutes;
