#pragma once

#include "constants/device_const.hpp"
#include "logger/device_logger.hpp"
#include "services/node_service.hpp"

#include <drogon/HttpController.h>
#include <json/json.h>

#include <functional>
#include <string>

class NodeController : public drogon::HttpController<NodeController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(NodeController::getNodesInfo, device::PATH_LIST, drogon::Get, "AuthFilter");
  METHOD_LIST_END

  void getNodesInfo(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  std::string getBeszelToken();
  Json::Value getBeszelRecords(const std::string& collection, const std::string& query, const std::string& authToken);
  Json::Value requestBeszel(drogon::HttpMethod method,
                  const std::string& path,
                  const std::string& body,
                  const std::string& authToken);

  NodeService service;
  DeviceLogger logger;
};
