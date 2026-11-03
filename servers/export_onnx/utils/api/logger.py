import json
import logging
from datetime import datetime
from logging.handlers import TimedRotatingFileHandler

from utils.api.constants import (
    LOG_MAX_DAYS,
    LOG_ROOT,
    LOG_SUFFIX,
)


class RequestLogger:
    logger = logging.getLogger("export_onnx")

    def __init__(self) -> None:
        self.logger = RequestLogger.logger
        LOG_ROOT.mkdir(parents=True, exist_ok=True)
        if not self.logger.handlers:
            handler = TimedRotatingFileHandler(
                LOG_ROOT / f"export_onnx{LOG_SUFFIX}",
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
        method: str,
        url: str,
        code: int,
        logged_input: str,
        output: str | dict,
        latency_ms: float,
        ip: str,
        in_bytes: int,
        out_bytes: int,
        error: str,
        header: dict,
    ) -> None:
        entry = {
            "ts": datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3],
            "method": method,
            "url": url,
            "code": code,
            "input": logged_input,
            "output": output,
            "latency": latency_ms,
            "ip": ip,
            "in_bytes": in_bytes,
            "out_bytes": out_bytes,
            "error": error,
            "header": header,
        }
        text = json.dumps(entry, ensure_ascii=False, separators=(",", ":"))
        self.logger.info(text)

    def output_value(self, output: bytes):
        marker = b'name="file"; filename="'
        at = output.find(marker)
        start = at + len(marker) if at != -1 else -1
        end = output.find(b'"', start) if start != -1 else -1
        value = f"multipart({output[start:end].decode()})" if end != -1 else ""
        if end == -1 and output[:1] in b"{[":
            value = json.loads(output)
        return value

