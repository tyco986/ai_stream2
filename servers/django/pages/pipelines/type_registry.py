PIPELINE_TYPES = (
    "DetImagePipeline",
    "DetSahiImagePipeline",
    "SegImagePipeline",
    "SegSahiImagePipeline",
    "DetVideoPipeline",
    "DetVisVideoPipeline",
    "DetSahiVideoPipeline",
    "DetSahiVisVideoPipeline",
    "SegVideoPipeline",
    "SegVisVideoPipeline",
    "SegSahiVideoPipeline",
    "SegSahiVisVideoPipeline",
    "DetRTSPPipeline",
    "DetVisRTSPPipeline",
    "DetSahiRTSPPipeline",
    "DetSahiVisRTSPPipeline",
    "SegRTSPPipeline",
    "SegVisRTSPPipeline",
    "SegSahiRTSPPipeline",
    "SegSahiVisRTSPPipeline",
    "PresenceVisVideoPipeline",
    "PresenceSahiVisVideoPipeline",
    "PresenceRTSPPipeline",
    "PresenceVisRTSPPipeline",
    "PresenceSahiRTSPPipeline",
    "PresenceSahiVisRTSPPipeline",
)

RTSP_PIPELINE_TYPES = frozenset(name for name in PIPELINE_TYPES if "RTSP" in name)
VIS_VIDEO_PIPELINE_TYPES = frozenset(name for name in PIPELINE_TYPES if "VisVideo" in name)
IMAGE_PIPELINE_TYPES = frozenset(name for name in PIPELINE_TYPES if "Image" in name)
PRESENCE_PIPELINE_TYPES = frozenset(name for name in PIPELINE_TYPES if name.startswith("Presence"))
SAHI_PIPELINE_TYPES = frozenset(name for name in PIPELINE_TYPES if "Sahi" in name)
PARSER_PIPELINE_TYPES = frozenset(
    name
    for name in PIPELINE_TYPES
    if name not in PRESENCE_PIPELINE_TYPES
    and "Image" not in name
    and "Vis" not in name
)


class TypeRegistry:
    @classmethod
    def list_types(cls):
        items = [{"type": name} for name in PIPELINE_TYPES]
        return {"items": items}

    @classmethod
    def get_schema(cls, pipeline_type):
        if pipeline_type not in PIPELINE_TYPES:
            return None
        streams_available = pipeline_type in RTSP_PIPELINE_TYPES
        presence = pipeline_type in PRESENCE_PIPELINE_TYPES
        vis_video = pipeline_type in VIS_VIDEO_PIPELINE_TYPES
        image = pipeline_type in IMAGE_PIPELINE_TYPES
        file_input = image or vis_video or (
            pipeline_type.endswith("VideoPipeline") and "VisVideo" not in pipeline_type
        )
        file_output = image or vis_video
        debouncer_available = presence
        parser_available = pipeline_type in PARSER_PIPELINE_TYPES
        drawer_available = not parser_available
        tracker_available = not presence
        analyzer_available = not presence
        sahi_available = pipeline_type in SAHI_PIPELINE_TYPES
        logger_interval = 0 if presence else 50
        messager_interval = 0 if presence else 0
        generator_interval = 0 if presence else 50
        schema = {
            "pipeline": {
                "type": pipeline_type,
                "params": {
                    "parser": cls.parser_block(parser_available),
                    "drawer": cls.drawer_block(drawer_available),
                    "logger": cls.logger_block(logger_interval),
                    "messager": cls.messager_block(messager_interval),
                    "debouncer": cls.debouncer_block(debouncer_available),
                },
            },
            "generator": {
                "type": cls.generator_type(pipeline_type),
                "params": {
                    "streams": cls.streams_block(streams_available),
                    "pgie": cls.pgie_block(True),
                    "interval": cls.field(True, generator_interval),
                    "tracker": cls.tracker_block(tracker_available),
                    "analyzer": cls.analyzer_block(analyzer_available),
                    "sahi": cls.sahi_block(sahi_available),
                    "input": cls.io_field(file_input),
                    "output": cls.io_field(file_output),
                },
            },
        }
        return schema

    @classmethod
    def generator_type(cls, pipeline_type):
        mapping = {
            "PresenceVisVideoPipeline": "DetVisVideoPresenceGenerator",
            "PresenceSahiVisVideoPipeline": "DetSahiVisVideoPresenceGenerator",
            "PresenceRTSPPipeline": "DetRTSPPresenceGenerator",
            "PresenceVisRTSPPipeline": "DetVisRTSPPresenceGenerator",
            "PresenceSahiRTSPPipeline": "DetSahiRTSPPresenceGenerator",
            "PresenceSahiVisRTSPPipeline": "DetSahiVisRTSPPresenceGenerator",
        }
        mapped = mapping.get(pipeline_type, pipeline_type.replace("Pipeline", "Generator"))
        return mapped

    @classmethod
    def field(cls, available, default):
        return {"available": available, "default": default}

    @classmethod
    def block(cls, available, params_dict):
        return {"available": available, "params": params_dict}

    @classmethod
    def io_field(cls, available):
        field = {"available": available}
        if available:
            field["default"] = None
        return field

    @classmethod
    def parser_block(cls, available):
        return cls.block(available, {})

    @classmethod
    def drawer_block(cls, available):
        return cls.block(
            available,
            {
                "show_label": cls.field(available, True),
                "show_conf": cls.field(available, True),
                "show_id": cls.field(available, False),
                "interval": cls.field(available, 50),
                "fade_time": cls.field(available, 1),
            },
        )

    @classmethod
    def logger_block(cls, interval):
        return cls.block(
            True,
            {
                "interval": cls.field(True, interval),
            },
        )

    @classmethod
    def messager_block(cls, interval):
        return cls.block(
            True,
            {
                "interval": cls.field(True, interval),
            },
        )

    @classmethod
    def debouncer_block(cls, available):
        return cls.block(
            available,
            {
                "length": cls.field(available, 10),
                "threshold": cls.field(available, 0.5),
                "class_ids": cls.field(available, [0, 1]),
                "mode": cls.field(available, "fold"),
            },
        )

    @classmethod
    def streams_block(cls, available):
        return cls.block(available, {})

    @classmethod
    def pgie_block(cls, available):
        return cls.block(available, {})

    @classmethod
    def tracker_block(cls, available):
        return cls.block(
            available,
            {
                "class_id": cls.field(available, [-1]),
            },
        )

    @classmethod
    def analyzer_block(cls, available):
        return cls.block(
            available,
            {
                "streams": cls.field(available, []),
                "template": cls.field(available, None),
                "roi_filtering": cls.field(available, {"class_id": [-1]}),
                "overcrowding": cls.field(available, None),
                "line_crossing": cls.field(available, None),
                "direction_detection": cls.field(available, None),
            },
        )

    @classmethod
    def sahi_block(cls, available):
        block = {"available": available}
        if available:
            block["params"] = {
                "nvsahipreprocess": cls.field(
                    True,
                    {
                        "slice_width": 640,
                        "slice_height": 640,
                        "overlap_width_ratio": 0.2,
                        "overlap_height_ratio": 0.2,
                    },
                ),
                "nvsahipostprocess": cls.field(
                    True,
                    {
                        "match_threshold": 0.5,
                    },
                ),
            }
        return block
