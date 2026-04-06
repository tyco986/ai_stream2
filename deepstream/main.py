import logging
import os
import signal
import threading
from multiprocessing import Process

from pyservicemaker import (
    DynamicSourceMessage, PipelineState, StateTransitionMessage, utils,
)

from pipeline_builder import PipelineBuilder
from preview_server import PreviewServer
from recording_manager import RollingRecordManager
from command_consumer import CommandConsumer
from disk_guard import DiskGuard
from gpu_monitor import GpuMemoryMonitor

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger("deepstream")


class GracefulShutdown:
    """Register SIGTERM/SIGINT to deactivate the pipeline and stop daemons."""

    def __init__(self, pipeline, on_shutdown=None):
        self._pipeline = pipeline
        self._on_shutdown = on_shutdown
        signal.signal(signal.SIGTERM, self._handle)
        signal.signal(signal.SIGINT, self._handle)

    def _handle(self, signum, frame):
        logger.info("Received signal %d, shutting down …", signum)
        if self._on_shutdown:
            self._on_shutdown()
        self._pipeline.deactivate()


class MessageHandler:
    """Pipeline on_message callback — handles DynamicSourceMessage and
    StateTransitionMessage.  Extracted as a class to comply with the
    "no nested function definitions" code-style rule.
    """

    def __init__(self, source_map, perf_monitor, engine_monitor, rolling_manager):
        self._source_map = source_map
        self._perf = perf_monitor
        self._engine_monitor = engine_monitor
        self._rolling = rolling_manager

    def __call__(self, message):
        if isinstance(message, DynamicSourceMessage):
            self._handle_dynamic_source(message)
        elif isinstance(message, StateTransitionMessage):
            self._handle_state_transition(message)

    def _handle_dynamic_source(self, msg):
        if msg.source_added:
            self._source_map[msg.sensor_id] = msg.source_id
            self._perf.add_stream(
                source_id=msg.source_id,
                uri=msg.uri,
                sensor_id=msg.sensor_id,
                sensor_name=msg.sensor_name,
            )
            logger.info(
                "Stream added: sensor_id=%s source_id=%d uri=%s",
                msg.sensor_id, msg.source_id, msg.uri,
            )
            self._rolling.start_rolling(msg.source_id)
        else:
            self._source_map.pop(msg.sensor_id, None)
            self._perf.remove_stream(msg.source_id)
            self._rolling.stop_rolling(msg.source_id)
            logger.info("Stream removed: source_id=%d", msg.source_id)

    def _handle_state_transition(self, msg):
        if msg.new_state == PipelineState.PLAYING:
            if self._engine_monitor and not self._engine_monitor.started:
                self._engine_monitor.start()
                logger.info("Engine file monitor started")


def run_pipeline():
    builder = PipelineBuilder()
    comp = builder.build()
    pipeline = comp.pipeline

    # ── shared state ────────────────────────────────────────────────
    source_map = {}   # sensor_id(str) → source_id(int)

    # ── recording manager ───────────────────────────────────────────
    rolling_dir = os.environ.get("DS_ROLLING_DIR", "/app/recordings/rolling")
    locked_dir = os.environ.get("DS_LOCKED_DIR", "/app/recordings/locked")
    segment_sec = int(os.environ.get("DS_RECORDING_SEGMENT_SEC", "300"))

    rolling_manager = RollingRecordManager(
        sr_controller=comp.sr_controller,
        rolling_dir=rolling_dir,
        locked_dir=locked_dir,
        segment_duration=segment_sec,
    )

    # ── wire sr-done signal for chain recording / file locking ──────
    # SmartRecord sr-done callback:  nvmultiurisrcbin emits the
    # "sr-done" GObject signal when a recording segment finishes.
    # The native Kafka notification is handled by SmartRecordConfig's
    # proto_lib automatically; this Python-side callback handles:
    #   1. moving event/manual recordings to locked/
    #   2. chaining the next rolling segment
    pipeline.attach("src", "smart_recording_signal", "sr", "sr-done")
    # TODO(integration): verify that the above built-in wiring triggers
    # on_sr_done.  If pyservicemaker doesn't expose a Python callback
    # for sr-done, use CommonFactory as fallback (plan Section 5B).

    # ── performance monitor ─────────────────────────────────────────
    max_batch = int(os.environ.get("DS_MAX_BATCH_SIZE", "16"))

    perf_monitor = utils.PerfMonitor(
        batch_size=max_batch,
        interval=5,
        source_type="nvmultiurisrcbin",
        show_name=True,
    )
    perf_monitor.apply(comp.tracker_element, "src")

    # ── engine file monitor ─────────────────────────────────────────
    engine_file = comp.pgie_element.get("model-engine-file") or ""
    engine_monitor = utils.EngineFileMonitor(comp.pgie_element, engine_file) if engine_file else None

    # ── RTSP preview server ─────────────────────────────────────────
    rtsp_port = os.environ.get("DS_RTSP_PORT", "8554")
    preview_server = PreviewServer(rtsp_port=rtsp_port, udpsrc_port=5400)
    preview_server.start()

    # ── message callback ────────────────────────────────────────────
    msg_handler = MessageHandler(source_map, perf_monitor, engine_monitor, rolling_manager)

    # ── daemon threads ──────────────────────────────────────────────
    kafka_broker = os.environ.get("KAFKA_BROKER", "kafka:9092")
    command_topic = os.environ.get("KAFKA_COMMAND_TOPIC", "deepstream-commands")

    cmd_consumer = CommandConsumer(
        rolling_manager=rolling_manager,
        sr_controller=comp.sr_controller,
        screenshot_retriever=comp.screenshot_retriever,
        tiler_element=comp.tiler_element,
        source_map=source_map,
        kafka_config={
            "bootstrap.servers": kafka_broker,
            "group.id": "deepstream-cmd-consumer",
            "auto.offset.reset": "latest",
        },
        command_topic=command_topic,
    )

    disk_guard = DiskGuard(
        rolling_dir=rolling_dir,
        locked_dir=locked_dir,
        max_usage_percent=int(os.environ.get("DS_DISK_MAX_USAGE_PCT", "85")),
        locked_max_age_days=int(os.environ.get("DS_LOCKED_MAX_AGE_DAYS", "30")),
        check_interval=int(os.environ.get("DS_DISK_CHECK_INTERVAL", "60")),
    )
    threading.Thread(target=disk_guard.run, daemon=True, name="disk-guard").start()

    gpu_monitor = GpuMemoryMonitor(
        interval=30,
        gpu_index=0,
    )
    threading.Thread(target=gpu_monitor.run, daemon=True, name="gpu-monitor").start()

    # ── graceful shutdown ───────────────────────────────────────────
    GracefulShutdown(pipeline, on_shutdown=cmd_consumer.stop)

    # ── start pipeline ──────────────────────────────────────────────
    logger.info("Preparing pipeline …")
    pipeline.prepare(msg_handler)

    logger.info("Activating pipeline …")
    pipeline.activate()

    logger.info(
        "Pipeline running. REST API at http://0.0.0.0:%s  RTSP at rtsp://0.0.0.0:%s/preview",
        os.environ.get("DS_REST_PORT", "9000"),
        rtsp_port,
    )
    pipeline.wait()
    logger.info("Pipeline stopped.")


def main():
    process = Process(target=run_pipeline)
    process.start()
    process.join()


if __name__ == "__main__":
    main()
