#!/usr/bin/env python3
import argparse
from pathlib import Path

import gi

gi.require_version("Gst", "1.0")
gi.require_version("GstRtspServer", "1.0")
from gi.repository import GLib, Gst, GstRtspServer  # noqa: E402


class LoopVideoRtspServer:
    """Publish local MP4 files as looping RTSP mounts."""

    def __init__(self, port: int):
        self._server = GstRtspServer.RTSPServer.new()
        self._server.set_property("service", str(port))
        self._mounts = self._server.get_mount_points()

    def add_loop_mp4(self, mount_path: str, video_path: Path):
        if not video_path.exists():
            raise FileNotFoundError(video_path)
        factory = GstRtspServer.RTSPMediaFactory.new()
        launch = (
            "( multifilesrc location=\"{location}\" loop=true "
            "! qtdemux ! h264parse ! rtph264pay name=pay0 pt=96 )"
        ).format(location=str(video_path.resolve()))
        factory.set_launch(launch)
        factory.set_shared(True)
        self._mounts.add_factory(mount_path, factory)

    def start(self):
        self._server.attach(None)


def parse_args():
    base = Path(__file__).resolve().parent.parent / "example_data"
    parser = argparse.ArgumentParser(description="Publish looping RTSP streams for tests.")
    parser.add_argument("--port", type=int, default=8554)
    parser.add_argument("--video1", type=Path, default=base / "video1_bf0.mp4")
    parser.add_argument("--video2", type=Path, default=base / "video2_bf0.mp4")
    return parser.parse_args()


def main():
    args = parse_args()
    Gst.init(None)

    server = LoopVideoRtspServer(args.port)
    server.add_loop_mp4("/video1", args.video1)
    server.add_loop_mp4("/video2", args.video2)
    server.start()

    print(f"Loop RTSP server started at rtsp://0.0.0.0:{args.port}/video1 and /video2")
    loop = GLib.MainLoop()
    loop.run()


if __name__ == "__main__":
    main()
