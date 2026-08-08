import re
import time
from pathlib import Path

from django.conf import settings

from pages.pipelines.clients import DeepStreamClient
from pages.servers.clients import DockerProxyClient, HealthHttpClient
from shared.http.exceptions import AppError

DOCKER_NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]*$")


class DeepStreamContainerService:
    def __init__(self, docker=None, health=None):
        self.docker = docker if docker is not None else DockerProxyClient()
        self.health = health if health is not None else HealthHttpClient()
        self.project = settings.PROJECT_NAME
        self.api_port = int(settings.DEEPSTREAM_API_PORT)

    def container_name(self, pipeline_name):
        return f"{self.project}_{pipeline_name}"

    def validate_pipeline_name(self, pipeline_name):
        if not pipeline_name or not DOCKER_NAME_RE.match(pipeline_name):
            raise AppError(
                "Pipeline name must match Docker container name rules "
                "[A-Za-z0-9][A-Za-z0-9_.-]*",
                status_code=400,
            )

    def base_url(self, pipeline_name):
        return f"http://{self.container_name(pipeline_name)}:{self.api_port}"

    def health_url(self, pipeline_name):
        return f"{self.base_url(pipeline_name)}/{self.project}/deepstream/health"

    def pipeline_status_url(self, pipeline_name):
        return f"{self.base_url(pipeline_name)}/{self.project}/deepstream/pipeline/status"

    def generator_config_dir(self, pipeline_name):
        return Path(settings.GENERATOR_CONFIG_ROOT) / pipeline_name

    def deepstream_config_path(self, pipeline_name):
        return Path(settings.DEEPSTREAM_CONFIG_ROOT) / f"{pipeline_name}.yaml"

    def uses_dev_image(self):
        image = (settings.DEEPSTREAM_IMAGE or "").strip()
        return image.endswith("_dev") or settings.DEBUG

    def app_bind(self, host_root):
        return f"{host_root}/servers/deepstream:/app"

    def build_binds(self, host_root):
        binds = [
            f"{host_root}/configs:/root/configs",
            f"{host_root}/models:/root/models",
            f"{host_root}/attachments:/root/attachments",
            f"{host_root}/outputs:/root/outputs",
            f"{host_root}/logs:/root/logs",
            "/etc/localtime:/etc/localtime:ro",
        ]
        if self.uses_dev_image():
            binds.append(self.app_bind(host_root))
        return binds

    def create(self, pipeline_id, pipeline_name, host_port):
        self.validate_pipeline_name(pipeline_name)
        host_root = (settings.HOST_PROJECT_ROOT or "").rstrip("/")
        if not host_root:
            raise AppError("HOST_PROJECT_ROOT is not configured", status_code=500)
        name = self.container_name(pipeline_name)
        api_port = str(self.api_port)
        body = {
            "Image": settings.DEEPSTREAM_IMAGE,
            "Env": [
                f"PROJECT_NAME={self.project}",
                f"DS_PREVIEW_RTP_HOST={self.project}_mediamtx",
            ],
            "Labels": {
                "ai_stream2.role": "pipeline",
                "ai_stream2.pipeline_id": str(pipeline_id),
                "ai_stream2.pipeline_name": pipeline_name,
            },
            "ExposedPorts": {f"{api_port}/tcp": {}},
            "HostConfig": {
                "NetworkMode": f"{self.project}_default",
                "Binds": self.build_binds(host_root),
                "PortBindings": {
                    f"{api_port}/tcp": [{"HostPort": str(host_port)}],
                },
                "DeviceRequests": [
                    {
                        "Driver": "nvidia",
                        "Count": -1,
                        "Capabilities": [["gpu"]],
                    }
                ],
            },
        }
        self.docker.create_container(name, body)
        return name

    def exists(self, pipeline_name):
        found = True
        try:
            self.docker.inspect(self.container_name(pipeline_name))
        except AppError as exc:
            message = str(exc.detail).lower()
            missing = "404" in message or "no such container" in message
            if not missing:
                raise
            found = False
        return found

    def has_required_binds(self, pipeline_name):
        ok = True
        if self.uses_dev_image():
            info = self.docker.inspect(self.container_name(pipeline_name))
            host_config = info.get("HostConfig") or {}
            binds = host_config.get("Binds") or []
            host_root = (settings.HOST_PROJECT_ROOT or "").rstrip("/")
            expected = self.app_bind(host_root)
            ok = expected in binds
        return ok

    def start(self, pipeline_name):
        self.docker.start(self.container_name(pipeline_name))

    def stop(self, pipeline_name):
        name = self.container_name(pipeline_name)
        try:
            self.docker.stop(name)
        except AppError as exc:
            message = str(exc.detail).lower()
            ignored = (
                "404" in message
                or "no such container" in message
                or "is not running" in message
            )
            if not ignored:
                raise

    def remove(self, pipeline_name, force=True):
        name = self.container_name(pipeline_name)
        try:
            self.docker.remove(name, force=force)
        except AppError as exc:
            message = str(exc.detail).lower()
            ignored = "404" in message or "no such container" in message
            if not ignored:
                raise

    def probe(self, pipeline_name):
        ok = False
        detail = ""
        pipeline_running = False
        name = self.container_name(pipeline_name)
        info = None
        try:
            info = self.docker.inspect(name)
        except AppError as exc:
            detail = str(exc.detail)
        if info is not None:
            state = info.get("State") or {}
            if not state.get("Running"):
                exit_code = state.get("ExitCode")
                detail = (
                    f"container not running "
                    f"(status={state.get('Status')}, exit={exit_code})"
                )
            else:
                result = self.health.get(self.health_url(pipeline_name))
                if result["ok"]:
                    ok = True
                    try:
                        data = DeepStreamClient(self.base_url(pipeline_name)).get_pipeline_status()
                        pipeline_running = bool(data.get("pipeline_running"))
                    except AppError as exc:
                        detail = str(exc.detail)
                        ok = False
                else:
                    detail = result["detail"] or "offline"
        return {
            "ok": ok,
            "detail": detail,
            "pipeline_running": pipeline_running,
        }

    def wait_healthy(self, pipeline_name):
        url = self.health_url(pipeline_name)
        name = self.container_name(pipeline_name)
        deadline = time.monotonic() + float(settings.DEEPSTREAM_HEALTH_TIMEOUT)
        interval = float(settings.DEEPSTREAM_HEALTH_POLL_INTERVAL)
        healthy = False
        last_detail = "health check timeout"
        while time.monotonic() < deadline:
            info = self.docker.inspect(name)
            state = info.get("State") or {}
            if not state.get("Running"):
                exit_code = state.get("ExitCode")
                logs = (self.docker.logs(name, tail=40) or "").strip()
                tail = logs[-800:] if logs else "(no logs)"
                last_detail = f"exited code={exit_code}: {tail}"
                break
            result = self.health.get(url)
            if result["ok"]:
                healthy = True
                break
            last_detail = result["detail"] or "offline"
            time.sleep(interval)
        if not healthy:
            raise AppError(
                f"DeepStream container not healthy: {last_detail}",
                status_code=502,
            )
