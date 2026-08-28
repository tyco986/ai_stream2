#include <iostream>
#include <string>

#include "pipeline_runner.hpp"

int main(int argc, char** argv) {
  int status = 0;
  if (argc != 2) {
    std::cerr << "Usage: pipeline_runner CONFIG_DIR\n";
    status = 1;
  } else {
    PipelineRunner runner(argv[1]);
    runner.run();
  }
  return status;
}
