from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .base_sahi_rtsp import BaseSahiRTSPGenerator

SAHI_VIS_RTSP_TOPOLOGY_DOC = """
    Topology::

        src{N} → mux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
              → tracker → analyzer → demux
              → queue_demux{N} → nvvidconv{N} → osd{N}
              → queue_enc{N} → encoder{N} → h264parse{N} → rtspclientsink{N}

    Notes::

        ``mux`` batch size is the stream count; ``pgie`` batch size is the SAHI tile count.

    Python (not in pipeline.yml)::

        attach(analyzer, Probe)   # logger → drawer → messager
"""


class BaseSahiVisRTSPGenerator(BaseSahiRTSPGenerator):
    f"""Generate YOLO SAHI RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {SAHI_VIS_RTSP_TOPOLOGY_DOC}
    """

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
            self._append_node(
                "nvtracker",
                "tracker",
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
            self._append_node(
                "nvosdbin",
                f"osd{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("queue", f"queue_enc{index}", self._add_queue())
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
            edges[f"nvvidconv{index}"] = f"osd{index}"
            edges[f"osd{index}"] = f"queue_enc{index}"
            edges[f"queue_enc{index}"] = f"encoder{index}"
            edges[f"encoder{index}"] = f"h264parse{index}"
            edges[f"h264parse{index}"] = f"sink{index}"
        self.pipeline["deepstream"]["edges"] = edges
