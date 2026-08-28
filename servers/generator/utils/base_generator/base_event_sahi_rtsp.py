from ..subelement_generator.kafka import KAFKA_CONN_STR, KAFKA_PROTO_LIB
from ..subelement_generator.nvmsgconv import PAYLOAD_DEEPSTREAM_MINIMAL
from ..subelement_generator.nvtracker import TRACKER_LL_LIB
from .base_sahi_rtsp import BaseSahiRTSPGenerator

SAHI_RTSP_EVENT_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
              → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
                ─┬→ nvstreamdemux
              → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
                    ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → nvrawcapturer{N} → fakesink_raw{N}
                    └→ queue_osd{N} → nvvideoconvert_osd{N} → capsfilter_osd{N}(RGBA) → nvosdbin{N}
                          → nvdetlogger{N} → queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → nvviscapturer{N} → fakesink_vis{N}

    Notes::

        ``mux`` batch size is the stream count; ``pgie`` batch size is the SAHI tile count.
        No RTSP preview sink: capture ends at capturer + fakesink.
        ``nvvideoconvert_raw`` / ``nvvideoconvert_osd`` after ``tee_raw`` force independent NVMM
        buffers so ``nvosd`` in-place drawing cannot leak into the raw capture branch.
        Capture branches force ``format=RGB`` (``nvrawcapturer`` / ``nvviscapturer`` NVMM caps).

"""


class BaseEventSahiRTSPGenerator(BaseSahiRTSPGenerator):
    f"""Generate YOLO SAHI RTSP pipeline for event alert + nvcapturer dump.

    Per-stream branches tee raw/vis capturers; no ``rtspclientsink`` preview.
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
            if self.drawer is not None:
                self._append_node(
                    "nvbboxsnapshot",
                    "nvbboxsnapshot",
                    self._add_nvbboxsnapshot(),
                )
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
            self._append_node(
                "nvrawcapturer",
                f"nvrawcapturer{index}",
                self._add_nvrawcapturer(),
            )
            self._append_node(
                "fakesink",
                f"fakesink_raw{index}",
                self._add_fakesink(sync=False, async_=False),
            )
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
            if self.drawer is not None:
                drawer = self.drawer
                self._append_node(
                    self.nvdet_drawer_element(),
                    f"nvdetfadedrawer{index}",
                    self.nvdet_drawer_properties(drawer),
                )
            self._append_node(
                "nvosdbin",
                f"nvosdbin{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node(
                "nvdetlogger",
                f"nvdetlogger{index}",
                self._add_nvdetlogger(
                    root=f"/root/logs/deepstream/{self.pipeline_name}",
                    interval=int(self.logger.get("interval", 0)),
                ),
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
            self._append_node(
                "nvviscapturer",
                f"nvviscapturer{index}",
                self._add_nvviscapturer(),
            )
            self._append_node(
                "fakesink",
                f"fakesink_vis{index}",
                self._add_fakesink(sync=False, async_=False),
            )

    def link(self) -> None:
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"nvurisrcbin{index}"] = "nvstreammux"
        edges["nvstreammux"] = "nvsahipreprocess"
        edges["nvsahipreprocess"] = "pgie"
        edges["pgie"] = "queue_sahi"
        edges["queue_sahi"] = "nvsahipostprocess"
        inference_tail = "nvsahipostprocess"
        if self.enable_nvtracker:
            if self.drawer is not None:
                edges[inference_tail] = "nvbboxsnapshot"
                edges["nvbboxsnapshot"] = "nvtracker"
            else:
                edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "nvdsanalytics"
        edges["nvdsanalytics"] = self.after_analytics()
        self.link_event_coder(edges)
        edges["tee_msg"] = ["nvstreamdemux", "queue_msg"]
        edges["queue_msg"] = "nvmsgconv"
        edges["nvmsgconv"] = "nvmsgbroker"
        for index in range(len(self.streams)):
            edges[f"queue_demux{index}"] = f"nvvideoconvert{index}"
            edges[f"nvvideoconvert{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"queue_osd{index}"]
            edges[f"queue_raw{index}"] = f"nvvideoconvert_raw{index}"
            edges[f"nvvideoconvert_raw{index}"] = f"capsfilter_raw{index}"
            edges[f"capsfilter_raw{index}"] = f"nvrawcapturer{index}"
            edges[f"nvrawcapturer{index}"] = f"fakesink_raw{index}"
            edges[f"queue_osd{index}"] = f"nvvideoconvert_osd{index}"
            edges[f"nvvideoconvert_osd{index}"] = f"capsfilter_osd{index}"
            osd_prev = f"nvosdbin{index}"
            if self.drawer is not None:
                osd_prev = f"nvdetfadedrawer{index}"
            edges[f"capsfilter_osd{index}"] = osd_prev
            if self.drawer is not None:
                edges[f"nvdetfadedrawer{index}"] = f"nvosdbin{index}"
            edges[f"nvosdbin{index}"] = f"nvdetlogger{index}"
            edges[f"nvdetlogger{index}"] = f"queue_vis{index}"
            edges[f"queue_vis{index}"] = f"nvvideoconvert_vis{index}"
            edges[f"nvvideoconvert_vis{index}"] = f"capsfilter_vis{index}"
            edges[f"capsfilter_vis{index}"] = f"nvviscapturer{index}"
            edges[f"nvviscapturer{index}"] = f"fakesink_vis{index}"
        self.pipeline["deepstream"]["edges"] = edges
