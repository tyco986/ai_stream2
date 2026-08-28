#include <iostream>

#include "api_server.hpp"
#include "constants.hpp"
#include "pipeline_service.hpp"

int main() {
  AppConfig config;
  PipelineService service(config.schema_dir, config.config_save_dir,
                          config.pipeline_runner);
  ApiServer server(std::move(config), service);
  int status = 0;
  if (!server.listen()) {
    std::cerr << "listen failed\n";
    status = 1;
  }
  return status;
}
