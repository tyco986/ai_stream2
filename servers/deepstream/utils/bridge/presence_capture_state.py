from threading import Lock


class PresenceCaptureState:
    lock = Lock()
    results = {}
    max_size = 128

    @classmethod
    def update(cls, result) -> None:
        key = (int(result["pad_index"]), int(result["frame_number"]))
        with cls.lock:
            cls.results[key] = result
            if len(cls.results) > cls.max_size:
                for old in sorted(cls.results)[: len(cls.results) - cls.max_size // 2]:
                    cls.results.pop(old, None)

    @classmethod
    def get(cls, pad_index, frame_number) -> dict | None:
        key = (int(pad_index), int(frame_number))
        with cls.lock:
            result = cls.results.get(key)
        return result
