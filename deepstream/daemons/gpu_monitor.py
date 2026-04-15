import logging
import threading

import pynvml

logger = logging.getLogger(__name__)


class GpuMemoryMonitor:
    """Daemon thread that logs GPU memory usage at a fixed interval.

    Emits a warning when memory utilisation exceeds 90 %.
    """

    WARN_THRESHOLD_PCT = 90

    def __init__(self, interval=30, gpu_index=0):
        self._interval = interval
        self._gpu_index = gpu_index
        self._shutdown = threading.Event()

    # ------------------------------------------------------------------
    # main loop (called via daemon thread)
    # ------------------------------------------------------------------

    def run(self):
        pynvml.nvmlInit()
        handle = pynvml.nvmlDeviceGetHandleByIndex(self._gpu_index)
        name = pynvml.nvmlDeviceGetName(handle)
        logger.info("GpuMemoryMonitor started: GPU %d (%s)", self._gpu_index, name)

        while not self._shutdown.wait(timeout=self._interval):
            try:
                info = pynvml.nvmlDeviceGetMemoryInfo(handle)
                used_mb = info.used / (1024 * 1024)
                total_mb = info.total / (1024 * 1024)
                pct = (info.used / info.total) * 100

                if pct > self.WARN_THRESHOLD_PCT:
                    logger.warning(
                        "GPU %d memory HIGH: %.0f / %.0f MB (%.1f%%)",
                        self._gpu_index, used_mb, total_mb, pct,
                    )
                else:
                    logger.info(
                        "GPU %d memory: %.0f / %.0f MB (%.1f%%)",
                        self._gpu_index, used_mb, total_mb, pct,
                    )
            except Exception:
                logger.exception("Failed to read GPU memory stats")

    def stop(self):
        self._shutdown.set()
