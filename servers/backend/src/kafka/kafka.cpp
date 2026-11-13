#include "kafka/kafka.hpp"

#include "constants/kafka_const.hpp"
#include "db/database.hpp"
#include "services/event_hub.hpp"

#include <drogon/drogon.h>
#include <librdkafka/rdkafkacpp.h>
#include <spdlog/spdlog.h>

#include <exception>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

class KafkaRebalance : public RdKafka::RebalanceCb {
 public:
  void rebalance_cb(RdKafka::KafkaConsumer* consumer,
                    RdKafka::ErrorCode err,
                    std::vector<RdKafka::TopicPartition*>& partitions) override {
    if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
      consumer->assign(partitions);
    } else {
      consumer->unassign();
    }
  }
};

}

Kafka::Kafka() : service(Database::client()) {}

void Kafka::dispatch(const Json::Value& message, const std::string& topic) {
  try {
    const Json::Value row = service.createFromKafka(message, topic);
    EventHub::shared().publish(row);
  } catch (const std::exception& error) {
    spdlog::error("{}", error.what());
  }
}

void Kafka::run() {
  std::string err;
  KafkaRebalance rebalance;
  auto conf = std::unique_ptr<RdKafka::Conf>(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  conf->set("bootstrap.servers", kafka::BROKERS, err);
  conf->set("group.id", kafka::GROUP, err);
  conf->set("enable.auto.commit", "true", err);
  conf->set("auto.offset.reset", "latest", err);
  conf->set("rebalance_cb", &rebalance, err);
  auto consumer = std::unique_ptr<RdKafka::KafkaConsumer>(RdKafka::KafkaConsumer::create(conf.get(), err));
  if (consumer) {
    consumer->subscribe({kafka::TOPIC_PATTERN});
  }
  while (true) {
    if (!consumer) {
      std::this_thread::sleep_for(std::chrono::milliseconds(kafka::POLL_MS));
      consumer.reset(RdKafka::KafkaConsumer::create(conf.get(), err));
      if (consumer) {
        consumer->subscribe({kafka::TOPIC_PATTERN});
      }
    }
    auto message = consumer ? std::unique_ptr<RdKafka::Message>(consumer->consume(kafka::POLL_MS)) : nullptr;
    if (message && message->err() == RdKafka::ERR_NO_ERROR && message->payload() != nullptr) {
      Json::Value body;
      Json::CharReaderBuilder builder;
      std::string parseError;
      const std::string payload(static_cast<const char*>(message->payload()), message->len());
      std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
      const bool parsed = reader->parse(payload.data(), payload.data() + payload.size(), &body, &parseError);
      const std::string topicName = message->topic_name();
      if (parsed && body.isObject() && !topicName.empty()) {
        dispatch(body, topicName);
      }
    }
  }
}

class KafkaRunner {
 public:
  KafkaRunner() {
    drogon::app().registerBeginningAdvice([] {
      std::thread([] {
        Kafka kafka;
        kafka.run();
      }).detach();
    });
  }
};

static KafkaRunner kafkaRunner;
