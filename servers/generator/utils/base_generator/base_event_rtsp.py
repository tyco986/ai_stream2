from ..subelement_generator.pipeline import TRACKER_LL_LIB

RTSP_EVENT_TOPOLOGY_DOC = """
    Topology::

        src{N} → mux → pgie → tracker → analyzer → demux
              → queue_demux{N} → nvvidconv{N} → tee_raw{N}
                    ─┬→ queue_raw{N} → nvvidconv_raw{N} → caps_raw{N} → appsink_raw{N}
                    └→ queue_osd{N} → nvvidconv_osd{N} → caps_osd{N}(RGBA) → osd{N}
                          → queue_vis{N} → nvvidconv_vis{N} → caps_vis{N} → appsink_vis{N}

    Notes::

        No RTSP preview sink: capture ends at appsinks only.
        ``nvvidconv_raw`` / ``nvvidconv_osd`` after ``tee_raw`` force independent NVMM
        buffers so ``nvosd`` in-place drawing cannot leak into the raw capture branch.
        Capture branches force ``format=RGB`` (BufferRetriever extract requirement).

    Python (not in pipeline.yml)::

        attach(analyzer, Probe)   # logger → debouncer → drawer → messager
        attach(appsink_raw{N}, Receiver)
        attach(appsink_vis{N}, Receiver)
"""


class BaseEventRTSPGenerator:
    f"""Generate YOLO RTSP pipeline for event alert + appsink capture.

    Per-stream branches tee raw/vis appsinks; no ``rtspclientsink`` preview.
    {RTSP_EVENT_TOPOLOGY_DOC}
    """

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
        }

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
                batch_size=len(self.streams),
                width=self.width,
                height=self.height,
                live_source=True,
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
                "nvvideoconvert",
                f"nvvidconv_raw{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"caps_raw{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
            )
            self._append_node("appsink", f"appsink_raw{index}", self._add_appsink())
            self._append_node("queue", f"queue_osd{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvidconv_osd{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"caps_osd{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGBA"),
            )
            self._append_node(
                "nvosdbin",
                f"osd{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("queue", f"queue_vis{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvidconv_vis{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"caps_vis{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
            )
            self._append_node("appsink", f"appsink_vis{index}", self._add_appsink())

    def link(self) -> None:
        self.pad_links = {"demux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"src{index}"] = "mux"
        edges["mux"] = "pgie"
        inference_tail = "pgie"
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        edges[inference_tail] = "analyzer"
        edges["analyzer"] = "demux"
        for index in range(len(self.streams)):
            self.pad_links["demux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvidconv{index}"
            edges[f"nvvidconv{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"queue_osd{index}"]
            edges[f"queue_raw{index}"] = f"nvvidconv_raw{index}"
            edges[f"nvvidconv_raw{index}"] = f"caps_raw{index}"
            edges[f"caps_raw{index}"] = f"appsink_raw{index}"
            edges[f"queue_osd{index}"] = f"nvvidconv_osd{index}"
            edges[f"nvvidconv_osd{index}"] = f"caps_osd{index}"
            edges[f"caps_osd{index}"] = f"osd{index}"
            edges[f"osd{index}"] = f"queue_vis{index}"
            edges[f"queue_vis{index}"] = f"nvvidconv_vis{index}"
            edges[f"nvvidconv_vis{index}"] = f"caps_vis{index}"
            edges[f"caps_vis{index}"] = f"appsink_vis{index}"
        self.pipeline["deepstream"]["edges"] = edges
