from pathlib import Path
from copy import deepcopy
default_kafka_config = (
    "[message-broker]\n"
    '#producer-proto-cfg = "queue.buffering.max.messages=200000;message.send.max.retries=3"\n'
)


class KafkaConfigGenerator:
    """Write kafka.txt for nvmsgbroker librdkafka settings."""

    def __init__(self) -> None:
        self.config = deepcopy(default_kafka_config)

    def write(self, save_path: str | Path) -> None:
        Path(save_path).write_text(self.config, encoding="utf-8")

if __name__ == "__main__":
    kafka_generator = KafkaConfigGenerator()
    print(kafka_generator.config)