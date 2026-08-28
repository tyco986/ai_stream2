import os
from copy import deepcopy
from pathlib import Path

default_kafka_config = (
    "[message-broker]\n"
    '#producer-proto-cfg = "queue.buffering.max.messages=200000;message.send.max.retries=3"\n'
)

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
KAFKA_PROTO_LIB = "/opt/nvidia/deepstream/deepstream/lib/libnvds_kafka_proto.so"
KAFKA_CONN_STR = f"{PROJECT_NAME}_kafka;9092"


class KafkaGenerator:
    """Write kafka.txt for nvmsgbroker librdkafka settings."""

    def __init__(self) -> None:
        self.config = deepcopy(default_kafka_config)

    def write(self, save_path: str | Path) -> None:
        Path(save_path).write_text(self.config, encoding="utf-8")

if __name__ == "__main__":
    kafka_generator = KafkaGenerator()
    print(kafka_generator.config)