class BaseSahiPose:
    SAHI_POSTPROCESS = "nvsahipostprocess_pose"

    def osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
        }

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return self.osd_kwargs(gpu_id)

    def sahi_postprocess_properties(self, postprocess: dict) -> dict:
        return self._add_nvsahipostprocess_pose(
            gie_ids=str(self.pgie_generator.config["property"]["gie-unique-id"]),
            class_agnostic=False,
            oks_threshold=postprocess.get("oks_threshold", 0.5),
            vis_threshold=postprocess.get("vis_threshold", 0.0),
            num_keypoints=postprocess.get("num_keypoints", 0),
        )
