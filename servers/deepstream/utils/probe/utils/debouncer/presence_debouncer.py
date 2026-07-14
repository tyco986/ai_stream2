class PresenceDebouncer:
    def __init__(
        self,
        class_ids: list[int],
        threshold: float,
        length: int,
        mode: str = "slide",
    ) -> None:
        self.class_ids = class_ids
        self.threshold = threshold
        self.length = length
        self.mode = mode
        assert self.mode in ["slide", "fold"]
        self.class_id = set(self.class_ids)
        self.window: list[bool] = []

    def __call__(self, result: dict) -> None:
        event_code = "0"
        detected = any(
            int(item["cls"]) in self.class_id for item in result["objects"]
        )

        self.window.append(detected)
        if len(self.window) >= self.length:
            ratio = self.window.count(True) / self.length
            if self.mode == "slide":
                self.window.pop(0)
            if self.mode == "fold":
                self.window = []

            if ratio >= self.threshold:
                event_code = "1" if detected else "2"
            else:
                event_code = "2" if detected else "0"

        result["event"] = {"event_code": event_code, "window": self.window}
