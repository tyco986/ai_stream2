from utils.probe.utils.drawer.det_fade_drawer import DetFadeDrawer
from utils.probe.utils.drawer.seg_drawer import SegDrawer


class SegFadeDrawer(DetFadeDrawer, SegDrawer):
    def __call__(
        self,
        batch_meta,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> list:
        results = DetFadeDrawer.__call__(
            self,
            batch_meta,
            box_color=mask_color,
            box_width=box_width,
            text_color=text_color,
            text_bg_color=text_bg_color,
        )
        return results
