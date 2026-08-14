from pydantic import BaseModel, ConfigDict


class AppError(Exception):
    def __init__(self, message: str, status_code: int = 400) -> None:
        self.message = message
        self.status_code = status_code
        super().__init__(message)


class ApiEnvelope(BaseModel):
    success: bool = True
    message: str = ""
    data: dict | list | str | None = None

    @classmethod
    def ok(cls, data=None, message: str = "") -> "ApiEnvelope":
        return cls(message=message, data=data)

    @classmethod
    def fail(cls, message: str, data=None) -> "ApiEnvelope":
        return cls(success=False, message=message, data=data)


class TrtExportConfig(BaseModel):
    model_config = ConfigDict(extra="forbid")

    type: str
    batch_size: int | None = None
    gpu_id: int = 0
    precision: str | None = None
    opt_level: int | None = None
