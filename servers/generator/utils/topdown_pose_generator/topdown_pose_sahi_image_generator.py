from pathlib import Path

from ..base_generator.base_sahi_image import BaseSahiImageGenerator
from .topdown_pose_mixin import TopdownPoseMixin

TOPDOWN_POSE_SAHI_IMAGE_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → nvdspreprocess_rtmpose → sgie0 → nvrtmposepostprocess → nvdsanalytics
            → nvosdbin → nvvideoconvert → nvjpegenc → filesink

    Notes::

        ``mux`` batch size is 1; ``pgie`` batch size is the SAHI tile count.
"""


class TopdownPoseSahiImageGenerator(TopdownPoseMixin, BaseSahiImageGenerator):
    GENERATOR = "TopdownPoseSahiImageGenerator"

    f"""Generate topdown-pose SAHI image pipeline YAML.

    {TOPDOWN_POSE_SAHI_IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        pipeline_name: str,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
        sahi: dict,
        logger: dict | None = None,
        drawer: dict | None = None,
        event_coder: dict | None = None,
    ) -> None:
        self.sgie = sgie
        super().__init__(
            pipeline_name=pipeline_name,
            input=input,
            output=output,
            analyzer=analyzer,
            pgie=pgie,
            sahi=sahi,
            logger=logger,
            drawer=drawer,
            event_coder=event_coder,
        )

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
        self.append_sgie_node()
        self._append_node(
            "nvdsanalytics",
            "nvdsanalytics",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self.append_event_coder()
        self.append_kafka_nodes()
        gpu_id = self.pgie_generator.gpu_id
        if self.drawer is not None:
            drawer = self.drawer
            self._append_node(
                self.nvpose_drawer_element(),
                "nvposefadedrawer",
                self.nvpose_drawer_properties(drawer),
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
            "nvstreammux": "nvsahipreprocess",
            "nvsahipreprocess": "pgie",
            "pgie": "queue_sahi",
            "queue_sahi": "nvsahipostprocess",
        }
        self.link_sgie_from(edges, "nvsahipostprocess")
        edges[self.pose_gie_tail()] = "nvdsanalytics"
        self.link_kafka_from_analytics(edges, self.vis_tee_next())
        self.link_drawer_before_osd(edges)
        edges.update({
            "queue_msg": "nvmsgconv",
            "nvmsgconv": "nvmsgbroker",
            "nvosdbin": "nvvideoconvert",
            "nvvideoconvert": "nvdetlogger",
            "nvdetlogger": "nvjpegenc",
            "nvjpegenc": "filesink",
        })
        self.pipeline["deepstream"]["edges"] = edges
