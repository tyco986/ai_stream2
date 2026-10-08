#include "proxy/proxy.hpp"

#include "response/response.hpp"

#include <drogon/HttpClient.h>

void Proxy::forward(const std::string& upstreamBase,
                    const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto client = drogon::HttpClient::newHttpClient(upstreamBase);
  auto upstream = drogon::HttpRequest::newHttpRequest();
  upstream->setMethod(request->method());
  upstream->setPath(std::string(request->path()));
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
      [callback = std::move(callback), client](drogon::ReqResult result,
                                               const drogon::HttpResponsePtr& response) {
        Proxy::complete(callback, result, response);
      });
}

void Proxy::complete(const std::function<void(const drogon::HttpResponsePtr&)>& callback,
                     drogon::ReqResult result,
                     const drogon::HttpResponsePtr& upstream) {
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k502BadGateway, "upstream unreachable");
  if (result == drogon::ReqResult::Ok && upstream) {
    response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(upstream->statusCode());
    response->setBody(std::string(upstream->body()));
    const auto contentType = upstream->getHeader("content-type");
    if (!contentType.empty()) {
      response->addHeader("content-type", contentType);
    }
  }
  callback(response);
}
