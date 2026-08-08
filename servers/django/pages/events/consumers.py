import logging
import time

from django.conf import settings

from pages.events.services import EventIngestService

logger = logging.getLogger(__name__)


class EventKafkaConsumer:
    def __init__(self, bootstrap_servers=None, topic=None, group_id=None):
        self.bootstrap_servers = (
            bootstrap_servers
            if bootstrap_servers is not None
            else settings.KAFKA_BOOTSTRAP_SERVERS
        )
        self.topic = topic if topic is not None else settings.EVENTS_KAFKA_TOPIC
        self.group_id = (
            group_id if group_id is not None else settings.EVENTS_KAFKA_GROUP_ID
        )
        self.ingest = EventIngestService()

    def run_forever(self, poll_timeout=1.0):
        from confluent_kafka import Consumer

        consumer = Consumer(
            {
                "bootstrap.servers": self.bootstrap_servers,
                "group.id": self.group_id,
                "auto.offset.reset": "earliest",
                "enable.auto.commit": True,
            }
        )
        consumer.subscribe([self.topic])
        logger.info(
            "events consumer started topic=%s bootstrap=%s",
            self.topic,
            self.bootstrap_servers,
        )
        try:
            while True:
                message = consumer.poll(poll_timeout)
                if message is None:
                    continue
                if message.error():
                    logger.warning("events consumer kafka error: %s", message.error())
                    continue
                self.ingest.try_ingest_message(message.value())
        finally:
            consumer.close()

    def run_with_backoff(self):
        while True:
            try:
                self.run_forever()
            except Exception as exc:
                logger.exception("events consumer crashed: %s", exc)
                time.sleep(5)
