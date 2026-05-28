import logging
import os
import signal
from multiprocessing import Process

from pyservicemaker import (
    DynamicSourceMessage, PipelineState, StateTransitionMessage, utils,
)

from pipeline.builder import PipelineBuilder
from daemons.command_consumer import CommandConsumer

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
    StateTransitionMessage.
    """

    def __init__(self, source_map, engine_monitor=None):
        self._source_map = source_map
        self._engine_monitor = engine_monitor

    def __call__(self, message):
        if isinstance(message, DynamicSourceMessage):
            self._handle_dynamic_source(message)
        elif isinstance(message, StateTransitionMessage):
            self._handle_state_transition(message)

    def _handle_dynamic_source(self, msg):
        if msg.source_added:
            self._source_map[msg.sensor_id] = msg.source_id
            logger.info(
                "Stream added: sensor_id=%s source_id=%d uri=%s",
                msg.sensor_id, msg.source_id, msg.uri,
            )
        else:
            self._source_map.pop(msg.sensor_id, None)
            logger.info("Stream removed: source_id=%d", msg.source_id)

    def _handle_state_transition(self, msg):
        if msg.new_state == PipelineState.PLAYING:
            if self._engine_monitor and not self._engine_monitor.started:
                self._engine_monitor.start()
                logger.info("Engine file monitor started")


class ShutdownActions:
    """Callable that bundles all cleanup actions for graceful shutdown."""

    def __init__(self, cmd_consumer):
        self._cmd = cmd_consumer

    def __call__(self):
        self._cmd.stop()


def run_pipeline():
    comp = PipelineBuilder().build()
    pipeline = comp.pipeline

    source_map = {}

    engine_file = comp.pgie_element.get("model-engine-file") or ""
    engine_monitor = utils.EngineFileMonitor(comp.pgie_element, engine_file) if engine_file else None

    msg_handler = MessageHandler(source_map, engine_monitor)

    kafka_broker = os.environ.get("KAFKA_BROKER", "kafka:9092")
    command_topic = os.environ.get("KAFKA_COMMAND_TOPIC", "deepstream-commands")
    event_topic = os.environ.get("KAFKA_EVENT_TOPIC", "deepstream-events")

    cmd_consumer = CommandConsumer(
        tiler_element=comp.tiler_element,
        osd_toggle=comp.osd_toggle,
        kafka_config={
            "bootstrap.servers": kafka_broker,
            "group.id": "deepstream-cmd-consumer",
            "auto.offset.reset": "latest",
        },
        command_topic=command_topic,
        event_topic=event_topic,
    )

    shutdown_actions = ShutdownActions(cmd_consumer)
    GracefulShutdown(pipeline, on_shutdown=shutdown_actions)

    logger.info("Preparing pipeline …")
    pipeline.prepare(msg_handler)
    logger.info("Pipeline prepare completed")

    logger.info("Activating pipeline …")
    pipeline.activate()
    logger.info("Pipeline activate completed")

    logger.info(
        "Pipeline running. REST API at http://0.0.0.0:%s",
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
