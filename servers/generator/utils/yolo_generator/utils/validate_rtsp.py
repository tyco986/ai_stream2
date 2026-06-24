import av

RTSP_PROBE_OPTIONS = {"rtsp_transport": "tcp", "stimeout": "5000000"}


def validate_rtsp(
    urls: list[str],
    validate_unified_resolution: bool = True,
) -> dict[str, dict[str, int]]:
    stream_info: dict[str, dict[str, int]] = {}
    unavailable: list[tuple[str, str]] = []

    for url in urls:
        try:
            with av.open(url, options=RTSP_PROBE_OPTIONS) as container:
                stream = container.streams.video[0]
                rate = stream.average_rate or stream.codec_context.framerate
                if rate is None:
                    raise RuntimeError("fps not available from stream metadata")
                fps = round(float(rate))
                if fps <= 0:
                    raise RuntimeError(f"invalid fps: {fps}")
                for frame in container.decode(stream):
                    stream_info[url] = {
                        "width": frame.width,
                        "height": frame.height,
                        "fps": fps,
                    }
                    break
                else:
                    raise RuntimeError(f"no video frame received: {url}")
        except Exception as exc:
            unavailable.append((url, str(exc)))

    if unavailable:
        raise ValueError(f"RTSP stream unavailable: {unavailable}")

    if validate_unified_resolution:
        resolutions = {(info["width"], info["height"]) for info in stream_info.values()}
        if len(resolutions) != 1:
            resolution_map = {
                f"{width}x{height}": [
                    url
                    for url, info in stream_info.items()
                    if (info["width"], info["height"]) == (width, height)
                ]
                for width, height in resolutions
            }
            raise ValueError(f"RTSP streams have inconsistent resolution: {resolution_map}")

    return stream_info
