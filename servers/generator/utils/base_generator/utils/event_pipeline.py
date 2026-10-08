class EventPipelineMixin:
    """Always-on presence coder + capturer params for event generators."""

    def bind_event(self, event_coder: dict, capturer: dict | None) -> None:
        self.event_coder = event_coder
        self.capturer = {"capture_codes": "1"}
        if capturer is not None:
            self.capturer.update(capturer)

    def capturer_codes(self) -> str:
        return str(self.capturer.get("capture_codes", "1"))

    def capture_output_dir(self) -> str:
        return f"/root/outputs/deepstream/{self.pipeline_name}"

    def append_event_coder(self, name: str = "nvpresencecoder") -> None:
        coder = self.event_coder
        self._append_node(
            "nvpresencecoder",
            name,
            self._add_nvpresencecoder(
                class_ids=coder.get("class_ids", []),
                event_names=coder.get("event_names", []),
                length=int(coder.get("length", 10)),
                threshold=float(coder.get("threshold", 0.5)),
                mode=coder.get("mode", "fold"),
            ),
        )

    def after_analytics(self, tee_name: str = "tee_msg") -> str:
        return "nvpresencecoder"

    def link_event_coder(self, edges: dict, tee_name: str = "tee_msg") -> None:
        edges["nvpresencecoder"] = tee_name
