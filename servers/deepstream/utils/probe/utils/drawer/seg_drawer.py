from pyservicemaker import osd

from utils.probe.utils.drawer.det_drawer import DetDrawer, DetFadeDrawer


class SegDrawer(DetDrawer):
    def append_object(
        self,
        batch_meta,
        frame_meta,
        item,
        box_color,
        box_width,
        text_color,
        text_bg_color,
        label,
        mask_color,
    ) -> None:
        obj_meta = batch_meta.acquire_object_meta()
        self.apply_rect_params(obj_meta.rect_params, item["rect_params"], box_color, box_width)
        mask_data = item["mask_params"]
        if mask_data is not None:
            r, g, b, a = mask_color
            mask = obj_meta.mask_params
            mask.width = mask_data["mask_width"]
            mask.height = mask_data["mask_height"]
            mask.size = mask_data["mask_size"]
            mask.threshold = mask_data["mask_threshold"]
            array = mask.alloc_mask_array()
            array.ravel()[: len(mask_data["mask_data"])] = mask_data["mask_data"]
            mask.color = osd.Color(float(r), float(g), float(b), float(a))
        self.apply_label(obj_meta, item, text_color, text_bg_color, label)
        frame_meta.append(obj_meta)

    def __call__(
        self,
        batch_meta,
        results,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
        label="",
    ) -> None:
        frame_meta_by_pad = {
            int(frame_meta.pad_index): frame_meta
            for frame_meta in batch_meta.frame_items
        }
        for frame_result in results:
            frame_meta = frame_meta_by_pad[int(frame_result["pad_index"])]
            for item in frame_result["objects"]:
                self.append_object(
                    batch_meta,
                    frame_meta,
                    item,
                    box_color,
                    box_width,
                    text_color,
                    text_bg_color,
                    label,
                    mask_color,
                )


class SegFadeDrawer(SegDrawer, DetFadeDrawer):
    def __call__(
        self,
        batch_meta,
        results,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
        label="",
    ) -> None:
        frame_meta_by_pad = {
            int(frame_meta.pad_index): frame_meta
            for frame_meta in batch_meta.frame_items
        }
        for frame_result in results:
            pad_index = int(frame_result["pad_index"])
            count = self.frame_counts.get(pad_index, 0) + 1
            self.frame_counts[pad_index] = count
            phase = (count - 1) % self.runtime_interval
            if phase == 0:
                self.caches[pad_index] = frame_result["objects"]
            cache = self.caches.get(pad_index, [])
            fade_alpha = self.alpha_lut[phase]
            faded_box_color = self.fade_color(box_color, fade_alpha)
            faded_mask_color = self.fade_color(mask_color, fade_alpha)
            faded_text_color = self.fade_color(text_color, fade_alpha)
            faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
            frame_meta = frame_meta_by_pad[pad_index]
            for item in cache:
                self.append_object(
                    batch_meta,
                    frame_meta,
                    item,
                    faded_box_color,
                    box_width,
                    faded_text_color,
                    faded_text_bg_color,
                    label,
                    faded_mask_color,
                )
