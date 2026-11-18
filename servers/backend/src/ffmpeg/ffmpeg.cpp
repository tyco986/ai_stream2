#include "ffmpeg/ffmpeg.hpp"

#include "constants/ffmpeg_const.hpp"
#include "constants/proxy_const.hpp"
#include "constants/response_const.hpp"

#include <drogon/HttpClient.h>
#include <json/json.h>

#include <exception>

drogon::Task<ProbeResult> Ffmpeg::rtspProbe(const std::string& url) {
  Json::Value body;
  Json::Value urls(Json::arrayValue);
  urls.append(url);
  body[ffmpeg_backend::KEY_URLS] = urls;
  auto request = drogon::HttpRequest::newHttpJsonRequest(body);
  request->setMethod(drogon::Post);
  request->setPath(ffmpeg_backend::RTSP_PROBE_PATH);
  auto client = drogon::HttpClient::newHttpClient(ffmpeg::DES_BASE_URL);
  ProbeResult result;
  result.accepted = false;
  try {
    const auto response = co_await client->sendRequestCoro(request, ffmpeg_backend::REQUEST_TIMEOUT_SECONDS);
    const bool httpOk = response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
    const auto payload = httpOk ? response->getJsonObject() : nullptr;
    const Json::Value payloadData = payload ? (*payload)[http::KEY_DATA] : Json::Value();
    result.data = payloadData.isMember(url) ? payloadData[url] : Json::Value(Json::objectValue);
    result.accepted = result.data.isMember(ffmpeg_backend::KEY_STATUS);
  } catch (const std::exception&) {
    result.accepted = false;
  }
  co_return result;
}

drogon::Task<ProbeResult> Ffmpeg::rtspPublished() {
  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Get);
  request->setPath(ffmpeg_backend::RTSP_PUBLISH_PATH);
  auto client = drogon::HttpClient::newHttpClient(ffmpeg::DES_BASE_URL);
  ProbeResult result;
  result.accepted = false;
  try {
    const auto response = co_await client->sendRequestCoro(request, ffmpeg_backend::REQUEST_TIMEOUT_SECONDS);
    const bool httpOk = response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
    const auto payload = httpOk ? response->getJsonObject() : nullptr;
    const Json::Value payloadData = payload ? (*payload)[http::KEY_DATA] : Json::Value();
    result.data = payloadData.isObject() ? payloadData : Json::Value(Json::objectValue);
    result.accepted = httpOk && payloadData.isObject();
  } catch (const std::exception&) {
    result.accepted = false;
  }
  co_return result;
}

drogon::Task<ProbeResult> Ffmpeg::rtspPublish(const std::string& name, const std::string& path) {
  Json::Value paths(Json::objectValue);
  paths[name] = path;
  Json::Value body;
  body[ffmpeg_backend::KEY_PATHS] = paths;
  auto request = drogon::HttpRequest::newHttpJsonRequest(body);
  request->setMethod(drogon::Post);
  request->setPath(ffmpeg_backend::RTSP_PUBLISH_PATH);
  auto client = drogon::HttpClient::newHttpClient(ffmpeg::DES_BASE_URL);
  ProbeResult result;
  result.accepted = false;
  try {
    const auto response = co_await client->sendRequestCoro(request, ffmpeg_backend::PUBLISH_TIMEOUT_SECONDS);
    const bool httpOk = response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
    const auto payload = httpOk ? response->getJsonObject() : nullptr;
    const Json::Value payloadData = payload ? (*payload)[http::KEY_DATA] : Json::Value();
    result.data = payloadData.isMember(name) ? payloadData[name] : Json::Value(Json::objectValue);
    result.accepted = payloadData.isMember(name);
  } catch (const std::exception&) {
    result.accepted = false;
  }
  co_return result;
}

drogon::Task<ProbeResult> Ffmpeg::rtspUnpublish(const std::string& name) {
  auto client = drogon::HttpClient::newHttpClient(ffmpeg::DES_BASE_URL);
  auto listedRequest = drogon::HttpRequest::newHttpRequest();
  listedRequest->setMethod(drogon::Get);
  listedRequest->setPath(ffmpeg_backend::RTSP_PUBLISH_PATH);
  listedRequest->setParameter(ffmpeg_backend::KEY_NAMES, name);
  ProbeResult result;
  result.accepted = false;
  bool listed = false;
  try {
    const auto listedResponse =
        co_await client->sendRequestCoro(listedRequest, ffmpeg_backend::REQUEST_TIMEOUT_SECONDS);
    const bool listedOk = listedResponse->statusCode() >= drogon::k200OK &&
        listedResponse->statusCode() < drogon::k300MultipleChoices;
    const auto listedPayload = listedOk ? listedResponse->getJsonObject() : nullptr;
    const Json::Value listedData = listedPayload ? (*listedPayload)[http::KEY_DATA] : Json::Value();
    listed = listedData.isMember(name);
    result.data = listed ? listedData[name] : Json::Value(Json::objectValue);
  } catch (const std::exception&) {
    listed = false;
  }
  if (listed) {
    auto removeRequest = drogon::HttpRequest::newHttpRequest();
    removeRequest->setMethod(drogon::Delete);
    removeRequest->setPath(ffmpeg_backend::RTSP_PUBLISH_PATH);
    removeRequest->setParameter(ffmpeg_backend::KEY_NAMES, name);
    try {
      const auto removedResponse =
          co_await client->sendRequestCoro(removeRequest, ffmpeg_backend::REQUEST_TIMEOUT_SECONDS);
      const bool removedOk = removedResponse->statusCode() >= drogon::k200OK &&
          removedResponse->statusCode() < drogon::k300MultipleChoices;
      const auto removedPayload = removedOk ? removedResponse->getJsonObject() : nullptr;
      const Json::Value removedData = removedPayload ? (*removedPayload)[http::KEY_DATA] : Json::Value();
      result.accepted = removedOk && !removedData.isMember(name);
    } catch (const std::exception&) {
      result.accepted = false;
    }
  }
  co_return result;
}
