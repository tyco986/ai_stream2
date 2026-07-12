import json
import os

from confluent_kafka import Producer


class DetMessager:
    def __init__(self, topic, bootstrap_servers=None):
        self.topic = topic
        self.bootstrap_servers = bootstrap_servers or os.environ.get(
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

    def __call__(self, results) -> None:
        for frame_result in results:
            for item in frame_result["objects"]:
                message = self.format_object(item)
                payload = json.dumps(message, ensure_ascii=False, separators=(",", ":"))
                self.producer.produce(self.topic, payload.encode("utf-8"))
        self.producer.poll(0)
