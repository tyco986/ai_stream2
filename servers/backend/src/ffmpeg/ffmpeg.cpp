#include "ffmpeg/ffmpeg.hpp"

#include "constants/ffmpeg.hpp"
#include "constants/proxy.hpp"
#include "constants/response.hpp"

#include <drogon/HttpClient.h>
#include <json/json.h>

void Ffmpeg::probe(const std::string& url,
                  std::function<void(bool accepted, const Json::Value& data)> callback) {
  Json::Value body;
  body[ffmpeg_backend::KEY_URL] = url;
  auto request = drogon::HttpRequest::newHttpJsonRequest(body);
  request->setMethod(drogon::Post);
  request->setPath(ffmpeg_backend::PROBE_PATH);
  auto client = drogon::HttpClient::newHttpClient(ffmpeg::BASE_URL);
  client->sendRequest(
      request,
      [callback = std::move(callback), client](drogon::ReqResult result,
                                               const drogon::HttpResponsePtr& response) {
        const bool accepted = result == drogon::ReqResult::Ok && response &&
            response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
        const auto payload = accepted ? response->getJsonObject() : nullptr;
        const Json::Value data = payload ? (*payload)[http::KEY_DATA] : Json::Value();
        callback(accepted, data);
      },
      ffmpeg_backend::REQUEST_TIMEOUT_SECONDS);
}
