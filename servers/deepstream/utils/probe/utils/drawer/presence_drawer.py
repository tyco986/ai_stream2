from utils.probe.utils.debouncer.presence_debouncer import PresenceDebouncer
from utils.probe.utils.drawer.det_drawer import DetFadeDrawer


class PresenceFadeDrawer:
    EVENT_BOX_COLORS = {
        "0": (0.0, 1.0, 0.0, 1.0),
        "1": (1.0, 0.0, 0.0, 1.0),
        "2": (1.0, 1.0, 0.0, 1.0),
    }

    def __init__(self, drawer=dict(), debouncer=dict()):
        self.drawer = DetFadeDrawer(**drawer)
        self.debouncer = PresenceDebouncer(**debouncer)

    def hide_object(self, object_meta) -> None:
        object_meta.rect_params.border_width = 0
        object_meta.rect_params.rotation_angle = 0.0
        object_meta.text_params.display_text = b""

    def resolve_box_color(self, event_code) -> tuple[float, float, float, float]:
        box_color = self.EVENT_BOX_COLORS.get(
            str(event_code),
            self.EVENT_BOX_COLORS["0"],
        )
        return box_color

    def collect(self, frame_meta) -> dict:
        pad_index = int(frame_meta.pad_index)
        objects = []
        for object_meta in frame_meta.object_items:
            item = self.drawer.parse_object(object_meta)
            self.hide_object(object_meta)
            objects.append(item)
        if objects:
            self.drawer.object_cache[pad_index] = objects
        else:
            objects = self.drawer.object_cache.get(pad_index, [])
        result = self.drawer.get_result(frame_meta, objects)
        return result

    def rebuild(self, batch_meta, frame_meta, result) -> None:
        pad_index = int(frame_meta.pad_index)
        phase = self.drawer.phase.get(pad_index, 0)
        fade_alpha = self.drawer.alpha_lut[phase]
        box_color = self.resolve_box_color(result["event"]["event_code"])
        faded_box_color = self.drawer.fade_color(box_color, fade_alpha)
        faded_text_color = self.drawer.fade_color((1.0, 1.0, 1.0, 1.0), fade_alpha)
        faded_text_bg_color = self.drawer.fade_color((0.0, 0.0, 0.0, 0.6), fade_alpha)
        for item in result["objects"]:
            self.drawer.append_object(
                batch_meta,
                frame_meta,
                item,
                faded_box_color,
                2,
                faded_text_color,
                faded_text_bg_color,
            )
        frame_count = self.drawer.frame_count.get(pad_index, 0) + 1
        self.drawer.frame_count[pad_index] = frame_count
        self.drawer.phase[pad_index] = frame_count % self.drawer.runtime_interval

    def __call__(self, batch_meta) -> list:
        results = []
        for frame_meta in batch_meta.frame_items:
            result = self.collect(frame_meta)
            self.debouncer(result)
            self.rebuild(batch_meta, frame_meta, result)
            results.append(result)
        return results
