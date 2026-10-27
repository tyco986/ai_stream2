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
  auto client = drogon::HttpClient::newHttpClient(ffmpeg::BASE_URL);
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
