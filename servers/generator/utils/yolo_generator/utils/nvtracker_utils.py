def align_tracker_dimension(value: int) -> int:
    # ponytail: NvDCF wants multiples of 32; upgrade path is per-model tracker config
    return (value // 32) * 32


def validate_tracker(tracker: dict | None, pgie_class_ids: set[int]) -> bool:
    if tracker is None:
        return False
    assert set(tracker) == {"class_id"}, "tracker dict must only contain class_id"
    class_id = tracker["class_id"]
    class_ids = class_id if isinstance(class_id, list) else [class_id]
    assert class_ids, "tracker class_id cannot be empty"
    if class_ids == [-1]:
        return True
    invalid = sorted(set(class_ids) - pgie_class_ids)
    assert not invalid, (
        f"tracker class_id {invalid} not in pgie classes {sorted(pgie_class_ids)}"
    )
    return True


def format_operate_on_class_ids(tracker: dict) -> str:
    class_id = tracker["class_id"]
    class_ids = class_id if isinstance(class_id, list) else [class_id]
    result = ""
    if class_ids != [-1]:
        result = ";".join(str(item) for item in class_ids)
    return result
