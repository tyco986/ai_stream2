#pragma once

#include <drogon/HttpFilter.h>

class AuthFilter : public drogon::HttpFilter<AuthFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr& request,
                drogon::FilterCallback&& failed,
                drogon::FilterChainCallback&& chain) override;
};
