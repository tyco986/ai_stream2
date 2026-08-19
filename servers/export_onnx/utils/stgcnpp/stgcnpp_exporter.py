from pathlib import Path

import onnx
import onnxslim
import torch

from utils.stgcnpp.model import NTU60_LABELS, StgcnppRecognizer
from utils.yolo_e2e.common import validate_export_args


class StgcnppExporter:
    num_person = 2
    clip_len = 100
    num_joints = 17
    in_channels = 3

    def export(
        self,
        weights: Path,
        size: int,
        opset: int,
        batch: int | None,
        dynamic: bool,
        simplify: bool,
        max_det: int,
        conf: float,
        output_dir: Path,
    ) -> None:
        dummy_batch = 1 if batch is None else batch
        validate_export_args(weights, dynamic, dummy_batch)
        model = self.build_model(weights)
        labels_path = output_dir / "labels.txt"
        labels_path.write_text("\n".join(NTU60_LABELS) + "\n", encoding="utf-8")

        dummy = torch.zeros(
            dummy_batch,
            self.num_person,
            self.clip_len,
            self.num_joints,
            self.in_channels,
        )
        onnx_path = output_dir / f"{weights.stem}.onnx"
        dynamic_axes = None
        if dynamic:
            dynamic_axes = {"input": {0: "batch"}, "output": {0: "batch"}}
        torch.onnx.export(
            model,
            dummy,
            str(onnx_path),
            verbose=False,
            opset_version=opset,
            do_constant_folding=True,
            input_names=["input"],
            output_names=["output"],
            dynamic_axes=dynamic_axes,
            dynamo=False,
        )
        if simplify:
            onnx.save(onnxslim.slim(onnx.load(str(onnx_path))), str(onnx_path))

    def build_model(self, weights: Path) -> StgcnppRecognizer:
        model = StgcnppRecognizer(num_classes=len(NTU60_LABELS))
        state = torch.load(str(weights), map_location="cpu", weights_only=False)
        if isinstance(state, dict) and "state_dict" in state:
            state = state["state_dict"]
        model.load_state_dict(state, strict=True)
        model.eval()
        return model
