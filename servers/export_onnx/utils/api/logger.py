import json
import logging
from datetime import datetime
from logging.handlers import TimedRotatingFileHandler

from starlette.requests import Request

from utils.api.constants import LOG_HEADERS, LOG_MAX_DAYS, LOG_PAGE, LOG_ROOT, LOG_SUFFIX


class RequestLogger:
    logger = logging.getLogger(LOG_PAGE)

    def __init__(self) -> None:
        self.logger = RequestLogger.logger
        directory = LOG_ROOT / LOG_PAGE
        directory.mkdir(parents=True, exist_ok=True)
        if not self.logger.handlers:
            handler = TimedRotatingFileHandler(
                directory / f"{LOG_PAGE}{LOG_SUFFIX}",
                when="midnight",
                backupCount=LOG_MAX_DAYS,
                encoding="utf-8",
            )
            handler.setFormatter(logging.Formatter("%(message)s"))
            self.logger.setLevel(logging.INFO)
            self.logger.propagate = False
            self.logger.addHandler(handler)

    def write(
        self,
        request: Request,
        code: int,
        output: bytes,
        latency_ms: float,
        error: str,
        in_bytes: int,
    ) -> None:
        output_text = output.decode("utf-8", errors="replace")
        output_value = self.json_value(output_text)
        header = {
            name: request.headers[name]
            for name in LOG_HEADERS
            if request.headers.get(name)
        }
        entry = {
            "ts": datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3],
            "method": request.method,
            "url": str(request.url.path)
            + (f"?{request.url.query}" if request.url.query else ""),
            "code": code,
            "input": "",
            "output": output_value,
            "latency": round(latency_ms, 2),
            "ip": request.client.host if request.client else "",
            "in_bytes": in_bytes,
            "out_bytes": len(output),
            "error": error,
            "header": header,
        }
        text = json.dumps(entry, ensure_ascii=False, separators=(",", ":"))
        text = text.replace(
            f'"latency":{entry["latency"]}',
            f'"latency":{latency_ms:.2f}',
            1,
        )
        self.logger.info(text)

    def json_value(self, text: str):
        value = ""
        if text[:1] in "{[":
            value = json.loads(text)
        return value
