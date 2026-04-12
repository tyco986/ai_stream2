"""Runtime toggle for nvosd overlay rendering.

Instead of using a probe to strip metadata, leverages nvdsosd's built-in
``display-bbox`` and ``display-text`` properties.  This avoids metadata
manipulation while still allowing future pose-estimation metadata to be
handled cleanly — pose overlays also go through display_meta, which is
controlled by the same mechanism.
"""

import logging

logger = logging.getLogger(__name__)


class OsdToggle:
    """Wraps the nvosd element to toggle overlay display at runtime."""

    def __init__(self, osd_node):
        self._osd = osd_node
        self.show_overlay = True

    def set_overlay(self, show: bool):
        self.show_overlay = show
        self._osd.set({
            "display-bbox": show,
            "display-text": show,
            "display-mask": show,
        })
        logger.info("OSD overlay: %s", "ON" if show else "OFF")
