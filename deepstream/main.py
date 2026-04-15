import logging
import os
import signal
import subprocess
import threading
from multiprocessing import Process

from pyservicemaker import (
    DynamicSourceMessage, PipelineState, StateTransitionMessage, utils,
)

from pipeline.builder import PipelineBuilder
from recording.manager import RollingRecordManager
from daemons.command_consumer import CommandConsumer
from daemons.disk_guard import DiskGuard
from daemons.gpu_monitor import GpuMemoryMonitor
from utils.storage import StorageManager

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
            self._rolling.register_source(msg.source_id, msg.sensor_id, msg.uri)
            self._rolling.start_rolling(msg.source_id, msg.uri)
        else:
            self._source_map.pop(msg.sensor_id, None)
            self._perf.remove_stream(msg.source_id)
            self._rolling.stop_rolling(msg.source_id)
            self._rolling.unregister_source(msg.source_id)
            logger.info("Stream removed: source_id=%d", msg.source_id)

    def _handle_state_transition(self, msg):
        if msg.new_state == PipelineState.PLAYING:
            if self._engine_monitor and not self._engine_monitor.started:
                self._engine_monitor.start()
                logger.info("Engine file monitor started")


class ShutdownActions:
    """Callable that bundles all cleanup actions for graceful shutdown."""

    def __init__(self, cmd_consumer, recording_manager):
        self._cmd = cmd_consumer
        self._rec = recording_manager
        self._mtx = None

    def set_mediamtx(self, mtx_proc):
        self._mtx = mtx_proc

    def __call__(self):
        self._cmd.stop()
        self._rec.shutdown()
        if self._mtx:
            self._mtx.terminate()


def run_pipeline():
    # ── storage manager ──────────────────────────────────────────────
    storage_dir = os.environ.get("DS_STORAGE_DIR", "/app/storage")
    storage = StorageManager(base_dir=storage_dir)

    # ── build pipeline ───────────────────────────────────────────────
    builder = PipelineBuilder(storage)
    comp = builder.build()
    pipeline = comp.pipeline

    # ── shared state ─────────────────────────────────────────────────
    source_map = {}   # sensor_id(str) → source_id(int)

    # ── recording manager ────────────────────────────────────────────
    segment_sec = int(os.environ.get("DS_RECORDING_SEGMENT_SEC", "300"))

    rolling_manager = RollingRecordManager(
        storage=storage,
        segment_duration=segment_sec,
        source_element=comp.source_element,
    )

    # ── performance monitor ──────────────────────────────────────────
    max_batch = int(os.environ.get("DS_MAX_BATCH_SIZE", "16"))

    perf_monitor = utils.PerfMonitor(
        batch_size=max_batch,
        interval=5,
        source_type="nvmultiurisrcbin",
        show_name=True,
    )
    perf_monitor.apply(comp.tracker_element, "src")

    # ── engine file monitor ──────────────────────────────────────────
    engine_file = comp.pgie_element.get("model-engine-file") or ""
    engine_monitor = utils.EngineFileMonitor(comp.pgie_element, engine_file) if engine_file else None

    # ── MediaMTX RTSP/WebRTC server ──────────────────────────────────
    mediamtx_cfg = os.environ.get("DS_MEDIAMTX_CONFIG", "/app/config/mediamtx.yml")

    # ── message callback ─────────────────────────────────────────────
    msg_handler = MessageHandler(source_map, perf_monitor, engine_monitor, rolling_manager)

    # ── daemon threads ───────────────────────────────────────────────
    kafka_broker = os.environ.get("KAFKA_BROKER", "kafka:9092")
    command_topic = os.environ.get("KAFKA_COMMAND_TOPIC", "deepstream-commands")

    cmd_consumer = CommandConsumer(
        rolling_manager=rolling_manager,
        screenshot_retriever=comp.screenshot_retriever,
        tiler_element=comp.tiler_element,
        osd_toggle=comp.osd_toggle,
        source_map=source_map,
        kafka_config={
            "bootstrap.servers": kafka_broker,
            "group.id": "deepstream-cmd-consumer",
            "auto.offset.reset": "latest",
        },
        command_topic=command_topic,
    )

    max_storage_gb = os.environ.get("DS_DISK_MAX_STORAGE_GB", "")
    max_storage_bytes = int(float(max_storage_gb) * (1024 ** 3)) if max_storage_gb else 0

    disk_guard = DiskGuard(
        storage=storage,
        max_usage_percent=int(os.environ.get("DS_DISK_MAX_USAGE_PCT", "85")),
        max_storage_bytes=max_storage_bytes,
        check_interval=int(os.environ.get("DS_DISK_CHECK_INTERVAL", "60")),
    )
    threading.Thread(target=disk_guard.run, daemon=True, name="disk-guard").start()

    # ── graceful shutdown ────────────────────────────────────────────
    shutdown_actions = ShutdownActions(cmd_consumer, rolling_manager)
    GracefulShutdown(pipeline, on_shutdown=shutdown_actions)

    # ── start pipeline ───────────────────────────────────────────────
    logger.info("Preparing pipeline …")
    pipeline.prepare(msg_handler)
    logger.info("Pipeline prepare completed")

    logger.info("Activating pipeline …")
    pipeline.activate()
    logger.info("Pipeline activate completed")

    # ── Start GPU monitor after pipeline is active (avoids pynvml/CUDA conflict) ──
    gpu_monitor = GpuMemoryMonitor(interval=30, gpu_index=0)
    threading.Thread(target=gpu_monitor.run, daemon=True, name="gpu-monitor").start()

    # ── Start MediaMTX after pipeline is active ──────────────────────
    mediamtx_proc = subprocess.Popen(
        ["mediamtx", mediamtx_cfg],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    logger.info("MediaMTX started (pid=%d, config=%s)", mediamtx_proc.pid, mediamtx_cfg)
    shutdown_actions.set_mediamtx(mediamtx_proc)

    logger.info(
        "Pipeline running. REST API at http://0.0.0.0:%s  MediaMTX at rtsp://0.0.0.0:8554/preview",
        os.environ.get("DS_REST_PORT", "9000"),
    )
    pipeline.wait()
    logger.info("Pipeline stopped.")


def main():
    process = Process(target=run_pipeline)
    process.start()
    process.join()


if __name__ == "__main__":
    main()
