class RectExpander:
    originals = {}
    max_size = 4096

    def __init__(self, infer_height=256, infer_width=192, padding=1.25):
        self.infer_height = int(infer_height)
        self.infer_width = int(infer_width)
        self.padding = float(padding)

    @classmethod
    def save(cls, source_id, frame_number, object_index, left, top, width, height) -> None:
        key = (int(source_id), int(frame_number), int(object_index))
        cls.originals[key] = (float(left), float(top), float(width), float(height))
        if len(cls.originals) > cls.max_size:
            for old in sorted(cls.originals)[: len(cls.originals) - cls.max_size // 2]:
                cls.originals.pop(old, None)

    @classmethod
    def take(cls, source_id, frame_number, object_index) -> tuple | None:
        key = (int(source_id), int(frame_number), int(object_index))
        original = cls.originals.pop(key, None)
        return original

    @classmethod
    def restore(cls, rect, source_id, frame_number, object_index) -> None:
        original = cls.take(source_id, frame_number, object_index)
        if original is not None:
            rect.left = original[0]
            rect.top = original[1]
            rect.width = original[2]
            rect.height = original[3]

    def expand_rect(self, rect, frame_width, frame_height) -> None:
        aspect = self.infer_width / float(self.infer_height)
        center_x = float(rect.left) + float(rect.width) * 0.5
        center_y = float(rect.top) + float(rect.height) * 0.5
        scale_w = float(rect.width) * self.padding
        scale_h = float(rect.height) * self.padding
        if scale_w > aspect * scale_h:
            scale_h = scale_w / aspect
        else:
            scale_w = scale_h * aspect
        left = max(0.0, center_x - scale_w * 0.5)
        top = max(0.0, center_y - scale_h * 0.5)
        right = min(float(frame_width), center_x + scale_w * 0.5)
        bottom = min(float(frame_height), center_y + scale_h * 0.5)
        rect.left = left
        rect.top = top
        rect.width = max(2.0, right - left)
        rect.height = max(2.0, bottom - top)

    def process_frame(self, frame_meta) -> None:
        frame_width = int(frame_meta.pipeline_width)
        frame_height = int(frame_meta.pipeline_height)
        source_id = int(frame_meta.source_id)
        frame_number = int(frame_meta.frame_number)
        for object_index, object_meta in enumerate(frame_meta.object_items):
            rect = object_meta.rect_params
            self.save(
                source_id,
                frame_number,
                object_index,
                rect.left,
                rect.top,
                rect.width,
                rect.height,
            )
            self.expand_rect(rect, frame_width, frame_height)

    def __call__(self, batch_meta) -> None:
        for frame_meta in batch_meta.frame_items:
            self.process_frame(frame_meta)
