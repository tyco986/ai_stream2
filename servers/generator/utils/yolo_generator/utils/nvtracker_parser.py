def validate_tracker(tracker: dict | None, pgie_class_ids: set[int]) -> bool:
    if tracker is None:
        return False
    assert set(tracker) == {"class_id"}, "tracker dict must only contain class_id"
    class_id = tracker["class_id"]
    class_ids = class_id if isinstance(class_id, list) else [class_id]
    assert class_ids, "tracker class_id cannot be empty"
    invalid = sorted(set(class_ids) - pgie_class_ids)
    assert not invalid, (
        f"tracker class_id {invalid} not in pgie classes {sorted(pgie_class_ids)}"
    )
    return True
