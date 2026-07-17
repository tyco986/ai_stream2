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
        self.windows = {}
        self.code_windows = {}

    def __call__(self, result: dict) -> None:
        pad_index = int(result["pad_index"])
        window = self.windows.setdefault(pad_index, [])
        code_window = self.code_windows.setdefault(pad_index, [])
        detected = any(int(item["object"][5]) in self.class_id for item in result["objects"])
        event_code = "0"
        ratio = 0

        window.append(detected)
        code_window.append(event_code)

        if len(window) >= self.length:
            ratio = window.count(True) / self.length
            if ratio >= self.threshold:
                event_code = "1" if detected else "2"
            else:
                event_code = "2" if detected else "0"
            code_window[-1] = event_code
            if self.mode == "slide":
                window.pop(0)
                code_window.pop(0)
            if self.mode == "fold":
                self.windows[pad_index] = []
                self.code_windows[pad_index] = []
                window = self.windows[pad_index]
                code_window = self.code_windows[pad_index]
        else:
            event_code = "2" if detected else "0"
            code_window[-1] = event_code

        result["event"] = {
            "event_code": event_code,
            "window": {
                "0": code_window.count("0"),
                "1": code_window.count("1"),
                "2": code_window.count("2"),
            },
            "ratio": ratio,
        }
