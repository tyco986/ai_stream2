#pragma once

#include "constants/device_const.hpp"
#include "logger/device_logger.hpp"
#include "services/device_service.hpp"

#include <drogon/HttpController.h>

#include <functional>

class DeviceController : public drogon::HttpController<DeviceController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(DeviceController::list, device::PATH_LIST, drogon::Get, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  DeviceService service;
  DeviceLogger logger;
};
