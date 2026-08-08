from django.apps import apps
from django.conf import settings

from shared.http.exceptions import AppError

CATEGORY_INFRASTRUCTURE = "infrastructure"
CATEGORY_PIPELINE = "pipeline"
PIPELINE_ID_PREFIX = "pipeline:"


class ServerRegistry:
    def static_entries(self):
        project = settings.PROJECT_NAME
        rows = [
            {
                "id": "ffmpeg",
                "name": "ffmpeg",
                "container_name": f"{project}_ffmpeg",
                "health_upstream": (
                    f"http://{project}_ffmpeg:8080/{project}/ffmpeg/health"
                ),
                "category": CATEGORY_INFRASTRUCTURE,
                "sort_order": 10,
            },
            {
                "id": "export_onnx",
                "name": "export_onnx",
                "container_name": f"{project}_export_onnx",
                "health_upstream": (
                    f"http://{project}_export_onnx:8090/{project}/export_onnx/health"
                ),
                "category": CATEGORY_INFRASTRUCTURE,
                "sort_order": 20,
            },
            {
                "id": "generator",
                "name": "generator",
                "container_name": f"{project}_generator",
                "health_upstream": (
                    f"http://{project}_generator:8091/{project}/generator/health"
                ),
                "category": CATEGORY_INFRASTRUCTURE,
                "sort_order": 30,
            },
            {
                "id": "export_trt",
                "name": "export_trt",
                "container_name": f"{project}_export_trt",
                "health_upstream": (
                    f"http://{project}_export_trt:9000/{project}/export_trt/health"
                ),
                "category": CATEGORY_INFRASTRUCTURE,
                "sort_order": 40,
            },
            {
                "id": "mediamtx",
                "name": "mediamtx",
                "container_name": f"{project}_mediamtx",
                "health_upstream": f"http://{project}_mediamtx:9997/v3/info",
                "category": CATEGORY_INFRASTRUCTURE,
                "sort_order": 50,
            },
            {
                "id": "kafka",
                "name": "kafka",
                "container_name": f"{project}_kafka",
                "health_upstream": (
                    f"http://{project}_kafka:9644/v1/status/ready"
                ),
                "category": CATEGORY_INFRASTRUCTURE,
                "sort_order": 60,
            },
            {
                "id": "nodejs",
                "name": "nodejs",
                "container_name": f"{project}_nodejs",
                "health_upstream": f"http://{project}_nodejs:5173/health",
                "category": CATEGORY_INFRASTRUCTURE,
                "sort_order": 70,
            },
        ]
        return rows

    def pipeline_entries(self):
        project = settings.PROJECT_NAME
        api_port = int(settings.DEEPSTREAM_API_PORT)
        pipeline_model = apps.get_model("pipelines", "Pipeline")
        rows = []
        sort_base = 1000
        index = 0
        for row in pipeline_model.objects.all().order_by("name"):
            container_name = f"{project}_{row.name}"
            rows.append(
                {
                    "id": f"{PIPELINE_ID_PREFIX}{row.id}",
                    "name": row.name,
                    "container_name": container_name,
                    "health_upstream": (
                        f"http://{container_name}:{api_port}"
                        f"/{project}/deepstream/health"
                    ),
                    "category": CATEGORY_PIPELINE,
                    "sort_order": sort_base + index,
                }
            )
            index += 1
        return rows

    def entries(self):
        return self.static_entries() + self.pipeline_entries()

    def get(self, server_id):
        entry = None
        for row in self.entries():
            if row["id"] == server_id:
                entry = row
                break
        return entry

    def require(self, server_id):
        entry = self.get(server_id)
        if entry is None:
            raise AppError("Server not found", status_code=404)
        return entry
