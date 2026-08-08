from utils.probe.utils.drawer.det_drawer import DetDrawer


class SegDrawer(DetDrawer):
    def apply_mask_color(self, object_meta, mask_color, box_width) -> None:
        self.apply_box_color(object_meta, mask_color, box_width)

    def __call__(
        self,
        batch_meta,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> list:
        results = []
        for frame_meta in batch_meta.frame_items:
            results.append(
                self.process_frame(
                    batch_meta,
                    frame_meta,
                    box_color=mask_color,
                    box_width=box_width,
                    text_color=text_color,
                    text_bg_color=text_bg_color,
                )
            )
        return results
