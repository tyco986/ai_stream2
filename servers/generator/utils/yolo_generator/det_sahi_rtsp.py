import copy
import json
import shutil
from pathlib import Path

import yaml

from .base import DeepstreamGenerator, TRACKER_LL_LIB
from ..subelement_generator import NvsahipreprocessGenerator
from .det_rtsp import (
    DetRTSPGenerator,
    DetVisRTSPGenerator,
    RTSP_TOPOLOGY_DOC,
    VIS_RTSP_TOPOLOGY_DOC,
)
from .utils import YoloDetSahi, get_sahi_box, get_sahi_preview

SAHI_RTSP_TOPOLOGY_DOC = """
    Batch inference chain::

        src{N} → mux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → tracker → analyzer → demux

    ``mux`` batch size is the stream count; ``pgie`` batch size is the SAHI tile count.
    Per-stream capture chain matches DetRTSPGenerator.
"""


class DetSahiRTSPGenerator(DetRTSPGenerator):
    GENERATOR = "DetSahiRTSPGenerator"
    SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini"

    f"""Generate YOLO SAHI detection RTSP pipeline for event alert + probe-side Kafka.

    Set ``analyzer=None`` to keep nvdsanalytics inserted with master switch off.
    Set ``tracker=None`` to skip nvtracker. Pass ``tracker={{"class_id": ...}}`` to insert nvtracker.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker`` or RTSP preview sink;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {RTSP_TOPOLOGY_DOC}
    {SAHI_RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        analyzer: dict | None,
        pgie: dict,
        sahi: dict,
        tracker: dict | None = None,
        interval: int = 0,
    ) -> None:
        self.sahi = sahi
        self.streams = streams
        self.analyzer = analyzer
        self.tracker = tracker
        self.interval = interval
        self.pgie = pgie
        DeepstreamGenerator.__init__(self)
        self.init_streams()
        self.init_pgie()
        self.init_sahi()
        self.init_nvdsanalytics()
        self.init_nvtracker()
        self.init_params()
        self.init_pipeline()

    def init_streams(self) -> None:
        super().init_streams()
        self.mux_batch_size = self.runtime_batch_size
        sahi = self.sahi["nvsahipreprocess"]
        slice_info = get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=True,
        )
        self.runtime_batch_size = int(slice_info["num"])

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloDetSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def init_sahi(self) -> None:
        meta = self.pgie_config_parser.meta
        input_shape = meta["input_tensor_shape"]
        network_input_shape = ";".join(
            str(value) for value in [self.runtime_batch_size, *input_shape[1:]]
        )
        self.nvsahipreprocess_generator = NvsahipreprocessGenerator(
            network_input_shape=network_input_shape,
            target_unique_ids=self.pgie_generator.config["property"]["gie-unique-id"],
            tensor_data_type=0,
            tensor_name=meta["input_tensor_name"],
        )
        self.nvsahipreprocess_yml = self.nvsahipreprocess_generator.config

    def init_params(self) -> None:
        super().init_params()
        self.params_yml["sahi"] = self.sahi

    def apply_save_paths(self, config_save_dir: Path) -> None:
        super().apply_save_paths(config_save_dir)
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "sahi_preprocess":
                properties["config-file"] = str(
                    config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        config_save_dir = Path(config_save_dir)
        pipeline_save_path = config_save_dir / self.PIPELINE_CONFIG_NAME
        pad_links_save_path = config_save_dir / self.PAD_LINKS_CONFIG_NAME
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        nvtracker_save_path = config_save_dir / self.TRACKER_CONFIG_NAME
        nvdsanalytics_save_path = config_save_dir / self.ANALYTICS_CONFIG_NAME
        params_save_path = config_save_dir / self.PARAMS_NAME
        sahi_preprocess_save_path = config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
        self.apply_save_paths(config_save_dir)
        shutil.copy2(
            self.pgie_config_parser.meta_path,
            config_save_dir / self.pgie_config_parser.meta_path.name,
        )
        with open(pgie_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.pgie_yml, handle, sort_keys=False, default_flow_style=False)
        if self.enable_nvtracker:
            with open(nvtracker_save_path, "w", encoding="utf-8") as handle:
                yaml.safe_dump(
                    self.nvtracker_yml, handle, sort_keys=False, default_flow_style=False
                )
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
        with open(pad_links_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.pad_links_yml, handle, sort_keys=False, default_flow_style=False
            )
        params = dict(self.params_yml)
        params["config_save_dir"] = str(config_save_dir)
        with open(params_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(params, handle, sort_keys=False, default_flow_style=False)
        self.nvsahipreprocess_generator.write(sahi_preprocess_save_path)
        sahi = self.sahi["nvsahipreprocess"]
        sahi_info = get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=True,
        )
        sahi_show = get_sahi_preview(sahi_info)
        sahi_show.save(config_save_dir / "sahi_slice_preview.jpg")
        with open(config_save_dir / "sahi_slice_info.json", "w", encoding="utf-8") as handle:
            json.dump(sahi_info, handle)

    def add(self) -> None:
        for index, name in enumerate(self.streams):
            self._append_node(
                "nvurisrcbin",
                f"src{index}",
                self._add_nvurisrcbin(self.streams[name]["url"], disable_audio=True),
            )
        self._append_node(
            "nvstreammux",
            "mux",
            self._add_nvstreammux(
                batch_size=self.mux_batch_size,
                width=self.width,
                height=self.height,
                live_source=True,
                enable_padding=False,
                batched_push_timeout=40000,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        sahi = self.sahi["nvsahipreprocess"]
        postprocess = self.sahi["nvsahipostprocess"]
        self._append_node(
            "nvsahipreprocess",
            "sahi_preprocess",
            self._add_nvsahipreprocess(
                self.SAHI_PREPROCESS_CONFIG_NAME,
                slice_width=sahi["slice_width"],
                slice_height=sahi["slice_height"],
                overlap_width_ratio=sahi["overlap_width_ratio"],
                overlap_height_ratio=sahi["overlap_height_ratio"],
                enable_full_frame=True,
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
                input_tensor_meta=True,
            ),
        )
        self._append_node("queue", "queue_sahi", self._add_queue())
        self._append_node(
            "nvsahipostprocess",
            "sahi_postprocess",
            self._add_nvsahipostprocess(
                gie_ids=str(self.pgie_generator.config["property"]["gie-unique-id"]),
                match_metric=1,
                match_threshold=postprocess["match_threshold"],
                class_agnostic=False,
                enable_merge=True,
                two_phase_nmm=True,
            ),
        )
        if self.enable_nvtracker:
            tracker_width, tracker_height = self.tracker_dimensions()
            self._append_node(
                "nvtracker",
                "tracker",
                self._add_nvtracker(
                    TRACKER_LL_LIB,
                    self.TRACKER_CONFIG_NAME,
                    tracker_width=tracker_width,
                    tracker_height=tracker_height,
                    gpu_id=self.pgie_generator.gpu_id,
                ),
            )
        self._append_node(
            "nvdsanalytics",
            "analyzer",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node("nvstreamdemux", "demux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        for index in range(len(self.streams)):
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvidconv{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node("tee", f"tee_raw{index}", self._add_tee())
            self._append_node("queue", f"queue_raw{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_raw{index}",
                self._add_appsink(),
            )
            self._append_node(
                "nvosdbin",
                f"osd{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("queue", f"queue_vis{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_vis{index}",
                self._add_appsink(),
            )

    def link(self) -> None:
        self.pad_links = {"demux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"src{index}"] = "mux"
        edges["mux"] = "sahi_preprocess"
        edges["sahi_preprocess"] = "pgie"
        edges["pgie"] = "queue_sahi"
        edges["queue_sahi"] = "sahi_postprocess"
        inference_tail = "sahi_postprocess"
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        edges[inference_tail] = "analyzer"
        edges["analyzer"] = "demux"
        for index in range(len(self.streams)):
            self.pad_links["demux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvidconv{index}"
            edges[f"nvvidconv{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"osd{index}"]
            edges[f"queue_raw{index}"] = f"appsink_raw{index}"
            edges[f"osd{index}"] = f"queue_vis{index}"
            edges[f"queue_vis{index}"] = f"appsink_vis{index}"
        self.pipeline["deepstream"]["edges"] = edges


class DetSahiVisRTSPGenerator(DetVisRTSPGenerator):
    GENERATOR = "DetSahiVisRTSPGenerator"
    SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini"

    f"""Generate YOLO SAHI detection RTSP pipeline for event alert + probe-side Kafka + live preview.

    Requires ``enable_visualized_rtsp=True``.
    Set ``analyzer=None`` to keep nvdsanalytics inserted with master switch off.
    Set ``tracker=None`` to skip nvtracker. Pass ``tracker={{"class_id": ...}}`` to insert nvtracker. Does not insert ``nvmsgconv`` / ``nvmsgbroker``;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {VIS_RTSP_TOPOLOGY_DOC}
    {SAHI_RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        enable_visualized_rtsp: bool,
        analyzer: dict | None,
        pgie: dict,
        sahi: dict,
        tracker: dict | None = None,
        interval: int = 0,
    ) -> None:
        self.enable_visualized_rtsp = enable_visualized_rtsp
        assert enable_visualized_rtsp, "enable_visualized_rtsp must be True"
        self.sahi = sahi
        self.streams = streams
        self.analyzer = analyzer
        self.tracker = tracker
        self.interval = interval
        self.pgie = pgie
        DeepstreamGenerator.__init__(self)
        self.init_streams()
        self.init_pgie()
        self.init_sahi()
        self.init_nvdsanalytics()
        self.init_nvtracker()
        self.init_params()
        self.init_pipeline()

    def init_streams(self) -> None:
        super().init_streams()
        self.mux_batch_size = self.runtime_batch_size
        sahi = self.sahi["nvsahipreprocess"]
        slice_info = get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=True,
        )
        self.runtime_batch_size = int(slice_info["num"])

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloDetSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def init_sahi(self) -> None:
        meta = self.pgie_config_parser.meta
        input_shape = meta["input_tensor_shape"]
        network_input_shape = ";".join(
            str(value) for value in [self.runtime_batch_size, *input_shape[1:]]
        )
        self.nvsahipreprocess_generator = NvsahipreprocessGenerator(
            network_input_shape=network_input_shape,
            target_unique_ids=self.pgie_generator.config["property"]["gie-unique-id"],
            tensor_data_type=0,
            tensor_name=meta["input_tensor_name"],
        )
        self.nvsahipreprocess_yml = self.nvsahipreprocess_generator.config

    def init_params(self) -> None:
        super().init_params()
        self.params_yml["sahi"] = self.sahi

    def apply_save_paths(self, config_save_dir: Path) -> None:
        super().apply_save_paths(config_save_dir)
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "sahi_preprocess":
                properties["config-file"] = str(
                    config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        config_save_dir = Path(config_save_dir)
        pipeline_save_path = config_save_dir / self.PIPELINE_CONFIG_NAME
        pad_links_save_path = config_save_dir / self.PAD_LINKS_CONFIG_NAME
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        nvtracker_save_path = config_save_dir / self.TRACKER_CONFIG_NAME
        nvdsanalytics_save_path = config_save_dir / self.ANALYTICS_CONFIG_NAME
        params_save_path = config_save_dir / self.PARAMS_NAME
        sahi_preprocess_save_path = config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
        self.apply_save_paths(config_save_dir)
        shutil.copy2(
            self.pgie_config_parser.meta_path,
            config_save_dir / self.pgie_config_parser.meta_path.name,
        )
        with open(pgie_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.pgie_yml, handle, sort_keys=False, default_flow_style=False)
        if self.enable_nvtracker:
            with open(nvtracker_save_path, "w", encoding="utf-8") as handle:
                yaml.safe_dump(
                    self.nvtracker_yml, handle, sort_keys=False, default_flow_style=False
                )
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
        with open(pad_links_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.pad_links_yml, handle, sort_keys=False, default_flow_style=False
            )
        params = dict(self.params_yml)
        params["config_save_dir"] = str(config_save_dir)
        with open(params_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(params, handle, sort_keys=False, default_flow_style=False)
        self.nvsahipreprocess_generator.write(sahi_preprocess_save_path)
        sahi = self.sahi["nvsahipreprocess"]
        sahi_info = get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=True,
        )
        sahi_show = get_sahi_preview(sahi_info)
        sahi_show.save(config_save_dir / "sahi_slice_preview.jpg")
        with open(config_save_dir / "sahi_slice_info.json", "w", encoding="utf-8") as handle:
            json.dump(sahi_info, handle)

    def add(self) -> None:
        for index, name in enumerate(self.streams):
            self._append_node(
                "nvurisrcbin",
                f"src{index}",
                self._add_nvurisrcbin(self.streams[name]["url"], disable_audio=True),
            )
        self._append_node(
            "nvstreammux",
            "mux",
            self._add_nvstreammux(
                batch_size=self.mux_batch_size,
                width=self.width,
                height=self.height,
                live_source=True,
                enable_padding=False,
                batched_push_timeout=40000,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        sahi = self.sahi["nvsahipreprocess"]
        postprocess = self.sahi["nvsahipostprocess"]
        self._append_node(
            "nvsahipreprocess",
            "sahi_preprocess",
            self._add_nvsahipreprocess(
                self.SAHI_PREPROCESS_CONFIG_NAME,
                slice_width=sahi["slice_width"],
                slice_height=sahi["slice_height"],
                overlap_width_ratio=sahi["overlap_width_ratio"],
                overlap_height_ratio=sahi["overlap_height_ratio"],
                enable_full_frame=True,
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
                input_tensor_meta=True,
            ),
        )
        self._append_node("queue", "queue_sahi", self._add_queue())
        self._append_node(
            "nvsahipostprocess",
            "sahi_postprocess",
            self._add_nvsahipostprocess(
                gie_ids=str(self.pgie_generator.config["property"]["gie-unique-id"]),
                match_metric=1,
                match_threshold=postprocess["match_threshold"],
                class_agnostic=False,
                enable_merge=True,
                two_phase_nmm=True,
            ),
        )
        if self.enable_nvtracker:
            tracker_width, tracker_height = self.tracker_dimensions()
            self._append_node(
                "nvtracker",
                "tracker",
                self._add_nvtracker(
                    TRACKER_LL_LIB,
                    self.TRACKER_CONFIG_NAME,
                    tracker_width=tracker_width,
                    tracker_height=tracker_height,
                    gpu_id=self.pgie_generator.gpu_id,
                ),
            )
        self._append_node(
            "nvdsanalytics",
            "analyzer",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node("nvstreamdemux", "demux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        for index, name in enumerate(self.streams):
            sink_uri = self.visualized_sink_uri(self.streams[name]["url"])
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvidconv{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node("tee", f"tee_raw{index}", self._add_tee())
            self._append_node("queue", f"queue_raw{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_raw{index}",
                self._add_appsink(),
            )
            self._append_node(
                "nvosdbin",
                f"osd{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("tee", f"tee_vis{index}", self._add_tee())
            self._append_node("queue", f"queue_vis{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_vis{index}",
                self._add_appsink(),
            )
            self._append_node(
                "nvv4l2h264enc",
                f"encoder{index}",
                self._add_nvv4l2h264enc(
                    bitrate=4_000_000,
                    iframeinterval=self.fps,
                    preset_id=1,
                    gpu_id=gpu_id,
                ),
            )
            self._append_node("h264parse", f"h264parse{index}", self._add_h264parse())
            self._append_node(
                "rtspclientsink",
                f"sink{index}",
                self._add_rtspclientsink(location=sink_uri, sync=False, async_=False),
            )

    def link(self) -> None:
        self.pad_links = {"demux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"src{index}"] = "mux"
        edges["mux"] = "sahi_preprocess"
        edges["sahi_preprocess"] = "pgie"
        edges["pgie"] = "queue_sahi"
        edges["queue_sahi"] = "sahi_postprocess"
        inference_tail = "sahi_postprocess"
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        edges[inference_tail] = "analyzer"
        edges["analyzer"] = "demux"
        for index in range(len(self.streams)):
            self.pad_links["demux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvidconv{index}"
            edges[f"nvvidconv{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"osd{index}"]
            edges[f"queue_raw{index}"] = f"appsink_raw{index}"
            edges[f"osd{index}"] = f"tee_vis{index}"
            edges[f"tee_vis{index}"] = [f"queue_vis{index}", f"encoder{index}"]
            edges[f"queue_vis{index}"] = f"appsink_vis{index}"
            edges[f"encoder{index}"] = f"h264parse{index}"
            edges[f"h264parse{index}"] = f"sink{index}"
        self.pipeline["deepstream"]["edges"] = edges
