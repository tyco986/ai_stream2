class NvdsanalyticsParser:
    RULE_NAMES = (
        "roi_filtering",
        "overcrowding",
        "line_crossing",
        "direction_detection",
    )

    def __init__(self, streams: list[str], template: dict) -> None:
        self.streams = streams
        self.template = template

    def validate(self, pipeline_stream_names: list[str], pgie_class_ids: set[int]) -> None:
        stream_name_set = set(pipeline_stream_names)
        for stream_name in self.streams:
            assert stream_name in stream_name_set, (
                f"analyzer stream not in pipeline streams: {stream_name!r}"
            )
        for rule_name in self.RULE_NAMES:
            rule = self.template.get(rule_name)
            if rule is None:
                continue
            for class_id in rule.get("class_id", []):
                if class_id == -1:
                    continue
                assert class_id in pgie_class_ids, (
                    f"analyzer {rule_name} class_id {class_id} not in pgie classes"
                )

    def format_class_id(self, class_ids: list) -> int | str:
        if len(class_ids) == 1:
            return class_ids[0]
        return ";".join(str(class_id) for class_id in class_ids)

    def iter_roi_entries(self, roi: list) -> list[tuple[str, str]]:
        if len(roi) == 2 and isinstance(roi[0], str) and isinstance(roi[1], str):
            return [(roi[0], roi[1])]
        return [(entry[0], entry[1]) for entry in roi]

    def build(
        self,
        pipeline_stream_names: list[str],
        config_width: int,
        config_height: int,
    ) -> dict:
        config = {
            "property": {
                "enable": 1,
                "config-width": config_width,
                "config-height": config_height,
                "osd-mode": 0,
                "display-font-size": 12,
            },
        }
        stream_indices = {
            stream_name: index
            for index, stream_name in enumerate(pipeline_stream_names)
        }
        for stream_name in self.streams:
            index = stream_indices[stream_name]
            roi_filtering = self.template.get("roi_filtering")
            if roi_filtering is not None:
                section_key = f"roi-filtering-stream-{index}"
                section = {
                    "enable": 1,
                    "class-id": self.format_class_id(roi_filtering["class_id"]),
                    "inverse-roi": roi_filtering["inverse_roi"],
                }
                for name, coords in self.iter_roi_entries(roi_filtering["roi"]):
                    section[f"roi-{name}"] = coords
                config[section_key] = section
            overcrowding = self.template.get("overcrowding")
            if overcrowding is not None:
                section_key = f"overcrowding-stream-{index}"
                section = {
                    "enable": 1,
                    "class-id": self.format_class_id(overcrowding["class_id"]),
                    "object-threshold": overcrowding["object_threshold"],
                    "time-threshold": overcrowding["time_threshold"],
                }
                for name, coords in self.iter_roi_entries(overcrowding["roi"]):
                    section[f"roi-{name}"] = coords
                config[section_key] = section
            line_crossing = self.template.get("line_crossing")
            if line_crossing is not None:
                section_key = f"line-crossing-stream-{index}"
                section = {
                    "enable": 1,
                    "extended": line_crossing["extended"],
                    "class-id": self.format_class_id(line_crossing["class_id"]),
                    "mode": line_crossing["mode"],
                }
                for name, coords in self.iter_roi_entries(line_crossing["roi"]):
                    section[f"line-crossing-{name}"] = coords
                config[section_key] = section
            direction_detection = self.template.get("direction_detection")
            if direction_detection is not None:
                section_key = f"direction-detection-stream-{index}"
                section = {
                    "enable": 1,
                    "class-id": self.format_class_id(direction_detection["class_id"]),
                    "mode": direction_detection["mode"],
                }
                for name, coords in self.iter_roi_entries(direction_detection["roi"]):
                    section[f"direction-{name}"] = coords
                config[section_key] = section
        return config
