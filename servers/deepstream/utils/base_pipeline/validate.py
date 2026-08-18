from pathlib import Path

import yaml

SGIE_YML = "sgie.yml"


def nvinfer_period(ds_interval: int) -> int:
    period = 0
    if ds_interval > 0:
        period = ds_interval + 1
    return period


def validate_probe_interval(
    pgie_interval: int, probe_interval: int, sgie_interval: int = 0
) -> None:
    probe_interval = int(probe_interval)
    stride = pgie_interval
    if sgie_interval > 0:
        stride = sgie_interval
    if stride > 0 and probe_interval > 0:
        assert probe_interval % stride == 0, (
            f"probe interval ({probe_interval}) must be a multiple of "
            f"gie interval ({stride})"
        )


def validate_sgie_interval(pgie_interval: int, sgie_interval: int) -> None:
    sgie_interval = int(sgie_interval)
    if pgie_interval > 0 and sgie_interval > 0:
        assert pgie_interval % sgie_interval == 0, (
            f"pgie interval ({pgie_interval}) must be a multiple of "
            f"sgie interval ({sgie_interval})"
        )


def sgie_period_from_config(config_dir) -> int:
    sgie = yaml.safe_load((Path(config_dir) / SGIE_YML).read_text(encoding="utf-8"))
    period = nvinfer_period(int(sgie["property"].get("interval", 0)))
    return period
