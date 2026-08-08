from pydantic import BaseModel


class ApiEnvelope(BaseModel):
    success: bool = True
    message: str = ""
    data: dict | list | str | None = None
    command: str = ""

    @classmethod
    def ok(cls, data=None, message: str = "", command: str = "") -> "ApiEnvelope":
        return cls(message=message, data=data, command=command)

    @classmethod
    def fail(cls, message: str, data=None, command: str = "") -> "ApiEnvelope":
        return cls(success=False, message=message, data=data, command=command)


class StartPipelineRequest(BaseModel):
    type: str
    name: str
    config_dir: str
    logger: dict = {}
    messager: dict = {}
    drawer: dict | None = None
    debouncer: dict | None = None
    capturer: dict | None = None


class SchemaRequest(BaseModel):
    pipeline_type: str
