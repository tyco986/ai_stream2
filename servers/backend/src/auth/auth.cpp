#include "auth/auth.hpp"

#include "constants/auth.hpp"
#include "response/response.hpp"

void AuthFilter::doFilter(const drogon::HttpRequestPtr& request,
                          drogon::FilterCallback&& failed,
                          drogon::FilterChainCallback&& chain) {
  const bool authorized = request->getHeader("authorization") == auth::BEARER_TOKEN;
  if (authorized) {
    chain();
  } else {
    failed(http::Response::error(drogon::k401Unauthorized, "unauthorized"));
  }
}
