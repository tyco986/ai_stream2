#include "auth/auth.hpp"

#include "constants/auth_const.hpp"
#include "db/database.hpp"
#include "response/response.hpp"

#include <string>

void AuthFilter::doFilter(const drogon::HttpRequestPtr& request,
                          drogon::FilterCallback&& failed,
                          drogon::FilterChainCallback&& chain) {
  const std::string token = request->getHeader("authorization");
  const auto route = auth::CRUD.find({request->method(), request->matchedPathPattern()});
  const bool open = route == auth::CRUD.end();
  bool authorized = false;
  std::string crud;
  if (!token.empty()) {
    const auto access = Database::client()->execSqlSync(
        "SELECT user_group.stream_crud, user_group.model_crud, user_group.preview_crud, "
        "user_group.pipeline_crud, user_group.recording_crud, user_group.server_crud, "
        "user_group.user_crud, user_group.event_crud FROM \"user\" "
        "LEFT JOIN user_group ON user_group.id = \"user\".group_id "
        "WHERE \"user\".token = $1 AND \"user\".active",
        token);
    authorized = !access.empty();
    const bool readable = authorized && !open && !access[0][route->second.column].isNull();
    crud = readable ? access[0][route->second.column].as<std::string>() : "";
  }
  const bool bit = !open && crud.size() > static_cast<size_t>(route->second.index) &&
                    crud[route->second.index] == '1';
  const bool permitted = authorized && (open || bit);
  if (permitted) {
    chain();
  } else if (authorized) {
    failed(http::Response::error(drogon::k403Forbidden, auth::ERR_FORBIDDEN));
  } else {
    failed(http::Response::error(drogon::k401Unauthorized, auth::ERR_UNAUTHORIZED));
  }
}
