def validate_probe_interval(pgie_interval: int, probe_interval: int) -> None:
    probe_interval = int(probe_interval)
    if pgie_interval > 0 and probe_interval > 0:
        assert probe_interval % pgie_interval == 0, (
            f"probe interval ({probe_interval}) must be a multiple of "
            f"pgie interval ({pgie_interval})"
        )
