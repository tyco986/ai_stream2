import argparse
import os
import threading
import traceback

import uvicorn
from fastapi import FastAPI
from fastapi.responses import JSONResponse
from pydantic import BaseModel

from utils.pipeline.yolo_pipeline import (
    DetRTSPPipeline,
    DetSahiRTSPPipeline,
    PoseRTSPPipeline,
    SegRTSPPipeline,
)
from utils.pipeline.pipeline_runner import PipelineRunner

PROJECT_NAME = "ai_stream2"
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8092"))

PIPELINE_BUILDERS = {
    "DetRTSPPipeline": DetRTSPPipeline,
    "SegRTSPPipeline": SegRTSPPipeline,
    "PoseRTSPPipeline": PoseRTSPPipeline,
    "DetSahiRTSPPipeline": DetSahiRTSPPipeline,
}


class BuildPipelineRequest(BaseModel):
    type: str
    name: str
    config: dict


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
        self.app.add_api_route(f"{prefix}/build_pipeline", self.build_pipeline, methods=["POST"])
        self.app.add_api_route(f"{prefix}/start_pipeline", self.start_pipeline, methods=["POST"])
        self.app.add_api_route(f"{prefix}/stop_pipeline", self.stop_pipeline, methods=["POST"])

    def is_running(self):
        return self.runner_thread is not None and self.runner_thread.is_alive()

    async def build_pipeline(self, body: BuildPipelineRequest):
        success = False
        message = ""
        if self.is_running():
            message = "pipeline is running"
        elif body.type not in PIPELINE_BUILDERS:
            message = f"unknown pipeline type: {body.type}"
        elif "config_dir" not in body.config:
            message = "config.config_dir is required"
        else:
            try:
                probe_config = body.config.get("probe", {})
                builder = PIPELINE_BUILDERS[body.type](body.config["config_dir"], body.name, **probe_config)
                self.pipeline = builder.build()
                self.runner = PipelineRunner(self.pipeline)
                success = True
            except Exception:
                message = traceback.format_exc()
        return json_response(success, message)

    async def start_pipeline(self):
        success = False
        message = ""
        if self.pipeline is None:
            message = "pipeline not built"
        elif self.is_running():
            message = "pipeline already running"
        else:
            self.runner_thread = threading.Thread(target=self.runner.start, daemon=True)
            self.runner_thread.start()
            success = True
        return json_response(success, message)

    async def stop_pipeline(self):
        success = False
        message = ""
        if self.pipeline is None:
            message = "pipeline not built"
        elif not self.is_running():
            message = "pipeline not running"
        else:
            self.runner.stop()
            self.runner_thread.join()
            self.runner_thread = None
            success = True
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
