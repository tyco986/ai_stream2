def nvinfer_skip(user_interval: int) -> int:
    skip = 0
    if user_interval > 0:
        skip = user_interval - 1
    return skip
