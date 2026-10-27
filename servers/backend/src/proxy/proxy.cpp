#include "proxy/proxy.hpp"

#include "constants/proxy_const.hpp"
#include "response/response.hpp"

#include <drogon/HttpClient.h>

#include <array>
#include <chrono>

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
  const std::array<const char*, 6> prefixes = {
      deepstream::PREFIX,
      generator::PREFIX,
      ffmpeg::PREFIX,
      export_trt::PREFIX,
      export_onnx::PREFIX,
      mediamtx::PREFIX};
  std::string stripped = path;
  for (const char* prefix : prefixes) {
    const std::string text(prefix);
    const bool matched = path.rfind(text, 0) == 0;
    stripped = matched ? path.substr(text.size()) : stripped;
  }
  const std::string result = stripped.empty() ? std::string("/") : stripped;
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
  logger.respond(request, response, started, callback, error);
}
