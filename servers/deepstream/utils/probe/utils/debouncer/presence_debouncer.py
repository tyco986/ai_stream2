class PresenceDebouncer():
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

        self.windows: dict[int, list[bool]] = {}

    def __call__(self, results) -> dict:
        presence = self.get_presence(results)
        for pad_index, detected in presence.items():
            window = self.windows.setdefault(pad_index, [])
            window.append(detected)
            if len(window) >= self.length:
                ratio = window.count(1) / self.length
                if self.mode == "slide":
                    window.pop(0)
                if self.mode == "fold":
                    self.windows[pad_index] = []

                if ratio >= self.threshold:
                    presence[pad_index] = 1 if detected else 2
                else:
                    presence[pad_index] = 2 if detected else 0
        return presence

    def get_presence(self, results) -> dict:
        tickets = []
        for frame_result in results:
            pad_index = int(frame_result["pad_index"])
            window = self.windows.setdefault(pad_index, [])
            window.append(detected)

            detected = any(
                int(item["cls"]) in self.class_id
                for item in frame_result["objects"]
            )
            tickets.append({
                "pad_index": pad_index, 
                "source_id": int(frame_result["source_id"]),
                "frame_number": int(frame_result["frame_number"]),
                "event_code": 1 if detected else 0,
            })
        return tickets
