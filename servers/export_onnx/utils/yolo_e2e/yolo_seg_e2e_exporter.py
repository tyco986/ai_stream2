from copy import deepcopy
from pathlib import Path

import onnx
import onnxslim
import torch
import torch.nn as nn
from torchvision.ops import roi_align
from ultralytics import YOLO
from ultralytics.nn.modules import Detect

from utils.yolo_e2e.common import set_dim_value, validate_export_args, write_labels


class E2eSegMaskDecode(nn.Module):
    """e2e dets+proto → five outputs (MatMul/Sigmoid + 1ch RoiAlign → bbox-local masks)."""

    def __init__(self, max_det, conf, spatial_scale):
        super().__init__()
        self.max_det = max_det
        self.conf = conf
        self.spatial_scale = spatial_scale

    def forward(self, x):
        dets = x[0]
        protos = x[1]
        boxes = dets[..., :4]
        scores = dets[..., 4]
        classes = dets[..., 5]
        coeffs = dets[..., 6:]

        batch_size = dets.shape[0]
        h_protos = protos.shape[2]
        w_protos = protos.shape[3]
        total_detections = batch_size * self.max_det

        full_masks = torch.matmul(coeffs, protos.flatten(2)).sigmoid()
        masks_nchw = full_masks.reshape(total_detections, 1, h_protos, w_protos)
        batch_index = (
            torch.arange(total_detections, device=dets.device, dtype=boxes.dtype)
            .reshape(total_detections, 1)
        )
        rois = torch.cat([batch_index, boxes.reshape(total_detections, 4)], dim=1)
        pooled = roi_align(
            masks_nchw,
            rois,
            output_size=(h_protos, w_protos),
            spatial_scale=self.spatial_scale,
            sampling_ratio=0,
            aligned=True,
        )
        det_masks = pooled.reshape(batch_size, self.max_det, h_protos * w_protos)
        num_dets = (scores >= self.conf).sum(dim=1, keepdim=True).to(torch.int32)
        result = (
            num_dets,
            boxes,
            scores,
            classes.to(torch.int32),
            det_masks,
        )
        return result


class YoloSegE2EExporter:
    """YOLO26-Seg e2e: top-k + MatMul/Sigmoid/RoiAlign → NvDsInferYoloMask blobs."""

    def prepare_core(self, yolo: YOLO, device: torch.device, max_det: int) -> nn.Module:
        core = deepcopy(yolo.model).to(device)
        for param in core.parameters():
            param.requires_grad = False
        core.eval()
        core.float()
        core = core.fuse()
        for module in core.modules():
            if isinstance(module, Detect):
                module.dynamic = False
                module.export = True
                module.format = "onnx"
                module.max_det = max_det
        return core

    def fix_batch_only_dynamic(self, onnx_path: Path, size: int, max_det: int) -> None:
        model = onnx.load(str(onnx_path))
        inp = model.graph.input[0]
        dims = inp.type.tensor_type.shape.dim
        if len(dims) != 4:
            raise ValueError(f"expected NCHW input, got {len(dims)} dims")
        set_dim_value(dims[1], 3)
        set_dim_value(dims[2], size)
        set_dim_value(dims[3], size)

        outputs = list(model.graph.output)
        if len(outputs) != 5:
            raise ValueError(f"expected 5 outputs, got {len(outputs)}")
        mask_size = size // 4
        mask_flat = mask_size * mask_size
        expected = [
            ("num_dets", [None, 1]),
            ("det_boxes", [None, max_det, 4]),
            ("det_scores", [None, max_det]),
            ("det_classes", [None, max_det]),
            ("det_masks", [None, max_det, mask_flat]),
        ]
        for output, (name, shape) in zip(outputs, expected):
            if output.name != name:
                raise ValueError(f"expected output {name!r}, got {output.name!r}")
            out_dims = output.type.tensor_type.shape.dim
            if len(out_dims) != len(shape):
                raise ValueError(
                    f"{name}: expected rank {len(shape)}, got {len(out_dims)}"
                )
            for dim, value in zip(out_dims, shape):
                if value is not None:
                    set_dim_value(dim, value)
        onnx.save(model, str(onnx_path))

    def export(
        self,
        weights: Path,
        size: int,
        opset: int,
        batch: int,
        dynamic: bool,
        simplify: bool,
        max_det: int,
        conf: float,
        output_dir: Path,
    ) -> None:
        validate_export_args(weights, dynamic, batch)

        yolo = YOLO(str(weights))
        write_labels(yolo.names, output_dir / "labels.txt")

        device = torch.device("cpu")
        core = self.prepare_core(yolo, device, max_det)
        spatial_scale = 0.25
        model = nn.Sequential(
            core,
            E2eSegMaskDecode(max_det, conf, spatial_scale),
        )
        onnx_path = output_dir / f"{weights.stem}.onnx"
        onnx_input = torch.zeros(batch, 3, size, size, device=device)
        output_names = ["num_dets", "det_boxes", "det_scores", "det_classes", "det_masks"]
        dynamic_axes = None
        if dynamic:
            dynamic_axes = {name: {0: "batch"} for name in ["images", *output_names]}

        torch.onnx.export(
            model,
            onnx_input,
            str(onnx_path),
            verbose=False,
            opset_version=opset,
            do_constant_folding=True,
            input_names=["images"],
            output_names=output_names,
            dynamic_axes=dynamic_axes,
            dynamo=False,
        )
        if simplify:
            onnx.save(onnxslim.slim(onnx.load(str(onnx_path))), str(onnx_path))
        if dynamic:
            self.fix_batch_only_dynamic(onnx_path, size, max_det)
