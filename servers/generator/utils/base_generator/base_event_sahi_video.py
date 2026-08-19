from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .base_sahi_video import BaseSahiVideoGenerator

SAHI_VIDEO_EVENT_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → nvtracker → nvdsanalytics → nvvideoconvert → tee_raw
              ─┬→ queue_raw → nvvideoconvert_raw → capsfilter_raw → appsink_raw0
              └→ queue_osd → nvvideoconvert_osd → capsfilter_osd(RGBA) → nvosdbin → tee_vis
                    ─┬→ queue_vis → nvvideoconvert_vis → capsfilter_vis → appsink_vis0
                    └→ queue_enc → nvv4l2h264enc → h264parse → mp4mux → filesink

    Notes::

        ``mux`` batch size is 1; ``pgie`` batch size is the SAHI tile count.
        ``nvvideoconvert_raw`` / ``nvvideoconvert_osd`` after ``tee_raw`` force independent NVMM
        buffers so ``nvosd`` in-place drawing cannot leak into the raw capture branch.
        Capture branches force ``format=RGB`` (BufferRetriever extract requirement).

    Python (not in pipeline.yml)::

        attach(nvdsanalytics, Probe)   # logger → debouncer → drawer → messager
        attach(appsink_raw0, Receiver)   # encode PNG when event alert matches
        attach(appsink_vis0, Receiver)   # encode JPEG when event alert matches
"""


class BaseEventSahiVideoGenerator(BaseSahiVideoGenerator):
    SINK_PATH_TEMPLATES = {
        "appsink_raw0": [
            "nvurisrcbin",
            "nvstreammux",
            "nvsahipreprocess",
            "pgie",
            "queue_sahi",
            "nvsahipostprocess",
            "nvtracker",
            "nvdsanalytics",
            "nvvideoconvert",
            "tee_raw",
            "queue_raw",
            "nvvideoconvert_raw",
            "capsfilter_raw",
            "appsink_raw0",
        ],
        "appsink_vis0": [
            "nvurisrcbin",
            "nvstreammux",
            "nvsahipreprocess",
            "pgie",
            "queue_sahi",
            "nvsahipostprocess",
            "nvtracker",
            "nvdsanalytics",
            "nvvideoconvert",
            "tee_raw",
            "queue_osd",
            "nvvideoconvert_osd",
            "capsfilter_osd",
            "nvosdbin",
            "tee_vis",
            "queue_vis",
            "nvvideoconvert_vis",
            "capsfilter_vis",
            "appsink_vis0",
        ],
        "filesink": [
            "nvurisrcbin",
            "nvstreammux",
            "nvsahipreprocess",
            "pgie",
            "queue_sahi",
            "nvsahipostprocess",
            "nvtracker",
            "nvdsanalytics",
            "nvvideoconvert",
            "tee_raw",
            "queue_osd",
            "nvvideoconvert_osd",
            "capsfilter_osd",
            "nvosdbin",
            "tee_vis",
            "queue_enc",
            "nvv4l2h264enc",
            "h264parse",
            "mp4mux",
            "filesink",
        ],
    }

    f"""Generate YOLO SAHI video pipeline for event alert + appsink capture.

    Reads ``input`` video via DeepStream, runs SAHI inference with event capture
    branches, and writes the annotated result to ``output``.
    {SAHI_VIDEO_EVENT_TOPOLOGY_DOC}
    """

    def add(self) -> None:
        self._append_node(
            "nvurisrcbin",
            "nvurisrcbin",
            self._add_nvurisrcbin(
                self.file_uri(self.input),
                disable_audio=True,
            ),
        )
        self._append_node(
            "nvstreammux",
            "nvstreammux",
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
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.osd_kwargs(gpu_id)
        self._append_node(
            "nvvideoconvert",
            "nvvideoconvert",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node("tee", "tee_raw", self._add_tee())
        self._append_node("queue", "queue_raw", self._add_queue())
        self._append_node(
            "nvvideoconvert",
            "nvvideoconvert_raw",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "capsfilter",
            "capsfilter_raw",
            self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
        )
        self._append_node("appsink", "appsink_raw0", self._add_appsink())
        self._append_node("queue", "queue_osd", self._add_queue())
        self._append_node(
            "nvvideoconvert",
            "nvvideoconvert_osd",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "capsfilter",
            "capsfilter_osd",
            self._add_capsfilter("video/x-raw(memory:NVMM), format=RGBA"),
        )
        self._append_node("nvosdbin", "nvosdbin", self._add_nvosdbin(**osd_kwargs))
        self._append_node("tee", "tee_vis", self._add_tee())
        self._append_node("queue", "queue_vis", self._add_queue())
        self._append_node(
            "nvvideoconvert",
            "nvvideoconvert_vis",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "capsfilter",
            "capsfilter_vis",
            self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
        )
        self._append_node("queue", "queue_enc", self._add_queue())
        self._append_node("appsink", "appsink_vis0", self._add_appsink())
        self._append_node(
            "nvv4l2h264enc",
            "nvv4l2h264enc",
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
            "filesink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "nvurisrcbin": "nvstreammux",
            "nvstreammux": "nvsahipreprocess",
            "nvsahipreprocess": "pgie",
            "pgie": "queue_sahi",
            "queue_sahi": "nvsahipostprocess",
        }
        inference_tail = "nvsahipostprocess"
        if self.enable_nvtracker:
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "nvdsanalytics"
        edges["nvdsanalytics"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "tee_raw"
        edges["tee_raw"] = ["queue_raw", "queue_osd"]
        edges["queue_raw"] = "nvvideoconvert_raw"
        edges["nvvideoconvert_raw"] = "capsfilter_raw"
        edges["capsfilter_raw"] = "appsink_raw0"
        edges["queue_osd"] = "nvvideoconvert_osd"
        edges["nvvideoconvert_osd"] = "capsfilter_osd"
        edges["capsfilter_osd"] = "nvosdbin"
        edges["nvosdbin"] = "tee_vis"
        edges["tee_vis"] = ["queue_vis", "queue_enc"]
        edges["queue_vis"] = "nvvideoconvert_vis"
        edges["nvvideoconvert_vis"] = "capsfilter_vis"
        edges["capsfilter_vis"] = "appsink_vis0"
        edges["queue_enc"] = "nvv4l2h264enc"
        edges["nvv4l2h264enc"] = "h264parse"
        edges["h264parse"] = "mp4mux"
        edges["mp4mux"] = "filesink"
        self.pipeline["deepstream"]["edges"] = edges
