#include "services/device_service.hpp"

#include "constants/device_const.hpp"

#include <nvml.h>

#include <stdexcept>
#include <string>

namespace {

void requireNvml(nvmlReturn_t code) {
  if (code != NVML_SUCCESS) {
    throw std::runtime_error(nvmlErrorString(code));
  }
}

class NvmlSession {
 public:
  NvmlSession() {
    requireNvml(nvmlInit_v2());
  }

  ~NvmlSession() {
    nvmlShutdown();
  }
};

}

Json::Value DeviceService::list() {
  NvmlSession session;
  unsigned int count = 0;
  requireNvml(nvmlDeviceGetCount_v2(&count));
  Json::Value data(Json::objectValue);
  for (unsigned int index = 0; index < count; ++index) {
    nvmlDevice_t handle = nullptr;
    requireNvml(nvmlDeviceGetHandleByIndex_v2(index, &handle));
    char name[device::NAME_BUFFER_SIZE] = {};
    requireNvml(nvmlDeviceGetName(handle, name, device::NAME_BUFFER_SIZE));
    nvmlMemory_t memory{};
    requireNvml(nvmlDeviceGetMemoryInfo(handle, &memory));
    nvmlUtilization_t utilization{};
    requireNvml(nvmlDeviceGetUtilizationRates(handle, &utilization));
    Json::Value item(Json::objectValue);
    item[device::KEY_NAME] = name;
    item[device::KEY_MEMORY_TOTAL] = static_cast<Json::UInt64>(memory.total);
    item[device::KEY_MEMORY_USED] = static_cast<Json::UInt64>(memory.used);
    item[device::KEY_MEMORY_UTILIZATION] = utilization.memory;
    item[device::KEY_GPU_UTILIZATION] = utilization.gpu;
    data[std::to_string(index)] = item;
  }
  return data;
}
