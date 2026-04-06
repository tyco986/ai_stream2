import json
import logging
import threading

from confluent_kafka import Producer
from pyservicemaker import BufferRetriever

logger = logging.getLogger(__name__)


class ScreenshotRetriever(BufferRetriever):
    """On-demand screenshot via the snapshot tee branch.

    The valve in the snapshot branch is normally closed (``drop=True``).
    When ``request_screenshot`` is called the valve opens and
    ``consume()`` filters buffers by ``source_id``, writing only the
    target frame as JPEG.  Once all pending requests are served the
    valve is closed again.

    Thread safety:
        ``request_screenshot`` is called from CommandConsumer (daemon
        thread); ``consume`` is called from the GStreamer streaming
        thread.  A ``threading.Lock`` protects the shared ``_pending``
        dict and valve property.

    Mounting (done by PipelineBuilder):
        pipeline.attach("snap_sink",
                        Receiver("snap-receiver", screenshot_retriever),
                        tips="new-sample")
    """

    def __init__(self, output_dir="/app/screenshots",
                 valve_element=None,
                 kafka_broker="kafka:9092",
                 kafka_topic="deepstream-events"):
        super().__init__()
        self._output_dir = output_dir
        self._valve = valve_element
        self._pending = {}           # source_id(int) → output_path(str)
        self._lock = threading.Lock()

        self._kafka_broker = kafka_broker
        self._kafka_topic = kafka_topic
        self._producer = None

    # ------------------------------------------------------------------
    # public API (called from CommandConsumer thread)
    # ------------------------------------------------------------------

    def request_screenshot(self, source_id: int, filename: str):
        with self._lock:
            self._pending[source_id] = f"{self._output_dir}/{filename}"
            self._valve.set_property("drop", False)
        logger.info("Screenshot requested: source_id=%d filename=%s", source_id, filename)

    # ------------------------------------------------------------------
    # BufferRetriever callback (GStreamer streaming thread)
    # ------------------------------------------------------------------

    def consume(self, buffer):
        source_id = buffer.source_id

        with self._lock:
            output_path = self._pending.pop(source_id, None)
            should_close = len(self._pending) == 0
            if should_close:
                self._valve.set_property("drop", True)

        if output_path is None:
            return 1

        jpeg_bytes = buffer.get_data()
        with open(output_path, "wb") as f:
            f.write(jpeg_bytes)

        logger.info("Screenshot saved: %s (source_id=%d)", output_path, source_id)
        self._send_event(source_id, output_path)
        return 1

    # ------------------------------------------------------------------
    # Kafka event notification
    # ------------------------------------------------------------------

    def _send_event(self, source_id: int, filepath: str):
        producer = self._get_producer()
        if producer is None:
            return

        # Reverse-lookup sensor_id from source_id is not available here;
        # the consumer on the backend side maps by filepath / source_id.
        event = {
            "event": "screenshot_done",
            "source_id": source_id,
            "filepath": filepath,
        }
        producer.produce(
            self._kafka_topic,
            value=json.dumps(event).encode("utf-8"),
        )
        producer.poll(0)

    def _get_producer(self):
        if self._producer is not None:
            return self._producer
        self._producer = Producer({"bootstrap.servers": self._kafka_broker})
        return self._producer
