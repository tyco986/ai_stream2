from typing import Literal

from pydantic import BaseModel, ConfigDict, Field


class AppError(Exception):
    def __init__(self, message: str, status_code: int = 400) -> None:
        self.message = message
        self.status_code = status_code
        super().__init__(message)


class ApiEnvelope(BaseModel):
    success: bool = True
    data: dict | list | str | None = None
    message: str = ""
    file: str | None = None

    @classmethod
    def ok(cls, data=None, message: str = "") -> "ApiEnvelope":
        return cls(data=data, message=message)

    @classmethod
    def fail(cls, message: str, data=None) -> "ApiEnvelope":
        return cls(success=False, data=data, message=message)


class OnnxExportConfig(BaseModel):
    model_config = ConfigDict(extra="forbid")

    type: str
    size: int = 640
    opset: int = 18
    batch_size: int | None = 1
    batch_mode: Literal["dynamic", "static"] = "static"
    simplify: bool = False
    max_det: int = 30
    conf: float = Field(default=0.25, gt=0, le=1)
    iou: float = Field(default=0.45, gt=0, le=1)
