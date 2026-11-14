#include "proxy/proxy.hpp"

#include "constants/proxy_const.hpp"
#include "response/response.hpp"

#include <drogon/HttpClient.h>

#include <array>
#include <chrono>
#include <utility>

ProxyLogger Proxy::logger;

void Proxy::forward(const std::string& upstreamBase,
                    const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  auto client = drogon::HttpClient::newHttpClient(upstreamBase);
  auto upstream = drogon::HttpRequest::newHttpRequest();
  upstream->setMethod(request->method());
  upstream->setPath(Proxy::upstreamPath(std::string(request->path())));
  for (const auto& parameter : request->getParameters()) {
    upstream->setParameter(parameter.first, parameter.second);
  }
  upstream->setBody(std::string(request->body()));
  const auto contentType = request->getHeader("content-type");
  if (!contentType.empty()) {
    upstream->addHeader("content-type", contentType);
  }
  client->sendRequest(
      upstream,
      [request, started, callback = std::move(callback), client](drogon::ReqResult result,
                                                                const drogon::HttpResponsePtr& response) {
        Proxy::complete(request, started, callback, result, response);
      });
}

std::string Proxy::upstreamPath(const std::string& path) {
  const std::array<std::pair<const char*, const char*>, 6> rewrites = {{
      {deepstream::SRC_PREFIX, deepstream::DES_PREFIX},
      {generator::SRC_PREFIX, generator::DES_PREFIX},
      {ffmpeg::SRC_PREFIX, ffmpeg::DES_PREFIX},
      {export_trt::SRC_PREFIX, export_trt::DES_PREFIX},
      {export_onnx::SRC_PREFIX, export_onnx::DES_PREFIX},
      {mediamtx::SRC_PREFIX, mediamtx::DES_PREFIX}}};
  std::string rewritten = path;
  for (const auto& rewrite : rewrites) {
    const std::string srcPrefix(rewrite.first);
    const bool matched = path.rfind(srcPrefix, 0) == 0;
    rewritten = matched ? std::string(rewrite.second) + path.substr(srcPrefix.size()) : rewritten;
  }
  const std::string result = rewritten.empty() ? std::string("/") : rewritten;
  return result;
}

void Proxy::complete(const drogon::HttpRequestPtr& request,
                     std::chrono::steady_clock::time_point started,
                     const std::function<void(const drogon::HttpResponsePtr&)>& callback,
                     drogon::ReqResult result,
                     const drogon::HttpResponsePtr& upstream) {
  const bool ok = result == drogon::ReqResult::Ok && upstream;
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k502BadGateway, proxy::MSG_UPSTREAM_UNREACHABLE);
  if (ok) {
    response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(upstream->statusCode());
    response->setBody(std::string(upstream->body()));
    const auto contentType = upstream->getHeader("content-type");
    if (!contentType.empty()) {
      response->addHeader("content-type", contentType);
    }
  }
  const std::string error = ok ? "" : proxy::MSG_UPSTREAM_UNREACHABLE;
  logger.record(request, response, started, error);
  callback(response);
}
