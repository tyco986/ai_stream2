import os
from pathlib import Path

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
LOGGER_NAME = "export_trt"
DEFAULT_MODEL_ROOT = Path("/root/models")
DEFAULT_LOG_ROOT = Path("/root/logs/export_trt")
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "9000"))
