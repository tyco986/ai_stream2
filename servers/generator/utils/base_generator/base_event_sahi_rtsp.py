from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .base_sahi_rtsp import BaseSahiRTSPGenerator

SAHI_RTSP_EVENT_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
              → nvtracker → nvdsanalytics → nvstreamdemux
              → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
                    ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → appsink_raw{N}
                    └→ queue_osd{N} → nvvideoconvert_osd{N} → capsfilter_osd{N}(RGBA) → nvosdbin{N}
                          → queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → appsink_vis{N}

    Notes::

        ``mux`` batch size is the stream count; ``pgie`` batch size is the SAHI tile count.
        No RTSP preview sink: capture ends at appsinks only.
        ``nvvideoconvert_raw`` / ``nvvideoconvert_osd`` after ``tee_raw`` force independent NVMM
        buffers so ``nvosd`` in-place drawing cannot leak into the raw capture branch.
        Capture branches force ``format=RGB`` (BufferRetriever extract requirement).

    Python (not in pipeline.yml)::

        attach(nvdsanalytics, Probe)   # logger → debouncer → drawer → messager
        attach(appsink_raw{N}, Receiver)
        attach(appsink_vis{N}, Receiver)
"""


class BaseEventSahiRTSPGenerator(BaseSahiRTSPGenerator):
    SINK_PATH_TEMPLATES = {
        "appsink_raw{index}": [
            "nvurisrcbin{index}",
            "nvstreammux",
            "nvsahipreprocess",
            "pgie",
            "queue_sahi",
            "nvsahipostprocess",
            "nvtracker",
            "nvdsanalytics",
            "nvstreamdemux",
            "queue_demux{index}",
            "nvvideoconvert{index}",
            "tee_raw{index}",
            "queue_raw{index}",
            "nvvideoconvert_raw{index}",
            "capsfilter_raw{index}",
            "appsink_raw{index}",
        ],
        "appsink_vis{index}": [
            "nvurisrcbin{index}",
            "nvstreammux",
            "nvsahipreprocess",
            "pgie",
            "queue_sahi",
            "nvsahipostprocess",
            "nvtracker",
            "nvdsanalytics",
            "nvstreamdemux",
            "queue_demux{index}",
            "nvvideoconvert{index}",
            "tee_raw{index}",
            "queue_osd{index}",
            "nvvideoconvert_osd{index}",
            "capsfilter_osd{index}",
            "nvosdbin{index}",
            "queue_vis{index}",
            "nvvideoconvert_vis{index}",
            "capsfilter_vis{index}",
            "appsink_vis{index}",
        ],
    }

    f"""Generate YOLO SAHI RTSP pipeline for event alert + appsink capture.

    Per-stream branches tee raw/vis appsinks; no ``rtspclientsink`` preview.
    {SAHI_RTSP_EVENT_TOPOLOGY_DOC}
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
                enable_full_frame=sahi.get("enable_full_frame", True),
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
        for index in range(len(self.streams)):
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node("tee", f"tee_raw{index}", self._add_tee())
            self._append_node("queue", f"queue_raw{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert_raw{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"capsfilter_raw{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
            )
            self._append_node("appsink", f"appsink_raw{index}", self._add_appsink())
            self._append_node("queue", f"queue_osd{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert_osd{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"capsfilter_osd{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGBA"),
            )
            self._append_node(
                "nvosdbin",
                f"nvosdbin{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("queue", f"queue_vis{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert_vis{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"capsfilter_vis{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
            )
            self._append_node("appsink", f"appsink_vis{index}", self._add_appsink())

    def link(self) -> None:
        self.pad_links = {"nvstreamdemux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"nvurisrcbin{index}"] = "nvstreammux"
        edges["nvstreammux"] = "nvsahipreprocess"
        edges["nvsahipreprocess"] = "pgie"
        edges["pgie"] = "queue_sahi"
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
            edges[f"nvvideoconvert{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"queue_osd{index}"]
            edges[f"queue_raw{index}"] = f"nvvideoconvert_raw{index}"
            edges[f"nvvideoconvert_raw{index}"] = f"capsfilter_raw{index}"
            edges[f"capsfilter_raw{index}"] = f"appsink_raw{index}"
            edges[f"queue_osd{index}"] = f"nvvideoconvert_osd{index}"
            edges[f"nvvideoconvert_osd{index}"] = f"capsfilter_osd{index}"
            edges[f"capsfilter_osd{index}"] = f"nvosdbin{index}"
            edges[f"nvosdbin{index}"] = f"queue_vis{index}"
            edges[f"queue_vis{index}"] = f"nvvideoconvert_vis{index}"
            edges[f"nvvideoconvert_vis{index}"] = f"capsfilter_vis{index}"
            edges[f"capsfilter_vis{index}"] = f"appsink_vis{index}"
        self.pipeline["deepstream"]["edges"] = edges
