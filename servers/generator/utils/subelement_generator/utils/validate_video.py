from pathlib import Path

import av


def probe_video(path: str | Path) -> dict[str, int]:
    video_path = Path(path).expanduser().resolve()
    assert video_path.is_file(), f"input video not found: {video_path}"

    with av.open(str(video_path)) as container:
        stream = container.streams.video[0]
        rate = stream.average_rate or stream.codec_context.framerate
        if rate is None:
            raise RuntimeError("fps not available from stream metadata")
        fps = round(float(rate))
        if fps <= 0:
            raise RuntimeError(f"invalid fps: {fps}")
        for frame in container.decode(stream):
            return {
                "width": frame.width,
                "height": frame.height,
                "fps": fps,
            }
        raise RuntimeError(f"no video frame received: {video_path}")
