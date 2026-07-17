from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .base_event_video import BaseEventVideoGenerator

SAHI_VIDEO_EVENT_TOPOLOGY_DOC = """
    Topology::

        src → mux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → tracker → analyzer → nvvidconv → tee_raw
              ─┬→ queue_raw → nvvidconv_raw → caps_raw → appsink_raw0
              └→ queue_osd → nvvidconv_osd → caps_osd(RGBA) → osd → tee_vis
                    ─┬→ queue_vis → nvvidconv_vis → caps_vis → appsink_vis0
                    └→ queue_enc → encoder → h264parse → mp4mux → filesink

    Notes::

        ``mux`` batch size is 1; ``pgie`` batch size is the SAHI tile count.
        ``nvvidconv_raw`` / ``nvvidconv_osd`` after ``tee_raw`` force independent NVMM
        buffers so ``nvosd`` in-place drawing cannot leak into the raw capture branch.
        Capture branches force ``format=RGB`` (BufferRetriever extract requirement).

    Python (not in pipeline.yml)::

        attach(analyzer, Probe)   # logger → debouncer → drawer → messager
        attach(appsink_raw0, Receiver)   # encode PNG when event alert matches
        attach(appsink_vis0, Receiver)   # encode JPEG when event alert matches
"""


class BaseEventSahiVideoGenerator(BaseEventVideoGenerator):
    f"""Generate YOLO SAHI video pipeline for event alert + appsink capture.

    Reads ``input`` video via DeepStream, runs SAHI inference with event capture
    branches, and writes the annotated result to ``output``.
    {SAHI_VIDEO_EVENT_TOPOLOGY_DOC}
    """

    def add(self) -> None:
        self._append_node(
            "nvurisrcbin",
            "src",
            self._add_nvurisrcbin(
                self.file_uri(self.input),
                disable_audio=True,
            ),
        )
        self._append_node(
            "nvstreammux",
            "mux",
            self._add_nvstreammux(
                batch_size=self.mux_batch_size,
                width=self.width,
                height=self.height,
                live_source=False,
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
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        self._append_node(
            "nvvideoconvert",
            "nvvidconv",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node("tee", "tee_raw", self._add_tee())
        self._append_node("queue", "queue_raw", self._add_queue())
        self._append_node(
            "nvvideoconvert",
            "nvvidconv_raw",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "capsfilter",
            "caps_raw",
            self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
        )
        self._append_node("appsink", "appsink_raw0", self._add_appsink())
        self._append_node("queue", "queue_osd", self._add_queue())
        self._append_node(
            "nvvideoconvert",
            "nvvidconv_osd",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "capsfilter",
            "caps_osd",
            self._add_capsfilter("video/x-raw(memory:NVMM), format=RGBA"),
        )
        self._append_node("nvosdbin", "osd", self._add_nvosdbin(**osd_kwargs))
        self._append_node("tee", "tee_vis", self._add_tee())
        self._append_node("queue", "queue_vis", self._add_queue())
        self._append_node(
            "nvvideoconvert",
            "nvvidconv_vis",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "capsfilter",
            "caps_vis",
            self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
        )
        self._append_node("queue", "queue_enc", self._add_queue())
        self._append_node("appsink", "appsink_vis0", self._add_appsink())
        self._append_node(
            "nvv4l2h264enc",
            "encoder",
            self._add_nvv4l2h264enc(
                bitrate=4_000_000,
                iframeinterval=self.fps,
                preset_id=1,
                gpu_id=gpu_id,
            ),
        )
        self._append_node("h264parse", "h264parse", self._add_h264parse())
        self._append_node("mp4mux", "mp4mux", self._add_mp4mux())
        self._append_node(
            "filesink",
            "sink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "src": "mux",
            "mux": "sahi_preprocess",
            "sahi_preprocess": "pgie",
            "pgie": "queue_sahi",
            "queue_sahi": "sahi_postprocess",
        }
        inference_tail = "sahi_postprocess"
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        edges[inference_tail] = "analyzer"
        edges["analyzer"] = "nvvidconv"
        edges["nvvidconv"] = "tee_raw"
        edges["tee_raw"] = ["queue_raw", "queue_osd"]
        edges["queue_raw"] = "nvvidconv_raw"
        edges["nvvidconv_raw"] = "caps_raw"
        edges["caps_raw"] = "appsink_raw0"
        edges["queue_osd"] = "nvvidconv_osd"
        edges["nvvidconv_osd"] = "caps_osd"
        edges["caps_osd"] = "osd"
        edges["osd"] = "tee_vis"
        edges["tee_vis"] = ["queue_vis", "queue_enc"]
        edges["queue_vis"] = "nvvidconv_vis"
        edges["nvvidconv_vis"] = "caps_vis"
        edges["caps_vis"] = "appsink_vis0"
        edges["queue_enc"] = "encoder"
        edges["encoder"] = "h264parse"
        edges["h264parse"] = "mp4mux"
        edges["mp4mux"] = "sink"
        self.pipeline["deepstream"]["edges"] = edges
