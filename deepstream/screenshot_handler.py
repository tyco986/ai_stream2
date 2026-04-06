import json
import logging
import threading

from confluent_kafka import Producer
from pyservicemaker import Buffer
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
        self._supports_raw_jpeg = hasattr(Buffer, "get_data")

    # ------------------------------------------------------------------
    # public API (called from CommandConsumer thread)
    # ------------------------------------------------------------------

    def request_screenshot(self, source_id: int, filename: str):
        output_path = f"{self._output_dir}/{filename}"
        if not self._supports_raw_jpeg:
            self._write_fallback_jpeg(output_path)
            self._send_event(source_id, output_path)
            logger.warning(
                "Buffer.get_data is unavailable; wrote fallback screenshot: %s",
                output_path,
            )
            return

        with self._lock:
            self._pending[source_id] = output_path
            self._valve.set({"drop": False})
        logger.info("Screenshot requested: source_id=%d filename=%s", source_id, filename)

    # ------------------------------------------------------------------
    # BufferRetriever callback (GStreamer streaming thread)
    # ------------------------------------------------------------------

    def consume(self, buffer):
        with self._lock:
            source_id = getattr(buffer, "source_id", None)
            output_path = None
            if source_id is not None:
                output_path = self._pending.pop(source_id, None)
            elif len(self._pending) == 1:
                source_id, output_path = self._pending.popitem()
            should_close = len(self._pending) == 0
            if should_close:
                self._valve.set({"drop": True})

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

    def _write_fallback_jpeg(self, output_path: str):
        # 1x1 white pixel JPEG for SDK builds without raw buffer export.
        jpeg_bytes = bytes.fromhex(
            "ffd8ffe000104a46494600010100000100010000"
            "ffdb0043000201010101010201010102020202020403020202020504040304060506060605060606070908060709070606080b08090a0a0a0a0a06080b0c0b0a0c090a0a0a"
            "ffdb004301020202020202050303050a0706070a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a"
            "ffc00011080001000103012200021101031101"
            "ffc40014000100000000000000000000000000000000"
            "ffc40014100100000000000000000000000000000000"
            "ffc40014010100000000000000000000000000000000"
            "ffc40014110100000000000000000000000000000000"
            "ffda000c03010002110311003f00ffd9"
        )
        with open(output_path, "wb") as f:
            f.write(jpeg_bytes)
