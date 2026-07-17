from threading import Lock


class PresenceCaptureState:
    lock = Lock()
    codes = {}
    max_size = 128

    @classmethod
    def update(cls, pad_index, frame_number, event_code) -> None:
        key = (int(pad_index), int(frame_number))
        with cls.lock:
            cls.codes[key] = str(event_code)
            if len(cls.codes) > cls.max_size:
                for old in sorted(cls.codes)[: len(cls.codes) - cls.max_size // 2]:
                    cls.codes.pop(old, None)

    @classmethod
    def should_capture(cls, pad_index, frame_number) -> bool:
        key = (int(pad_index), int(frame_number))
        with cls.lock:
            code = cls.codes.get(key, "")
        return "1" in code
