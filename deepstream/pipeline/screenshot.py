import json
import logging
import threading

import cupy
from confluent_kafka import Producer
from PIL import Image
from pyservicemaker import BufferRetriever

logger = logging.getLogger(__name__)

JPEG_QUALITY = 95


class ScreenshotRetriever(BufferRetriever):
    """On-demand screenshot via the snapshot tee branch.

    Pipeline layout (built by PipelineBuilder):
        tee → queue → valve(drop=True) → nvvideoconvert → capsfilter(RGB) → appsink

    The valve is normally closed.  When ``request_screenshot`` is called the
    valve opens and ``consume()`` receives raw RGB frames.  The target frame
    is converted to JPEG via Pillow and written to disk.  Once all pending
    requests are served the valve is closed again.

    GPU→CPU path:  buffer.extract(0).clone()  →  cupy.from_dlpack  →  cupy.asnumpy
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
        output_path = f"{self._output_dir}/{filename}"
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

        tensor = buffer.extract(0).clone()
        np_arr = cupy.asnumpy(cupy.from_dlpack(tensor))
        Image.fromarray(np_arr).save(output_path, "JPEG", quality=JPEG_QUALITY)

        logger.info("Screenshot saved: %s (source_id=%s)", output_path, source_id)
        self._send_event(source_id, output_path)
        return 1

    # ------------------------------------------------------------------
    # Kafka event notification
    # ------------------------------------------------------------------

    def _send_event(self, source_id, filepath: str):
        producer = self._get_producer()
        if producer is None:
            return
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
