import logging

logger = logging.getLogger("shared.audit")


class AuditService:
    sinks = []

    def register_sink(self, sink):
        existing = {getattr(item, "__qualname__", id(item)) for item in self.sinks}
        key = getattr(sink, "__qualname__", id(sink))
        if key not in existing:
            self.sinks.append(sink)

    def record(self, user, action, detail=""):
        username = getattr(user, "username", None) or "anonymous"
        user_id = getattr(user, "pk", None)
        logger.info(
            "audit action=%s user=%s user_id=%s detail=%s",
            action,
            username,
            user_id,
            detail,
        )
        for sink in self.sinks:
            sink(user, action, detail)
