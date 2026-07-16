import numpy as np

from utils.probe.utils.drawer.det_drawer import DetDrawer, DetFadeDrawer


class SegDrawer(DetDrawer):
    def parse_mask_shape(self, bitmap) -> tuple[int, int]:
        shape = bitmap.shape
        if len(shape) == 2:
            mask_height, mask_width = int(shape[0]), int(shape[1])
        else:
            flat_size = int(bitmap.size)
            side = int(flat_size**0.5)
            mask_width = side if side * side == flat_size else flat_size
            mask_height = flat_size // mask_width if mask_width else 0
        return mask_width, mask_height

    def parse_mask_params(self, mask) -> dict | None:
        bitmap = np.asarray(mask.mask_array, dtype=np.float32).copy()
        mask_params = None
        if bitmap.size > 0:
            mask_width, mask_height = self.parse_mask_shape(bitmap)
            mask_params = {
                "mask_data": bitmap,
                "mask_width": mask_width,
                "mask_height": mask_height,
                "mask_size": int(bitmap.size),
                "mask_threshold": float(mask.threshold),
            }
        return mask_params

    def parse_object(self, object_meta) -> dict:
        result = super().parse_object(object_meta)
        result["mask_params"] = self.parse_mask_params(object_meta.mask_params)
        return result

    def apply_mask_color(self, object_meta, mask_color, box_width) -> None:
        self.apply_box_color(object_meta, mask_color, box_width)

    def __call__(
        self,
        batch_meta,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        frame_meta = next(iter(batch_meta.frame_items))
        objects = []
        for object_meta in frame_meta.object_items:
            item = self.parse_object(object_meta)
            self.draw_inplace(
                object_meta,
                item,
                mask_color,
                box_width,
                text_color,
                text_bg_color,
            )
            objects.append(item)
        result = self.get_result(frame_meta, objects)
        return result


class SegFadeDrawer(DetFadeDrawer, SegDrawer):
    def __call__(
        self,
        batch_meta,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        result = DetFadeDrawer.__call__(
            self,
            batch_meta,
            box_color=mask_color,
            box_width=box_width,
            text_color=text_color,
            text_bg_color=text_bg_color,
        )
        return result
