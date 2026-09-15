from pathlib import Path

import av

from utils.tester.rtmpose.constants import (
    BGR_PIX_FMT,
    DEFAULT_FPS,
    ENCODE_PIX_FMT,
    OUTPUT_DIR,
)
from utils.tester.rtmpose.rtmpose_image_tester import RtmposeImageTester


class RtmposeVideoTester(RtmposeImageTester):
    def __call__(self, video_path):
        path = Path(video_path)
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        out_path = OUTPUT_DIR / path.with_suffix(".mp4").name
        source = av.open(str(path))
        video = source.streams.video[0]
        dest = av.open(str(out_path), mode="w")
        stream = dest.add_stream(video.codec.name, rate=video.average_rate or DEFAULT_FPS)
        stream.width = video.width
        stream.height = video.height
        stream.pix_fmt = ENCODE_PIX_FMT
        if video.bit_rate:
            stream.bit_rate = video.bit_rate
        for frame in source.decode(video=0):
            out_frame = av.VideoFrame.from_ndarray(
                self.annotate(frame.to_ndarray(format=BGR_PIX_FMT)),
                format=BGR_PIX_FMT,
            )
            out_frame.pts = frame.pts
            out_frame.time_base = frame.time_base
            for packet in stream.encode(out_frame):
                dest.mux(packet)
        for packet in stream.encode():
            dest.mux(packet)
        dest.close()
        source.close()
        return out_path
