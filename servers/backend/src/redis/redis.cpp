#include "redis/redis.hpp"

#include "constants/redis_const.hpp"
#include "db/database.hpp"
#include "logger/redis_logger.hpp"
#include "services/event_hub.hpp"

#include <drogon/drogon.h>
#include <hiredis/hiredis.h>
#include <json/json.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

Redis::Redis() : service(Database::client()) {}

Redis& Redis::shared() {
  static Redis redis;
  return redis;
}

void Redis::watch(const std::string& name) {
  const std::string stream = std::string(redis::TOPIC_PREFIX) + name;
  std::lock_guard<std::mutex> guard(mutex);
  names.insert(stream);
}

std::vector<std::string> Redis::streams() {
  std::lock_guard<std::mutex> guard(mutex);
  return {names.begin(), names.end()};
}

void Redis::close() {
  if (context) {
    redisFree(context);
    context = nullptr;
  }
}

void Redis::connect() {
  const bool open = context && !context->err;
  if (!open) {
    close();
    context = redisConnect(redis::HOST, redis::PORT);
    const bool failed = !context || context->err;
    if (failed) {
      if (context) {
        spdlog::error("{}", context->errstr);
      }
      close();
    }
    if (!failed) {
      std::lock_guard<std::mutex> guard(mutex);
      grouped.clear();
    }
  }
}

redisReply* Redis::command(const std::vector<std::string>& args) {
  std::vector<const char*> argv;
  std::vector<size_t> lengths;
  for (const std::string& arg : args) {
    argv.push_back(arg.c_str());
    lengths.push_back(arg.size());
  }
  redisReply* reply = context
      ? static_cast<redisReply*>(redisCommandArgv(context, static_cast<int>(argv.size()), argv.data(), lengths.data()))
      : nullptr;
  return reply;
}

void Redis::ensureGroups(const std::vector<std::string>& keys) {
  std::vector<std::string> missing;
  {
    std::lock_guard<std::mutex> guard(mutex);
    for (const std::string& key : keys) {
      if (!grouped.contains(key)) {
        missing.push_back(key);
      }
    }
  }
  for (const std::string& key : missing) {
    redisReply* reply = command({"XGROUP", "CREATE", key, redis::GROUP, redis::STREAM_START, "MKSTREAM"});
    const bool busy = reply && reply->type == REDIS_REPLY_ERROR && reply->str && std::strstr(reply->str, redis::BUSYGROUP);
    const bool created = reply && reply->type != REDIS_REPLY_ERROR;
    if (created || busy) {
      std::lock_guard<std::mutex> guard(mutex);
      grouped.insert(key);
    }
    if (reply && reply->type == REDIS_REPLY_ERROR && !busy) {
      spdlog::error("{}", reply->str);
    }
    if (reply) {
      freeReplyObject(reply);
    }
    if (!reply) {
      close();
    }
  }
}

void Redis::collect(redisReply& reply) {
  if (reply.type == REDIS_REPLY_ARRAY) {
    for (size_t index = 0; index < reply.elements; ++index) {
      redisReply* stream = reply.element[index];
      const bool shaped = stream && stream->type == REDIS_REPLY_ARRAY && stream->elements == 2 && stream->element[0]->str;
      if (shaped) {
        const std::string name = stream->element[0]->str;
        redisReply* entries = stream->element[1];
        for (size_t entryIndex = 0; entries && entryIndex < entries->elements; ++entryIndex) {
          redisReply* entry = entries->element[entryIndex];
          const bool entryShaped = entry && entry->type == REDIS_REPLY_ARRAY && entry->elements == 2 && entry->element[0]->str;
          if (entryShaped) {
            const std::string id = entry->element[0]->str;
            redisReply* fields = entry->element[1];
            std::string payload;
            for (size_t field = 0; fields && field + 1 < fields->elements; field += 2) {
              const bool payloadField = fields->element[field]->str && std::strcmp(fields->element[field]->str, redis::PAYLOAD_KEY) == 0;
              if (payloadField && fields->element[field + 1]->str) {
                payload.assign(fields->element[field + 1]->str, fields->element[field + 1]->len);
              }
            }
            handle(name, id, payload);
          }
        }
      }
    }
  }
}

bool Redis::read(const std::vector<std::string>& keys, const char* id, bool block) {
  std::vector<std::string> args = {"XREADGROUP", "GROUP", redis::GROUP, redis::CONSUMER, "COUNT", std::to_string(redis::READ_COUNT)};
  if (block) {
    args.push_back("BLOCK");
    args.push_back(std::to_string(redis::BLOCK_MS));
  }
  args.push_back("STREAMS");
  args.insert(args.end(), keys.begin(), keys.end());
  args.insert(args.end(), keys.size(), id);
  redisReply* reply = command(args);
  const bool alive = reply != nullptr;
  if (reply) {
    if (reply->type == REDIS_REPLY_ERROR && reply->str) {
      spdlog::error("{}", reply->str);
    }
    collect(*reply);
    freeReplyObject(reply);
  }
  if (!alive) {
    close();
  }
  return alive;
}

void Redis::handle(const std::string& stream, const std::string& id, const std::string& payload) {
  Json::Value body;
  Json::CharReaderBuilder builder;
  std::string parseError;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  const bool parsed = reader->parse(payload.data(), payload.data() + payload.size(), &body, &parseError);
  const Json::Value logged = parsed ? body : Json::Value(payload);
  RedisLogger::write(stream, logged);
  if (parsed && body.isObject() && !stream.empty()) {
    dispatch(body, stream);
  }
  acknowledge(stream, id);
}

void Redis::dispatch(const Json::Value& message, const std::string& stream) {
  try {
    const Json::Value row = service.createFromRedis(message, stream);
    EventHub::shared().publish(row);
  } catch (const std::exception& error) {
    spdlog::error("{}", error.what());
  }
}

void Redis::acknowledge(const std::string& stream, const std::string& id) {
  redisReply* reply = command({"XACK", stream, redis::GROUP, id});
  if (reply && reply->type == REDIS_REPLY_ERROR && reply->str) {
    spdlog::error("{}", reply->str);
  }
  if (reply) {
    freeReplyObject(reply);
  }
  if (!reply) {
    close();
  }
}

void Redis::run() {
  while (true) {
    connect();
    const std::vector<std::string> keys = streams();
    const bool ready = context && !keys.empty();
    if (ready) {
      ensureGroups(keys);
    }
    const bool readable = ready && context;
    if (readable) {
      const bool pending = read(keys, redis::STREAM_START, false);
      if (pending) {
        read(keys, redis::STREAM_NEW, true);
      }
    }
    if (!readable) {
      std::this_thread::sleep_for(std::chrono::milliseconds(redis::BLOCK_MS));
    }
  }
}

class RedisRunner {
 public:
  RedisRunner() {
    drogon::app().registerBeginningAdvice([] {
      std::thread([] {
        Redis::shared().run();
      }).detach();
    });
  }
};

static RedisRunner redisRunner;
