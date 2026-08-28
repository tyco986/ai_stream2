from pathlib import Path

import yaml
from PIL import Image

from ..subelement_generator.kafka import (
    KAFKA_CONN_STR,
    KAFKA_PROTO_LIB,
    PROJECT_NAME,
    KafkaGenerator,
)
from ..subelement_generator.nvdsanalytics import NvdsanalyticsGenerator
from ..subelement_generator.nvmsgconv import PAYLOAD_DEEPSTREAM_MINIMAL, NvmsgconvGenerator
from ..subelement_generator.pgie import PgieGenerator
from ..subelement_generator.pipeline import PipelineGenerator
from ..subelement_generator.utils.nvdsanalytics_parser import NvdsanalyticsParser
from ..subelement_generator.utils.pgie_parser import PgieParser

IMAGE_STREAM_NAME = "image"

IMAGE_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → pgie → nvdsanalytics → tee_msg
          ─┬→ nvosdbin → nvvideoconvert → nvdetlogger → nvjpegenc → filesink
          └→ queue_msg → nvmsgconv → nvmsgbroker
"""


class BaseImageGenerator(PipelineGenerator):
    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    MSGCONV_CONFIG_NAME = "nvmsgconv.yml"
    KAFKA_CONFIG_NAME = "kafka.txt"
    PARAMS_CONFIG_NAME = "params.yml"

    f"""Generate YOLO image pipeline YAML.

    Reads ``input`` image via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``.
    {IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        pipeline_name: str,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        logger: dict | None = None,
        drawer: dict | None = None,
        event_coder: dict | None = None,
    ) -> None:
        self.pipeline_name = pipeline_name
        self.input = Path(input).expanduser().resolve()
        self.output = Path(output).expanduser().resolve()
        self.analyzer = analyzer
        self.pgie = pgie
        self.logger = {"interval": 0}
        if logger is not None:
            self.logger.update(logger)
        self.logger["root"] = f"/root/logs/deepstream/{pipeline_name}"
        self.drawer = drawer
        self.event_coder = event_coder

        super().__init__()

        self.init_input()
        self.init_pgie()
        self.init_nvdsanalytics()
        self.init_nvmsgconv()
        self.init_kafka()
        self.init_params()
        self.init_pipeline()

    def init_input(self) -> None:
        assert self.input.is_file(), f"input image not found: {self.input}"
        with Image.open(self.input) as image:
            self.width, self.height = image.size
        self.runtime_batch_size = 1
        self.output.parent.mkdir(parents=True, exist_ok=True)

    def init_params(self) -> None:
        self.params_yml = {}
        self.params_yml["pipeline_name"] = self.pipeline_name
        self.params_yml["generator"] = self.GENERATOR
        self.params_yml["input"] = str(self.input)
        self.params_yml["output"] = str(self.output)
        self.params_yml["pgie"] = self.pgie
        self.params_yml["analyzer"] = self.analyzer
        self.params_yml["logger"] = self.logger
        self.params_yml["drawer"] = self.drawer
        self.params_yml["event_coder"] = self.event_coder

    def init_pipeline(self) -> None:
        self.add()
        self.link()
        self.pipeline_yml = self.pipeline

    def init_pgie(self) -> None:
        self.pgie = {
            "model_dir": self.pgie["model_dir"],
            "class_attrs": self.pgie["class_attrs"],
            "interval": int(self.pgie.get("interval", 1)),
        }
        self.pgie_config_parser = PgieParser(
            self.pgie["model_dir"],
            self.runtime_batch_size,
            self.pgie["class_attrs"],
            self.pgie["interval"],
        )
        self.pgie_generator = PgieGenerator(**self.pgie_config_parser.build())
        self.apply_pgie_config()

    def apply_pgie_config(self) -> None:
        raise NotImplementedError

    def init_nvdsanalytics(self) -> None:
        self.enable_nvdsanalytics = self.analyzer is not None
        if not self.enable_nvdsanalytics:
            self.nvdsanalytics_yml = NvdsanalyticsGenerator().config
            self.nvdsanalytics_yml["property"]["config-width"] = self.width
            self.nvdsanalytics_yml["property"]["config-height"] = self.height
        else:
            template = dict(self.analyzer["template"])
            line_crossing = template.get("line_crossing")
            if line_crossing is not None:
                assert int(line_crossing.get("enable", 0)) == 0, (
                    "line_crossing enable must be 0 for image pipelines"
                )
                del template["line_crossing"]
            direction_detection = template.get("direction_detection")
            if direction_detection is not None:
                assert int(direction_detection.get("enable", 0)) == 0, (
                    "direction_detection enable must be 0 for image pipelines"
                )
                del template["direction_detection"]
            parser = NvdsanalyticsParser(
                self.analyzer["streams"],
                template,
                self.analyzer.get("osd_mode", 0),
            )
            parser.validate([IMAGE_STREAM_NAME], self.pgie_config_parser.class_ids)
            config = parser.build([IMAGE_STREAM_NAME], self.width, self.height)
            self.nvdsanalytics_yml = NvdsanalyticsGenerator(config).config

    def kafka_streams(self) -> list[str]:
        return [IMAGE_STREAM_NAME]

    def init_nvmsgconv(self) -> None:
        self.nvmsgconv_generator = NvmsgconvGenerator(self.kafka_streams())
        self.nvmsgconv_yml = self.nvmsgconv_generator.config

    def init_kafka(self) -> None:
        self.kafka_generator = KafkaGenerator()
        self.kafka_yml = self.kafka_generator.config
        self.kafka_topic = f"{PROJECT_NAME}_{self.pipeline_name}"

    def apply_save_paths(self, config_save_dir: Path) -> None:
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "pgie":
                properties["config-file-path"] = str(
                    config_save_dir / self.PGIE_CONFIG_NAME
                )
            if name == "nvdsanalytics":
                properties["config-file"] = str(
                    config_save_dir / self.ANALYTICS_CONFIG_NAME
                )
            if name == "nvmsgconv":
                properties["config"] = str(config_save_dir / self.MSGCONV_CONFIG_NAME)
            if name == "nvmsgbroker":
                properties["config"] = str(config_save_dir / self.KAFKA_CONFIG_NAME)

    def write(self, config_save_dir: str | Path) -> None:
        config_save_dir = Path(config_save_dir)
        pipeline_save_path = config_save_dir / self.PIPELINE_CONFIG_NAME
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        nvdsanalytics_save_path = config_save_dir / self.ANALYTICS_CONFIG_NAME
        self.apply_save_paths(config_save_dir)
        with open(pgie_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.pgie_yml, handle, sort_keys=False, default_flow_style=False)
        with open(nvdsanalytics_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.nvdsanalytics_yml,
                handle,
                sort_keys=False,
                default_flow_style=False,
            )
        with open(pipeline_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.pipeline_yml, handle, sort_keys=False, default_flow_style=False
            )
        with open(config_save_dir / self.MSGCONV_CONFIG_NAME, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.nvmsgconv_yml, handle, sort_keys=False, default_flow_style=False
            )
        (config_save_dir / self.KAFKA_CONFIG_NAME).write_text(
            self.kafka_yml, encoding="utf-8"
        )
        with open(config_save_dir / self.PARAMS_CONFIG_NAME, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.params_yml, handle, sort_keys=False, default_flow_style=False
            )

    def osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
        }

    def nvdet_drawer_element(self) -> str:
        return "nvdetfadedrawer"

    def nvpose_drawer_element(self) -> str:
        return "nvposefadedrawer"

    def nvseg_drawer_element(self) -> str:
        return "nvsegfadedrawer"

    def nvdet_drawer_properties(self, drawer: dict) -> dict:
        return self._add_nvdetfadedrawer(
            interval=int(drawer.get("interval", 0)),
            fade_time=int(drawer.get("fade_time", 0)),
            show_label=bool(drawer.get("show_label", False)),
        )

    def nvpose_drawer_properties(self, drawer: dict) -> dict:
        return self._add_nvposefadedrawer(
            interval=int(drawer.get("interval", 0)),
            fade_time=int(drawer.get("fade_time", 0)),
            show_label=bool(drawer.get("show_label", False)),
            show_pose=bool(drawer.get("show_pose", True)),
            pose_threshold=float(drawer.get("pose_threshold", 0.0)),
            mode=drawer.get("mode", "coco17"),
        )

    def nvseg_drawer_properties(self, drawer: dict) -> dict:
        return self._add_nvsegfadedrawer(
            interval=int(drawer.get("interval", 0)),
            fade_time=int(drawer.get("fade_time", 0)),
            show_label=bool(drawer.get("show_label", False)),
            show_mask=bool(drawer.get("show_mask", True)),
        )

    def append_event_coder(self, name: str = "nvpresencecoder") -> None:
        if self.event_coder is not None:
            coder = self.event_coder
            self._append_node(
                "nvpresencecoder",
                name,
                self._add_nvpresencecoder(
                    class_ids=coder.get("class_ids", []),
                    event_names=coder.get("event_names", []),
                    length=int(coder.get("length", 10)),
                    threshold=float(coder.get("threshold", 0.5)),
                    mode=coder.get("mode", "fold"),
                ),
            )

    def after_analytics(self, tee_name: str = "tee_msg") -> str:
        next_name = tee_name
        if self.event_coder is not None:
            next_name = "nvpresencecoder"
        return next_name

    def link_event_coder(self, edges: dict, tee_name: str = "tee_msg") -> None:
        if self.event_coder is not None:
            edges["nvpresencecoder"] = tee_name

    def add(self) -> None:
        self._append_node(
            "nvurisrcbin",
            "nvurisrcbin",
            self._add_nvurisrcbin(
                self.file_uri(self.input),
                disable_audio=True,
                num_buffers=1,
            ),
        )
        self._append_node(
            "nvstreammux",
            "nvstreammux",
            self._add_nvstreammux(
                batch_size=1,
                width=self.width,
                height=self.height,
                live_source=False,
                enable_padding=False,
                batched_push_timeout=40000,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node(
            "nvinfer",
            "pgie",
            self._add_nvinfer(
                config_file_path=self.PGIE_CONFIG_NAME,
                batch_size=self.pgie_generator.batch_size,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node(
            "nvdsanalytics",
            "nvdsanalytics",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self.append_event_coder()
        self._append_node("tee", "tee_msg", self._add_tee())
        self._append_node("queue", "queue_msg", self._add_queue())
        self._append_node(
            "nvmsgconv",
            "nvmsgconv",
            self._add_nvmsgconv(
                self.MSGCONV_CONFIG_NAME,
                payload_type=PAYLOAD_DEEPSTREAM_MINIMAL,
            ),
        )
        self._append_node(
            "nvmsgbroker",
            "nvmsgbroker",
            self._add_nvmsgbroker(
                KAFKA_PROTO_LIB,
                KAFKA_CONN_STR,
                self.kafka_topic,
                self.KAFKA_CONFIG_NAME,
                sync=False,
                async_=False,
            ),
        )
        gpu_id = self.pgie_generator.gpu_id
        if self.drawer is not None:
            drawer = self.drawer
            self._append_node(
                self.nvdet_drawer_element(),
                "nvdetfadedrawer",
                self.nvdet_drawer_properties(drawer),
            )
        self._append_node(
            "nvosdbin",
            "nvosdbin",
            self._add_nvosdbin(**self.osd_kwargs(gpu_id)),
        )
        self._append_node(
            "nvvideoconvert",
            "nvvideoconvert",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "nvdetlogger",
            "nvdetlogger",
            self._add_nvdetlogger(
                root=f"/root/logs/deepstream/{self.pipeline_name}",
                interval=int(self.logger.get("interval", 0)),
            ),
        )
        self._append_node("nvjpegenc", "nvjpegenc", self._add_nvjpegenc())
        self._append_node(
            "filesink",
            "filesink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "nvurisrcbin": "nvstreammux",
            "nvstreammux": "pgie",
            "pgie": "nvdsanalytics",
        }
        edges["nvdsanalytics"] = self.after_analytics()
        self.link_event_coder(edges)
        vis_next = "nvosdbin"
        if self.drawer is not None:
            vis_next = "nvdetfadedrawer"
        edges["tee_msg"] = [vis_next, "queue_msg"]
        if self.drawer is not None:
            edges["nvdetfadedrawer"] = "nvosdbin"
        edges["queue_msg"] = "nvmsgconv"
        edges["nvmsgconv"] = "nvmsgbroker"
        edges["nvosdbin"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "nvdetlogger"
        edges["nvdetlogger"] = "nvjpegenc"
        edges["nvjpegenc"] = "filesink"
        self.pipeline["deepstream"]["edges"] = edges

    @staticmethod
    def file_uri(path: str | Path) -> str:
        return Path(path).expanduser().resolve().as_uri()
