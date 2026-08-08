from pydantic import BaseModel


class ApiErrorResponse(BaseModel):
    success: bool = False
    message: str
    output: None = None
    data: dict | list | str | None = None


class ApiJsonResponse(BaseModel):
    success: bool = True
    message: str = ""
    output: None = None
    data: dict | list | str | None = None

    @classmethod
    def ok(cls, message: str = "", data=None) -> "ApiJsonResponse":
        return cls(message=message, data=data)
