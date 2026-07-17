import argparse
import os
import threading
import traceback

import uvicorn
import yaml
from fastapi import FastAPI, File, UploadFile
from fastapi.responses import JSONResponse
from pydantic import BaseModel

from utils.pipeline.base import (
    BaseImagePipeline,
    BaseRTSPFakeSinkPipeline,
    BaseRTSPPipeline,
    BaseVideoPipeline,
)
from utils.pipeline.pipeline_runner import PipelineRunner
from utils.pipeline.presence_pipeline import PresenceRTSPPipeline, PresenceVideoPipeline
from utils.pipeline.yolo_pipeline import (
    DetImagePipeline,
    DetVisRTSPPipeline,
    DetSahiImagePipeline,
    DetSahiVisRTSPPipeline,
    DetSahiVideoPipeline,
    DetVideoPipeline,
    SegImagePipeline,
    SegSahiImagePipeline,
    SegSahiVideoPipeline,
    SegSahiVisRTSPPipeline,
    SegVisRTSPPipeline,
    SegVideoPipeline,
)

PROJECT_NAME = "ai_stream2"
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8092"))
RUNNER_LOG_ROOT = "/root/logs/deepstream"
BASE_PIPELINE_TYPES = frozenset(
    {
        "BaseImagePipeline",
        "BaseVideoPipeline",
        "BaseRTSPPipeline",
        "BaseRTSPFakeSinkPipeline",
    }
)
PRESENCE_PIPELINE_TYPES = frozenset({"PresenceRTSPPipeline", "PresenceVideoPipeline"})

PIPELINE_BUILDERS = {
    "BaseImagePipeline": BaseImagePipeline,
    "BaseVideoPipeline": BaseVideoPipeline,
    "BaseRTSPPipeline": BaseRTSPPipeline,
    "BaseRTSPFakeSinkPipeline": BaseRTSPFakeSinkPipeline,
    "DetVisRTSPPipeline": DetVisRTSPPipeline,
    "SegVisRTSPPipeline": SegVisRTSPPipeline,
    "DetImagePipeline": DetImagePipeline,
    "SegImagePipeline": SegImagePipeline,
    "SegSahiImagePipeline": SegSahiImagePipeline,
    "SegSahiVideoPipeline": SegSahiVideoPipeline,
    "SegSahiVisRTSPPipeline": SegSahiVisRTSPPipeline,
    "DetVideoPipeline": DetVideoPipeline,
    "SegVideoPipeline": SegVideoPipeline,
    "DetSahiVisRTSPPipeline": DetSahiVisRTSPPipeline,
    "DetSahiImagePipeline": DetSahiImagePipeline,
    "DetSahiVideoPipeline": DetSahiVideoPipeline,
    "PresenceRTSPPipeline": PresenceRTSPPipeline,
    "PresenceVideoPipeline": PresenceVideoPipeline,
}


class StartPipelineRequest(BaseModel):
    type: str
    name: str
    config_dir: str
    logger: dict = {}
    messager: dict = {}
    drawer: dict | None = None
    debouncer: dict | None = None
    capturer: dict | None = None


def json_response(success: bool, message: str = ""):
    status = 200 if success else 400
    return JSONResponse(content={"success": success, "message": message}, status_code=status)


class DeepStreamServer:
    def __init__(self):
        self.pipeline = None
        self.runner = None
        self.runner_thread = None
        self.app = FastAPI(title="DeepStream API", version="1.0.0")
        prefix = f"/{PROJECT_NAME}/deepstream"
        self.app.add_api_route(f"{prefix}/start_pipeline", self.start_pipeline, methods=["POST"])

    def is_running(self):
        return self.runner_thread is not None and self.runner_thread.is_alive()

    def build_pipeline_kwargs(self, body: StartPipelineRequest) -> dict:
        kwargs = {
            "logger": body.logger,
            "messager": body.messager,
        }
        if body.type in PRESENCE_PIPELINE_TYPES:
            if body.debouncer is not None:
                kwargs["debouncer"] = body.debouncer
            if body.drawer is not None:
                kwargs["drawer"] = body.drawer
            if body.capturer is not None:
                kwargs["capturer"] = body.capturer
        elif body.drawer is not None:
            kwargs["drawer"] = body.drawer
        return kwargs

    async def start_pipeline(self, input: UploadFile = File(...)):
        success = False
        message = ""
        body = StartPipelineRequest(**yaml.safe_load(await input.read()))
        if self.is_running():
            message = "pipeline is running"
        elif body.type not in PIPELINE_BUILDERS:
            message = f"unknown pipeline type: {body.type}"
        else:
            try:
                builder_cls = PIPELINE_BUILDERS[body.type]
                if body.type in BASE_PIPELINE_TYPES:
                    builder = builder_cls(body.config_dir, body.name)
                else:
                    builder = builder_cls(
                        body.config_dir,
                        body.name,
                        **self.build_pipeline_kwargs(body),
                    )
                self.pipeline = builder.build()
                self.runner = PipelineRunner(
                    self.pipeline,
                    logger={"root": f"{RUNNER_LOG_ROOT}/{body.name}/runner"},
                )
                self.runner_thread = threading.Thread(target=self.runner.start, daemon=True)
                self.runner_thread.start()
                success = True
            except Exception:
                message = traceback.format_exc()
        return json_response(success, message)


def parse_args():
    parser = argparse.ArgumentParser(description="DeepStream API service")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    server = DeepStreamServer()
    uvicorn.run(server.app, host=args.host, port=args.port)
