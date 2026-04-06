#!/usr/bin/env python3
import argparse
from pathlib import Path

import gi

gi.require_version("Gst", "1.0")
gi.require_version("GstRtspServer", "1.0")
from gi.repository import GLib, Gst, GstRtspServer  # noqa: E402


class MultiVideoRtspServer:
    """Publish local MP4 videos as RTSP mounts."""

    def __init__(self, port: int):
        self._port = str(port)
        self._server = GstRtspServer.RTSPServer.new()
        self._server.set_property("service", self._port)
        self._mounts = self._server.get_mount_points()

    def add_mp4(self, mount_path: str, video_path: Path):
        if not mount_path.startswith("/"):
            raise ValueError("mount_path must start with '/'")
        if not video_path.exists():
            raise FileNotFoundError(f"Video not found: {video_path}")

        factory = GstRtspServer.RTSPMediaFactory.new()
        # For test videos encoded in H.264 inside MP4:
        # MP4 -> demux -> h264 parse -> RTP payload.
        launch = (
            "( filesrc location=\"{location}\" "
            "! qtdemux "
            "! h264parse "
            "! rtph264pay name=pay0 pt=96 )"
        ).format(location=str(video_path.resolve()))
        factory.set_launch(launch)
        factory.set_shared(True)
        self._mounts.add_factory(mount_path, factory)

    def start(self):
        self._server.attach(None)


def parse_args():
    default_dir = Path(__file__).resolve().parent.parent / "example_data"
    default_video_1 = default_dir / "video1_bf0.mp4"
    default_video_2 = default_dir / "video2_bf0.mp4"

    parser = argparse.ArgumentParser(
        description="Publish two local MP4 videos as RTSP streams."
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8554,
        help="RTSP service port, default: 8554",
    )
    parser.add_argument(
        "--video1",
        type=Path,
        default=default_video_1,
        help=f"Video file for /video1, default: {default_video_1}",
    )
    parser.add_argument(
        "--video2",
        type=Path,
        default=default_video_2,
        help=f"Video file for /video2, default: {default_video_2}",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    Gst.init(None)

    server = MultiVideoRtspServer(port=args.port)
    server.add_mp4("/video1", args.video1)
    server.add_mp4("/video2", args.video2)
    server.start()

    print(f"RTSP server started on port {args.port}")
    print(f"  rtsp://127.0.0.1:{args.port}/video1  <- {args.video1.resolve()}")
    print(f"  rtsp://127.0.0.1:{args.port}/video2  <- {args.video2.resolve()}")
    print("Press Ctrl+C to stop.")

    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
