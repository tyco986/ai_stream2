#pragma once

#include "constants/event_const.hpp"
#include "constants/log_const.hpp"
#include "constants/model_const.hpp"
#include "constants/pipeline_analyzer_const.hpp"
#include "constants/pipeline_const.hpp"
#include "constants/pipeline_gie_const.hpp"
#include "constants/preview_const.hpp"
#include "constants/server_const.hpp"
#include "constants/stream_const.hpp"
#include "constants/stream_group_const.hpp"
#include "constants/user_const.hpp"

#include <drogon/HttpTypes.h>

#include <map>
#include <string>
#include <utility>

namespace auth {

inline const char* ERR_UNAUTHORIZED = "unauthorized";
inline const char* ERR_FORBIDDEN = "forbidden";

inline const int CRUD_CREATE = 0;
inline const int CRUD_READ = 1;
inline const int CRUD_UPDATE = 2;
inline const int CRUD_DELETE = 3;

struct CrudGate {
  const char* column;
  int index;
};

inline const std::map<std::pair<drogon::HttpMethod, std::string>, CrudGate> CRUD = {
    // stream
    {{drogon::Get, stream::PATH_LIST}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Post, stream::PATH_LIST}, {user::KEY_STREAM_CRUD, CRUD_CREATE}},
    {{drogon::Post, stream::PATH_IMPORT}, {user::KEY_STREAM_CRUD, CRUD_CREATE}},
    {{drogon::Get, stream::PATH_EXPORT}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Get, stream::PATH_ITEM}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Patch, stream::PATH_ITEM}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, stream::PATH_ITEM}, {user::KEY_STREAM_CRUD, CRUD_DELETE}},
    {{drogon::Post, stream::PATH_PROBE}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    {{drogon::Post, stream::PATH_BATCH_REMOVE}, {user::KEY_STREAM_CRUD, CRUD_DELETE}},
    {{drogon::Post, stream::PATH_BATCH_ENABLE}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    {{drogon::Post, stream::PATH_BATCH_DISABLE}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    {{drogon::Post, stream::PATH_BATCH_RECORD}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    {{drogon::Post, stream::PATH_BATCH_UNRECORD}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    {{drogon::Post, stream::PATH_BATCH_PROBE}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    // stream group
    {{drogon::Post, stream_group::PATH_IMPORT}, {user::KEY_STREAM_CRUD, CRUD_CREATE}},
    {{drogon::Get, stream_group::PATH_EXPORT}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Get, stream_group::PATH_TREE}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Get, stream_group::PATH_MAP}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Post, stream_group::PATH_GROUP_CREATE}, {user::KEY_STREAM_CRUD, CRUD_CREATE}},
    {{drogon::Patch, stream_group::PATH_GROUP}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, stream_group::PATH_GROUP}, {user::KEY_STREAM_CRUD, CRUD_DELETE}},
    {{drogon::Get, stream_group::PATH_GROUP_MEMBERS}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Get, stream_group::PATH_GROUP_CANDIDATES}, {user::KEY_STREAM_CRUD, CRUD_READ}},
    {{drogon::Put, stream_group::PATH_GROUP_MEMBERS}, {user::KEY_STREAM_CRUD, CRUD_UPDATE}},
    // model
    {{drogon::Get, model::PATH_LIST}, {user::KEY_MODEL_CRUD, CRUD_READ}},
    {{drogon::Post, model::PATH_LIST}, {user::KEY_MODEL_CRUD, CRUD_CREATE}},
    {{drogon::Post, model::PATH_IMPORT}, {user::KEY_MODEL_CRUD, CRUD_CREATE}},
    {{drogon::Get, model::PATH_ITEM}, {user::KEY_MODEL_CRUD, CRUD_READ}},
    {{drogon::Patch, model::PATH_ITEM}, {user::KEY_MODEL_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, model::PATH_ITEM}, {user::KEY_MODEL_CRUD, CRUD_DELETE}},
    {{drogon::Post, model::PATH_UPLOAD}, {user::KEY_MODEL_CRUD, CRUD_UPDATE}},
    {{drogon::Post, model::PATH_BUILD}, {user::KEY_MODEL_CRUD, CRUD_UPDATE}},
    {{drogon::Get, model::PATH_STATUS}, {user::KEY_MODEL_CRUD, CRUD_READ}},
    {{drogon::Get, model::PATH_MAP}, {user::KEY_MODEL_CRUD, CRUD_READ}},
    {{drogon::Post, model::PATH_BATCH_BUILD}, {user::KEY_MODEL_CRUD, CRUD_UPDATE}},
    {{drogon::Post, model::PATH_BATCH_STATUS}, {user::KEY_MODEL_CRUD, CRUD_READ}},
    {{drogon::Post, model::PATH_BATCH_REMOVE}, {user::KEY_MODEL_CRUD, CRUD_DELETE}},
    // preview
    {{drogon::Get, preview::PATH_MAP}, {user::KEY_PREVIEW_CRUD, CRUD_READ}},
    {{drogon::Get, preview::PATH_TREE}, {user::KEY_PREVIEW_CRUD, CRUD_READ}},
    {{drogon::Get, preview::PATH_ACTIVE}, {user::KEY_PREVIEW_CRUD, CRUD_READ}},
    {{drogon::Get, preview::PATH_ITEM}, {user::KEY_PREVIEW_CRUD, CRUD_READ}},
    {{drogon::Post, preview::PATH_LIST}, {user::KEY_PREVIEW_CRUD, CRUD_CREATE}},
    {{drogon::Patch, preview::PATH_ITEM}, {user::KEY_PREVIEW_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, preview::PATH_ITEM}, {user::KEY_PREVIEW_CRUD, CRUD_DELETE}},
    // pipeline
    {{drogon::Get, pipeline::PATH_LIST}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Get, pipeline::PATH_MAP}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Get, pipeline::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Post, pipeline::PATH_LIST}, {user::KEY_PIPELINE_CRUD, CRUD_CREATE}},
    {{drogon::Patch, pipeline::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, pipeline::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_DELETE}},
    {{drogon::Post, pipeline::PATH_START}, {user::KEY_PIPELINE_CRUD, CRUD_UPDATE}},
    {{drogon::Post, pipeline::PATH_PROBE}, {user::KEY_PIPELINE_CRUD, CRUD_UPDATE}},
    {{drogon::Post, pipeline::PATH_STOP}, {user::KEY_PIPELINE_CRUD, CRUD_UPDATE}},
    {{drogon::Post, pipeline::PATH_BATCH_STOP}, {user::KEY_PIPELINE_CRUD, CRUD_UPDATE}},
    {{drogon::Post, pipeline::PATH_BATCH_REMOVE}, {user::KEY_PIPELINE_CRUD, CRUD_DELETE}},
    // pipeline gie
    {{drogon::Get, pipeline_gie::PATH_LIST}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Get, pipeline_gie::PATH_MODELS}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Get, pipeline_gie::PATH_MAP}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Get, pipeline_gie::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Post, pipeline_gie::PATH_LIST}, {user::KEY_PIPELINE_CRUD, CRUD_CREATE}},
    {{drogon::Put, pipeline_gie::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, pipeline_gie::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_DELETE}},
    {{drogon::Post, pipeline_gie::PATH_BATCH_REMOVE}, {user::KEY_PIPELINE_CRUD, CRUD_DELETE}},
    // pipeline analyzer
    {{drogon::Get, pipeline_analyzer::PATH_LIST}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Get, pipeline_analyzer::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_READ}},
    {{drogon::Post, pipeline_analyzer::PATH_LIST}, {user::KEY_PIPELINE_CRUD, CRUD_CREATE}},
    {{drogon::Put, pipeline_analyzer::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, pipeline_analyzer::PATH_ITEM}, {user::KEY_PIPELINE_CRUD, CRUD_DELETE}},
    {{drogon::Post, pipeline_analyzer::PATH_BATCH_REMOVE}, {user::KEY_PIPELINE_CRUD, CRUD_DELETE}},
    // server
    {{drogon::Get, server::PATH_ITEM}, {user::KEY_SERVER_CRUD, CRUD_READ}},
    {{drogon::Post, server::PATH_PROBE}, {user::KEY_SERVER_CRUD, CRUD_UPDATE}},
    // user
    {{drogon::Get, user::PATH_USER_LIST}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Get, user::PATH_USER_MAP}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Post, user::PATH_USER_LIST}, {user::KEY_USER_CRUD, CRUD_CREATE}},
    {{drogon::Get, user::PATH_USER_ITEM}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Patch, user::PATH_USER_ITEM}, {user::KEY_USER_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, user::PATH_USER_ITEM}, {user::KEY_USER_CRUD, CRUD_DELETE}},
    {{drogon::Post, user::PATH_BATCH_REMOVE}, {user::KEY_USER_CRUD, CRUD_DELETE}},
    // user group
    {{drogon::Get, user::PATH_LIST}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Get, user::PATH_MAP}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Post, user::PATH_LIST}, {user::KEY_USER_CRUD, CRUD_CREATE}},
    {{drogon::Get, user::PATH_ITEM}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Patch, user::PATH_ITEM}, {user::KEY_USER_CRUD, CRUD_UPDATE}},
    {{drogon::Delete, user::PATH_ITEM}, {user::KEY_USER_CRUD, CRUD_DELETE}},
    {{drogon::Get, user::PATH_MEMBERS}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Get, user::PATH_CANDIDATES}, {user::KEY_USER_CRUD, CRUD_READ}},
    {{drogon::Put, user::PATH_MEMBERS}, {user::KEY_USER_CRUD, CRUD_UPDATE}},
    // event
    {{drogon::Get, event::PATH_LIST}, {user::KEY_EVENT_CRUD, CRUD_READ}},
    {{drogon::Post, event::PATH_STATUS}, {user::KEY_EVENT_CRUD, CRUD_UPDATE}},
    // log
    {{drogon::Get, log::PATH_LIST}, {user::KEY_EVENT_CRUD, CRUD_READ}},
    {{drogon::Post, log::PATH_ENABLE}, {user::KEY_EVENT_CRUD, CRUD_UPDATE}},
};

}
