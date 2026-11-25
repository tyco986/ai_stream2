#include "services/node_service.hpp"

#include "constants/device_const.hpp"

#include <map>

Json::Value NodeService::byte2MB(const Json::Value& bytes) {
  const bool present = bytes.isNumeric();
  Json::Value value(Json::nullValue);
  if (present) {
    value = static_cast<Json::UInt64>(bytes.asUInt64() / device::BYTES_PER_MB);
  }
  return value;
}

Json::Value NodeService::formatUptime(const Json::Value& info) {
  const bool present = info.isMember("u") && info["u"].isNumeric();
  Json::Value uptime(Json::nullValue);
  if (present) {
    const auto seconds = info["u"].asUInt64();
    uptime = Json::Value(Json::objectValue);
    uptime[device::KEY_DAYS] = static_cast<Json::UInt64>(seconds / device::SECONDS_PER_DAY);
    uptime[device::KEY_HOURS] = static_cast<Json::UInt64>((seconds % device::SECONDS_PER_DAY) / device::SECONDS_PER_HOUR);
    uptime[device::KEY_MINUTES] = static_cast<Json::UInt64>((seconds % device::SECONDS_PER_HOUR) / device::SECONDS_PER_MINUTE);
  }
  return uptime;
}

Json::Value NodeService::textOrNull(const Json::Value& object, const char* key) {
  const bool present = object.isMember(key) && object[key].isString();
  Json::Value value(Json::nullValue);
  if (present) {
    value = object[key].asString();
  }
  return value;
}

Json::Value NodeService::intOrNull(const Json::Value& object, const char* key) {
  const bool present = object.isMember(key) && object[key].isNumeric();
  Json::Value value(Json::nullValue);
  if (present) {
    value = object[key].asInt();
  }
  return value;
}

Json::Value NodeService::formatGpu(const Json::Value& stats) {
  Json::Value gpus(Json::objectValue);
  const Json::Value source = stats.isMember("g") && stats["g"].isObject() ? stats["g"] : Json::Value(Json::objectValue);
  for (const auto& name : source.getMemberNames()) {
    const Json::Value& item = source[name];
    const double total = item.isMember("mt") && item["mt"].isNumeric() ? item["mt"].asDouble() : 0;
    const double used = item.isMember("mu") && item["mu"].isNumeric() ? item["mu"].asDouble() : 0;
    Json::Value gpu(Json::objectValue);
    gpu[device::KEY_NAME] = textOrNull(item, "n");
    gpu[device::KEY_MEMORY_TOTAL] = static_cast<Json::UInt64>(total + 0.5);
    gpu[device::KEY_MEMORY_USED] = static_cast<Json::UInt64>(used + 0.5);
    gpu[device::KEY_UTILIZATION] = item.isMember("u") && item["u"].isNumeric() ? item["u"].asDouble() : 0;
    gpus[name] = gpu;
  }
  return gpus;
}

Json::Value NodeService::formatNodesInfo(const Json::Value& systems,
                                           const Json::Value& details,
                                           const Json::Value& stats) {
  std::map<std::string, Json::Value> detailById;
  std::map<std::string, Json::Value> statsById;
  for (const auto& item : details) {
    detailById.emplace(item["id"].asString(), item);
  }
  for (const auto& item : stats) {
    const std::string id = item["system"].asString();
    if (statsById.find(id) == statsById.end()) {
      statsById.emplace(id, item.isMember("stats") ? item["stats"] : Json::Value(Json::objectValue));
    }
  }
  Json::Value data(Json::objectValue);
  for (const auto& system : systems) {
    const std::string id = system["id"].asString();
    const Json::Value info = system.isMember("info") && system["info"].isObject() ? system["info"] : Json::Value(Json::objectValue);
    const auto detailFound = detailById.find(id);
    const Json::Value detail = detailFound == detailById.end() ? Json::Value(Json::objectValue) : detailFound->second;
    const auto statsFound = statsById.find(id);
    const bool hasStats = statsFound != statsById.end();
    const Json::Value stat = hasStats ? statsFound->second : Json::Value(Json::objectValue);
    const bool hasMemory = hasStats && stat.isMember("mem") && stat["mem"].isArray() && stat["mem"].size() >= 2;
    Json::Value node(Json::objectValue);
    Json::Value systemNode(Json::objectValue);
    systemNode[device::KEY_NAME] = system.isMember("name") ? system["name"].asString() : "";
    systemNode[device::KEY_HOST] = system.isMember("host") ? system["host"].asString() : "";
    systemNode[device::KEY_STATUS] = system.isMember("status") ? system["status"].asString() : "";
    systemNode[device::KEY_HOSTNAME] = textOrNull(detail, "hostname");
    systemNode[device::KEY_OS_NAME] = textOrNull(detail, "os_name");
    systemNode[device::KEY_KERNEL] = textOrNull(detail, "kernel");
    systemNode[device::KEY_ARCH] = textOrNull(detail, "arch");
    systemNode[device::KEY_UPTIME] = formatUptime(info);
    Json::Value cpu(Json::objectValue);
    cpu[device::KEY_MODEL] = textOrNull(detail, "cpu");
    cpu[device::KEY_CORES] = intOrNull(detail, "cores");
    cpu[device::KEY_THREADS] = intOrNull(detail, "threads");
    cpu[device::KEY_UTILIZATION] = hasStats && stat.isMember("cpu") && stat["cpu"].isNumeric() ? Json::Value(stat["cpu"].asDouble()) : Json::Value(Json::nullValue);
    Json::Value memory(Json::objectValue);
    memory[device::KEY_TOTAL] = hasMemory ? byte2MB(stat["mem"][0]) : Json::Value(Json::nullValue);
    memory[device::KEY_USED] = hasMemory ? byte2MB(stat["mem"][1]) : Json::Value(Json::nullValue);
    node[device::KEY_SYSTEM] = systemNode;
    node[device::KEY_CPU] = cpu;
    node[device::KEY_MEMORY] = memory;
    node[device::KEY_GPU] = hasStats ? formatGpu(stat) : Json::Value(Json::objectValue);
    data[id] = node;
  }
  return data;
}
