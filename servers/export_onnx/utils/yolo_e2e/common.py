from pathlib import Path

import hashlib
import json

import onnx


def write_labels(names, labels_path: Path) -> None:
    lines = [str(name) for name in names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def validate_export_args(weights: Path, dynamic: bool, batch: int) -> None:
    if dynamic and batch > 1:
        raise ValueError("dynamic batch and static batch > 1 are incompatible")
    if not weights.is_file():
        raise ValueError(f"invalid weights file: {weights}")


def set_dim_value(dim, value: int) -> None:
    dim.ClearField("dim_param")
    dim.dim_value = value


def onnx_shape(value_info) -> list:
    dims = []
    for dim in value_info.type.tensor_type.shape.dim:
        if dim.dim_param or not dim.dim_value:
            dims.append(None)
        else:
            dims.append(int(dim.dim_value))
    return dims


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_bundle_meta(
    output_dir: Path,
    onnx_path: Path,
    labels_path: Path,
    version: str,
    task: str,
    yolo_export: str,
    max_det: int,
    conf: float,
    dynamic: bool,
    batch: int,
) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    out0 = model.graph.output[0]
    classes = [
        line.strip()
        for line in labels_path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    bundle_files = {
        path.name: file_sha256(path)
        for path in sorted(output_dir.iterdir())
        if path.is_file() and path.name != "meta.json"
    }
    hasher = hashlib.sha256()
    for name in sorted(bundle_files):
        hasher.update(f"{name}:{bundle_files[name]}\n".encode("utf-8"))
    meta = {
        "batch_mode": "dynamic" if dynamic else "static",
        "input_tensor_name": inp.name,
        "output_tensor_name": out0.name,
        "classes": classes,
        "input_tensor_shape": onnx_shape(inp),
        "output_tensor_shape": onnx_shape(out0),
        "batch_size": None if dynamic else batch,
        "precision": "fp32",
        "version": version,
        "task": task,
        "yolo_export": yolo_export,
        "max_det": max_det,
        "conf": conf,
        "bundle_sha256": hasher.hexdigest(),
        "bundle_files": bundle_files,
    }
    (output_dir / "meta.json").write_text(
        json.dumps(meta, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def fix_batch_only_dynamic(onnx_path: Path, size: int, max_det: int) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    dims = inp.type.tensor_type.shape.dim
    if len(dims) != 4:
        raise ValueError(f"expected NCHW input, got {len(dims)} dims")
    set_dim_value(dims[1], 3)
    set_dim_value(dims[2], size)
    set_dim_value(dims[3], size)

    outputs = list(model.graph.output)
    if not outputs:
        raise ValueError("onnx missing outputs")

    out0_dims = outputs[0].type.tensor_type.shape.dim
    if len(out0_dims) != 3:
        raise ValueError(f"expected output0 rank 3, got {len(out0_dims)}")
    set_dim_value(out0_dims[1], max_det)

    if len(outputs) == 1:
        set_dim_value(out0_dims[2], 6)
    else:
        out1_dims = outputs[1].type.tensor_type.shape.dim
        if len(out1_dims) != 4:
            raise ValueError(f"expected output1 rank 4, got {len(out1_dims)}")
        mask_channels = int(out1_dims[1].dim_value) if out1_dims[1].dim_value else 32
        set_dim_value(out1_dims[1], mask_channels)
        mask_size = size // 4
        set_dim_value(out1_dims[2], mask_size)
        set_dim_value(out1_dims[3], mask_size)
        if out0_dims[2].dim_param or not out0_dims[2].dim_value:
            set_dim_value(out0_dims[2], 6 + mask_channels)

    onnx.save(model, str(onnx_path))


def pose_output_channels(num_keypoints: int) -> int:
    return 6 + 3 * num_keypoints


def fix_pose_batch_only_dynamic(
    onnx_path: Path, size: int, max_det: int, channels: int
) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    dims = inp.type.tensor_type.shape.dim
    if len(dims) != 4:
        raise ValueError(f"expected NCHW input, got {len(dims)} dims")
    set_dim_value(dims[1], 3)
    set_dim_value(dims[2], size)
    set_dim_value(dims[3], size)

    outputs = list(model.graph.output)
    if len(outputs) != 1:
        raise ValueError(f"expected 1 pose output, got {len(outputs)}")
    out0_dims = outputs[0].type.tensor_type.shape.dim
    if len(out0_dims) != 3:
        raise ValueError(f"expected output0 rank 3, got {len(out0_dims)}")
    set_dim_value(out0_dims[1], max_det)
    set_dim_value(out0_dims[2], channels)
    onnx.save(model, str(onnx_path))



def write_labels(names, labels_path: Path) -> None:
    lines = [str(name) for name in names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def validate_export_args(weights: Path, dynamic: bool, batch: int) -> None:
    if dynamic and batch > 1:
        raise ValueError("dynamic batch and static batch > 1 are incompatible")
    if not weights.is_file():
        raise ValueError(f"invalid weights file: {weights}")


def set_dim_value(dim, value: int) -> None:
    dim.ClearField("dim_param")
    dim.dim_value = value


def fix_batch_only_dynamic(onnx_path: Path, size: int, max_det: int) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    dims = inp.type.tensor_type.shape.dim
    if len(dims) != 4:
        raise ValueError(f"expected NCHW input, got {len(dims)} dims")
    set_dim_value(dims[1], 3)
    set_dim_value(dims[2], size)
    set_dim_value(dims[3], size)

    outputs = list(model.graph.output)
    if not outputs:
        raise ValueError("onnx missing outputs")

    out0_dims = outputs[0].type.tensor_type.shape.dim
    if len(out0_dims) != 3:
        raise ValueError(f"expected output0 rank 3, got {len(out0_dims)}")
    set_dim_value(out0_dims[1], max_det)

    if len(outputs) == 1:
        set_dim_value(out0_dims[2], 6)
    else:
        out1_dims = outputs[1].type.tensor_type.shape.dim
        if len(out1_dims) != 4:
            raise ValueError(f"expected output1 rank 4, got {len(out1_dims)}")
        mask_channels = int(out1_dims[1].dim_value) if out1_dims[1].dim_value else 32
        set_dim_value(out1_dims[1], mask_channels)
        mask_size = size // 4
        set_dim_value(out1_dims[2], mask_size)
        set_dim_value(out1_dims[3], mask_size)
        if out0_dims[2].dim_param or not out0_dims[2].dim_value:
            set_dim_value(out0_dims[2], 6 + mask_channels)

    onnx.save(model, str(onnx_path))
