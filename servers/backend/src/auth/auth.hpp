#pragma once

#include <drogon/HttpFilter.h>

#include <string>

class AuthFilter : public drogon::HttpFilter<AuthFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr& request,
                drogon::FilterCallback&& failed,
                drogon::FilterChainCallback&& chain) override;
  static std::string token(const drogon::HttpRequestPtr& request);
};
