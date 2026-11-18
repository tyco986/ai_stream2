#pragma once

#include <json/json.h>

#include <chrono>

struct StepSpan {
  StepSpan(Json::Value& steps, const char* key)
      : steps(steps), key(key), began(std::chrono::steady_clock::now()) {}
  StepSpan(const StepSpan&) = delete;
  StepSpan& operator=(const StepSpan&) = delete;
  ~StepSpan() {
    steps[key] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();
  }

  Json::Value& steps;
  const char* key;
  std::chrono::steady_clock::time_point began;
};
