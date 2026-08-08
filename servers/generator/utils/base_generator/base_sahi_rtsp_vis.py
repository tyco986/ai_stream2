from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .base_sahi_rtsp import BaseSahiRTSPGenerator

SAHI_VIS_RTSP_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin{N} → nvstreammux → nvsahipreprocess → nvinfer → queue_sahi → nvsahipostprocess
              → nvtracker → nvdsanalytics → nvstreamdemux
              → queue_demux{N} → nvvideoconvert{N} → nvosdbin{N}
              → queue_enc{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}

    Notes::

        ``mux`` batch size is the stream count; ``pgie`` batch size is the SAHI tile count.

    Python (not in pipeline.yml)::

        attach(nvdsanalytics, Probe)   # logger → drawer → messager
"""


class BaseSahiVisRTSPGenerator(BaseSahiRTSPGenerator):
    SINK_PATH_TEMPLATES = {
        "rtspclientsink{index}": [
            "nvurisrcbin{index}",
            "nvstreammux",
            "nvsahipreprocess",
            "nvinfer",
            "queue_sahi",
            "nvsahipostprocess",
            "nvtracker",
            "nvdsanalytics",
            "nvstreamdemux",
            "queue_demux{index}",
            "nvvideoconvert{index}",
            "nvosdbin{index}",
            "queue_enc{index}",
            "nvv4l2h264enc{index}",
            "h264parse{index}",
            "rtspclientsink{index}",
        ],
    }

    f"""Generate YOLO SAHI RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {SAHI_VIS_RTSP_TOPOLOGY_DOC}
    """

    def add(self) -> None:
        for index, name in enumerate(self.streams):
            self._append_node(
                "nvurisrcbin",
                f"nvurisrcbin{index}",
                self._add_nvurisrcbin(self.streams[name]["url"], disable_audio=True),
            )
        self._append_node(
            "nvstreammux",
            "nvstreammux",
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
            "nvsahipreprocess",
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
            "nvinfer",
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
            "nvsahipostprocess",
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
            self._append_node(
                "nvtracker",
                "nvtracker",
                self._add_nvtracker(
                    TRACKER_LL_LIB,
                    self.TRACKER_CONFIG_NAME,
                    tracker_width=self.tracker_width,
                    tracker_height=self.tracker_height,
                    gpu_id=self.pgie_generator.gpu_id,
                    operate_on_class_ids=self.operate_on_class_ids,
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
        self._append_node("nvstreamdemux", "nvstreamdemux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        for index, name in enumerate(self.streams):
            sink_uri = self.visualized_sink_uri(self.streams[name]["url"])
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "nvosdbin",
                f"nvosdbin{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("queue", f"queue_enc{index}", self._add_queue())
            self._append_node(
                "nvv4l2h264enc",
                f"nvv4l2h264enc{index}",
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
                f"rtspclientsink{index}",
                self._add_rtspclientsink(location=sink_uri, sync=False, async_=False),
            )

    def link(self) -> None:
        self.pad_links = {"nvstreamdemux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"nvurisrcbin{index}"] = "nvstreammux"
        edges["nvstreammux"] = "nvsahipreprocess"
        edges["nvsahipreprocess"] = "nvinfer"
        edges["nvinfer"] = "queue_sahi"
        edges["queue_sahi"] = "nvsahipostprocess"
        inference_tail = "nvsahipostprocess"
        if self.enable_nvtracker:
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "nvdsanalytics"
        edges["nvdsanalytics"] = "nvstreamdemux"
        for index in range(len(self.streams)):
            self.pad_links["nvstreamdemux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvideoconvert{index}"
            edges[f"nvvideoconvert{index}"] = f"nvosdbin{index}"
            edges[f"nvosdbin{index}"] = f"queue_enc{index}"
            edges[f"queue_enc{index}"] = f"nvv4l2h264enc{index}"
            edges[f"nvv4l2h264enc{index}"] = f"h264parse{index}"
            edges[f"h264parse{index}"] = f"rtspclientsink{index}"
        self.pipeline["deepstream"]["edges"] = edges
