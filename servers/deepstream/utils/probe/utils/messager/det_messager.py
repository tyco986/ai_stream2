import json
import os

from confluent_kafka import Producer


class DetMessager:
    def __init__(self, topic, bootstrap_servers=None, interval=0):
        self.topic = topic
        self.bootstrap_servers = bootstrap_servers
        self.interval = int(interval)
        self.counter = 0
        assert self.interval >= 0, "interval must be greater than or equal to 0"
        self.bootstrap_servers = self.bootstrap_servers or os.environ.get(
            "KAFKA_BROKER", "localhost:9092"
        )
        self.producer = Producer({"bootstrap.servers": self.bootstrap_servers})

    def format_object(self, item) -> list:
        message = [
            *item["box"],
            item["conf"],
            item["cls"],
            item["label"],
        ]
        return message

    def __call__(self, result: dict) -> None:
        if self.interval == 0 or self.counter % self.interval == 0:
            message = [self.format_object(item) for item in result["objects"]]
            payload = json.dumps(message, ensure_ascii=False, separators=(",", ":"))
            self.producer.produce(self.topic, payload.encode("utf-8"))
            self.producer.poll(0)
            self.counter = 0
        self.counter += 1
