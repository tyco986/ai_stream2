import os
import textwrap

import structlog
from django.conf import settings

logger = structlog.get_logger(__name__)

# Default shared volume path where DeepStream reads config files
_DEPLOY_DIR = os.environ.get("DS_CONFIG_DEPLOY_DIR", "/shared/deepstream-config")


class PipelineDeployer:
    """Generates DeepStream config files from PipelineProfile + AnalyticsZones
    and writes them to the shared volume.
    """

    def __init__(self, deploy_dir=None):
        self.deploy_dir = deploy_dir or _DEPLOY_DIR

    def deploy(self, pipeline_profile):
        """Generate config files and mark profile as deployed."""
        os.makedirs(self.deploy_dir, exist_ok=True)

        self._generate_pgie_config(pipeline_profile)
        if pipeline_profile.tracker:
            self._generate_tracker_config(pipeline_profile)
        if pipeline_profile.analytics_enabled:
            self._generate_analytics_config(pipeline_profile)

        pipeline_profile.analytics_config_stale = False
        pipeline_profile.save(update_fields=["analytics_config_stale", "updated_at"])

        logger.info("Pipeline deployed",
                     profile=pipeline_profile.name,
                     deploy_dir=self.deploy_dir)

    def _generate_pgie_config(self, profile):
        detector = profile.detector
        config = detector.config or {}
        content = {
            "property": {
                "gpu-id": 0,
                "net-scale-factor": config.get("net_scale_factor", 0.00392),
                "model-file": detector.model_file,
                "num-detected-classes": config.get("num_classes", 80),
                "cluster-mode": config.get("cluster_mode", 2),
                "network-mode": config.get("network_mode", 1),
                "batch-size": int(os.environ.get("DS_MAX_BATCH_SIZE", "16")),
            },
        }
        if detector.label_file:
            content["property"]["labelfile-path"] = detector.label_file

        lines = ["[property]"]
        for key, val in content["property"].items():
            lines.append(f"{key}={val}")

        path = os.path.join(self.deploy_dir, "pgie_config.txt")
        with open(path, "w") as f:
            f.write("\n".join(lines) + "\n")

    def _generate_tracker_config(self, profile):
        tracker = profile.tracker
        config = tracker.config or {}
        tracker_type = config.get("tracker_type", "NvDCF_perf")

        path = os.path.join(self.deploy_dir, "tracker_config.yml")
        with open(path, "w") as f:
            f.write(textwrap.dedent(f"""\
                %YAML:1.0
                BaseConfig:
                  minDetectorConfidence: 0.2
                  trackerType: {tracker_type}
            """))

    def _generate_analytics_config(self, profile):
        """Generate nvdsanalytics config from all bound cameras' AnalyticsZones.

        stream-id is assigned by sorting Camera.uid to ensure stable ordering.
        """
        from apps.cameras.models import Camera  # noqa: E402 — 解决循环依赖
        from apps.pipelines.models import CameraModelBinding

        bindings = CameraModelBinding.objects.filter(
            pipeline_profile=profile,
            is_enabled=True,
        ).select_related("camera")

        cameras = sorted(
            [b.camera for b in bindings],
            key=lambda c: c.uid,
        )

        config_width = getattr(settings, "ANALYTICS_WIDTH", 1920)
        config_height = getattr(settings, "ANALYTICS_HEIGHT", 1080)
        lines = ["[property]", "enable=1", f"config-width={config_width}",
                 f"config-height={config_height}",
                 "osd-mode=2", "display-font-size=12", ""]

        for stream_idx, camera in enumerate(cameras):
            zones = camera.analytics_zones.filter(is_enabled=True)
            roi_count = 0
            lc_count = 0
            oc_count = 0

            for zone in zones:
                coords = zone.coordinates
                coord_str = ";".join(f"{p[0]};{p[1]}" for p in coords)

                if zone.zone_type == "roi":
                    lines.append(f"[roi-filtering-stream-{stream_idx}]")
                    lines.append(f"enable=1")
                    lines.append(f"roi-{zone.name}={coord_str}")
                    class_id = zone.config.get("class_id", -1)
                    inverse = int(zone.config.get("inverse", False))
                    lines.append(f"class-id={class_id}")
                    lines.append(f"inverse-roi={inverse}")
                    lines.append("")
                    roi_count += 1

                elif zone.zone_type == "line_crossing":
                    lines.append(f"[line-crossing-stream-{stream_idx}]")
                    lines.append(f"enable=1")
                    lines.append(f"line-crossing-{zone.name}={coord_str}")
                    class_id = zone.config.get("class_id", 0)
                    extended = int(zone.config.get("extended", False))
                    lines.append(f"class-id={class_id}")
                    lines.append(f"extended={extended}")
                    lines.append(f"mode=balanced")
                    lines.append("")
                    lc_count += 1

                elif zone.zone_type == "overcrowding":
                    lines.append(f"[overcrowding-stream-{stream_idx}]")
                    lines.append(f"enable=1")
                    lines.append(f"roi-{zone.name}={coord_str}")
                    threshold = zone.config.get("object_threshold", 5)
                    lines.append(f"object-threshold={threshold}")
                    lines.append("")
                    oc_count += 1

                elif zone.zone_type == "direction":
                    lines.append(f"[direction-detection-stream-{stream_idx}]")
                    lines.append(f"enable=1")
                    lines.append(f"direction-{zone.name}={coord_str}")
                    direction_name = zone.config.get("direction_name", "")
                    lines.append(f"direction-name={direction_name}")
                    lines.append("")

        path = os.path.join(self.deploy_dir, "analytics_config.txt")
        with open(path, "w") as f:
            f.write("\n".join(lines) + "\n")

        logger.info("Analytics config generated",
                     cameras=len(cameras),
                     path=path)
