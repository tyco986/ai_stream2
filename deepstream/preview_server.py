import logging

import gi

gi.require_version("GstRtspServer", "1.0")
from gi.repository import GstRtspServer  # noqa: E402

logger = logging.getLogger(__name__)


class PreviewServer:
    """Wrap ``GstRtspServer`` to serve a single RTSP mount at ``/preview``.

    The pipeline's preview branch ends with a ``udpsink`` writing RTP to
    ``127.0.0.1:<udpsrc_port>``.  This server creates an RTSP media factory
    whose launch string reads from the same UDP port, making the live
    preview available at ``rtsp://0.0.0.0:<rtsp_port>/preview``.

    MediaMTX can then pull this RTSP source and re-publish as WebRTC for
    the frontend.
    """

    def __init__(self, rtsp_port="8554", udpsrc_port=5400):
        self._rtsp_port = rtsp_port
        self._udpsrc_port = udpsrc_port
        self._server = None

    def start(self):
        """Create and attach the RTSP server to the GLib default context.

        Must be called **before** ``pipeline.wait()`` since ``wait()``
        runs the GLib main loop that the server hooks into.
        """
        self._server = GstRtspServer.RTSPServer.new()
        self._server.set_property("service", str(self._rtsp_port))

        factory = GstRtspServer.RTSPMediaFactory.new()
        launch = (
            "( udpsrc port={port} "
            "caps=\"application/x-rtp,media=video,encoding-name=H264,payload=96\" "
            "! rtph264depay ! h264parse ! rtph264pay name=pay0 pt=96 )"
        ).format(port=self._udpsrc_port)
        factory.set_launch(launch)
        factory.set_shared(True)

        mounts = self._server.get_mount_points()
        mounts.add_factory("/preview", factory)

        self._server.attach(None)
        logger.info(
            "RTSP preview server listening on rtsp://0.0.0.0:%s/preview (udpsrc port=%d)",
            self._rtsp_port, self._udpsrc_port,
        )
