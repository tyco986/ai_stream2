def nvinfer_skip(user_interval: int) -> int:
    assert user_interval >= 1, f"interval must be >= 1, got {user_interval}"
    skip = user_interval - 1
    return skip
