import os
import re
from pathlib import Path

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
LOGGER_NAME = "ffmpeg_api"
FFMPEG_BASE = ("ffmpeg", "-hide_banner", "-loglevel", "warning")
FFPROBE_BASE = (
    "ffprobe",
    "-v",
    "error",
    "-rtsp_transport",
    "tcp",
    "-print_format",
    "json",
    "-show_format",
    "-show_streams",
)
PUBLISHER_START_TIMEOUT_S = 1.0
DEFAULT_RECORDINGS_ROOT = Path("/root/recordings")
INPUT_ROOT = Path("/root/tmp")
CAPTURE_OUTPUT_ROOT = Path("/root/outputs/ffmpeg/capture")
NOB_OUTPUT_ROOT = Path("/root/outputs/nob")
LOG_ROOT = Path("/root/logs/ffmpeg")
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8080"))
TIMESTAMP_PATTERN = re.compile(r"^\d{2}:\d{2}:\d{2}(\.\d{1,3})?$")
