import os
from pathlib import Path

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
LOGGER_NAME = "deepstream_api"
LOG_ROOT = Path("/root/logs/deepstream")
RUNNER_LOG_ROOT = LOG_ROOT
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8092"))
BASE_PIPELINE_TYPES = frozenset(
    {
        "BaseImagePipeline",
        "BaseVideoPipeline",
        "BaseRTSPPipeline",
    }
)
PRESENCE_PIPELINE_TYPES = frozenset({"PresenceRTSPPipeline", "PresenceVideoPipeline"})
