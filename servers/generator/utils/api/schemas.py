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


class SchemaRequest(BaseModel):
    generator: str
