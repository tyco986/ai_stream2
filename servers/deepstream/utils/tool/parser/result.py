import copy

EVENT_CODE_LENGTH = 8

RESULT = {
    "pad_index": 0,
    "frame_number": 0,
    "source_id": 0,
    "source_width": 0,
    "source_height": 0,
    "pipeline_width": 0,
    "pipeline_height": 0,
    "num_objects": 0,
    "objects": [],
    "inference": True,
    "event": {
        "event_codes": "0" * EVENT_CODE_LENGTH,
        "event_names": [],
        "capture": False,
        "capture_id": 0,
        "windows": {},
        "ratios": {},
    },
}


def new_result() -> dict:
    return copy.deepcopy(RESULT)
