from typing import Any

import json

from confluent_kafka import Producer


class DetMessager:
    def __init__(self, topic, host, port, interval=0):
        self.topic = topic
        self.host = host
        self.port = int(port)
        self.interval = int(interval)
        self.runtime_interval = self.interval + 1 if self.interval > 0 else 0
        self.counter = 0
        assert self.interval >= 0, "interval must be greater than or equal to 0"
        self.producer = Producer({"bootstrap.servers": f"{self.host}:{self.port}"})

    def format_message(self, result: dict) -> list:
        message = [list[Any](item["object"]) for item in result["objects"]]
        return message

    def __call__(self, result: dict) -> None:
        if self.interval == 0 or self.counter % self.runtime_interval == 0:
            message = self.format_message(result)
            payload = json.dumps(message, ensure_ascii=False, separators=(",", ":"))
            self.producer.produce(self.topic, payload.encode("utf-8"))
            self.producer.poll(0)
            self.counter = 0
        self.counter += 1
