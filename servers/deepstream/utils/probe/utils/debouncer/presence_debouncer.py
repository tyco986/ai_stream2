from utils.probe.utils.drawer.result import EVENT_CODE_LENGTH


class PresenceDebouncer:
    CODE_BOUNDS = ["0", "1", "2"]

    def __init__(
        self,
        class_ids: list[int],
        threshold: float,
        length: int,
        mode: str = "slide",
        event_names: dict[int, str] | None = None,
        capture_codes: list[str] | None = None,
    ) -> None:
        self.class_ids = class_ids
        self.threshold = threshold
        self.length = length
        self.mode = mode
        self.event_names = {index: "" for index in range(EVENT_CODE_LENGTH)}
        if event_names is not None:
            for key, value in event_names.items():
                index = int(key)
                assert 0 <= index < EVENT_CODE_LENGTH
                self.event_names[index] = str(value)
        self.capture_codes = (
            capture_codes if capture_codes is not None else [self.CODE_BOUNDS[1]]
        )
        assert self.mode in ["slide", "fold"]
        assert all(len(code) == 1 for code in self.capture_codes)
        assert all(code in self.CODE_BOUNDS for code in self.capture_codes)
        self.is_slide = self.mode == "slide"
        self.windows = {cid: [] for cid in self.class_ids}
        self.code_windows = {cid: [] for cid in self.class_ids}
        self.event_codes = {cid: self.CODE_BOUNDS[0] for cid in self.class_ids}
        self.ratios = {cid: 0 for cid in self.class_ids}
        self.capture_id = 0

    def __call__(self, result: dict) -> None:
        detected_ids = {int(item["object"][5]) for item in result["objects"]}
        detected = {cid: cid in detected_ids for cid in self.class_ids}
        absent, alert, transit = self.CODE_BOUNDS

        for cid in self.class_ids:
            window = self.windows[cid]
            code_window = self.code_windows[cid]
            window.append(detected[cid])
            if len(window) >= self.length:
                self.ratios[cid] = window.count(True) / self.length
                if self.ratios[cid] >= self.threshold:
                    self.event_codes[cid] = alert if detected[cid] else transit
                else:
                    self.event_codes[cid] = transit if detected[cid] else absent
                code_window.append(self.event_codes[cid])
                if self.is_slide:
                    window.pop(0)
                    code_window.pop(0)
                else:
                    self.windows[cid] = []
                    self.code_windows[cid] = []
            else:
                self.event_codes[cid] = transit if detected[cid] else absent
                code_window.append(self.event_codes[cid])

        event_codes = "".join(self.event_codes[cid] for cid in self.class_ids)
        event_codes = event_codes.ljust(EVENT_CODE_LENGTH, "0")[:EVENT_CODE_LENGTH]
        active_names = [
            self.event_names[index] if event_codes[index] == alert else ""
            for index in range(EVENT_CODE_LENGTH)
        ]
        capture = any(
            self.event_codes[cid] in self.capture_codes for cid in self.class_ids
        )
        result["event"] = {
            "event_codes": event_codes,
            "event_names": active_names,
            "capture": capture,
            "capture_id": self.capture_id,
            "windows": {
                cid: {
                    i: self.code_windows[cid].count(self.CODE_BOUNDS[i])
                    for i in range(len(self.CODE_BOUNDS))
                }
                for cid in self.class_ids
            },
            "ratios": dict(self.ratios),
        }
        if capture:
            self.capture_id += 1
