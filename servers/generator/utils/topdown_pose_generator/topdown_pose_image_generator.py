from pathlib import Path

from ..base_generator.base_image import BaseImageGenerator
from .topdown_pose_mixin import TopdownPoseMixin

TOPDOWN_POSE_IMAGE_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → pgie → nvdspreprocess_rtmpose → sgie0
            → nvrtmposepostprocess → nvdsanalytics → tee → nvposefadedrawer → nvosdbin
            → nvvideoconvert → nvjpegenc → filesink
"""


class TopdownPoseImageGenerator(TopdownPoseMixin, BaseImageGenerator):
    GENERATOR = "TopdownPoseImageGenerator"

    f"""Generate topdown-pose image pipeline YAML.

    {TOPDOWN_POSE_IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        pipeline_name: str,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
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
            "nvstreammux": "pgie",
        }
        self.link_sgie_from(edges, "pgie")
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
