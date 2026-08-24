from pathlib import Path

from utils.yolo.utils.constants import LIBS_ROOT, YOLO_SEG_PLUGIN_NAME
from utils.yolo.utils.onnx_bundle import OnnxBundle
from utils.yolo.yolo_det_exporter import YoloDetExporter


class YoloSegExporter(YoloDetExporter):
    def plugin_path(self) -> Path | None:
        plugin_path = LIBS_ROOT / YOLO_SEG_PLUGIN_NAME
        if not plugin_path.is_file():
            raise ValueError(f"missing static plugin: {plugin_path}")
        return plugin_path

    def build_trtexec_command(
        self,
        bundle: OnnxBundle,
        engine_path: Path,
        batch_size: int,
        gpu_id: int,
        precision: str,
        opt_level: int | None = None,
    ) -> list[str]:
        command = super().build_trtexec_command(
            bundle, engine_path, batch_size, gpu_id, precision, opt_level
        )
        command.append(f"--staticPlugins={self.plugin_path()}")
        return command
