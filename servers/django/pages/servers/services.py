from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path

from django.conf import settings

from pages.servers.clients import DockerProxyClient, HealthHttpClient
from pages.servers.registry import ServerRegistry
from shared.http.exceptions import AppError


class ServerStatusCache:
    entries = {}

    def get(self, server_id):
        row = self.entries.get(server_id)
        return dict(row) if row else None

    def set(self, server_id, status, last_refresh_at):
        self.entries[server_id] = {
            "status": status,
            "last_refresh_at": last_refresh_at,
        }


class HealthProbeService:
    def __init__(self):
        self.registry = ServerRegistry()
        self.cache = ServerStatusCache()
        self.http = HealthHttpClient()

    def probe(self, server_id):
        entry = self.registry.require(server_id)
        result = self.http.get(entry["health_upstream"])
        status = "online" if result["ok"] else "offline"
        last_refresh_at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        self.cache.set(server_id, status, last_refresh_at)
        return {
            "id": server_id,
            "status": status,
            "last_refresh_at": last_refresh_at,
            "latency_ms": result["latency_ms"],
            "detail": "" if result["ok"] else (result["detail"] or "offline"),
            "timed_out": result["timed_out"],
            "body": result["body"],
            "ok": result["ok"],
        }


class ServerStatusService:
    def __init__(self):
        self.registry = ServerRegistry()
        self.cache = ServerStatusCache()
        self.probe = HealthProbeService()

    def list_servers(self):
        items = []
        for entry in self.registry.entries():
            cached = self.cache.get(entry["id"])
            status = "offline"
            last_refresh_at = None
            if cached is not None:
                status = cached["status"]
                last_refresh_at = cached["last_refresh_at"]
            items.append(
                {
                    "id": entry["id"],
                    "name": entry["name"],
                    "status": status,
                    "last_refresh_at": last_refresh_at,
                    "category": entry["category"],
                }
            )
        return {"items": items}

    def refresh_all(self):
        entries = self.registry.entries()
        results_by_id = {}
        with ThreadPoolExecutor(max_workers=min(8, len(entries) or 1)) as pool:
            futures = {
                pool.submit(self.probe.probe, entry["id"]): entry["id"]
                for entry in entries
            }
            for future in as_completed(futures):
                server_id = futures[future]
                results_by_id[server_id] = future.result()
        results = []
        for entry in entries:
            row = results_by_id[entry["id"]]
            results.append(
                {
                    "id": row["id"],
                    "status": row["status"],
                    "last_refresh_at": row["last_refresh_at"],
                    "latency_ms": row["latency_ms"],
                    "detail": row["detail"],
                }
            )
        return {"results": results}


class RestartService:
    def __init__(self):
        self.registry = ServerRegistry()
        self.docker = DockerProxyClient()
        self.probe = HealthProbeService()

    def restart(self, server_id):
        entry = self.registry.require(server_id)
        self.docker.restart(entry["container_name"])
        restarted_at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        refresh = self.probe.probe(server_id)
        return {
            "id": server_id,
            "container_name": entry["container_name"],
            "restarted_at": restarted_at,
            "refresh": {
                "id": refresh["id"],
                "status": refresh["status"],
                "last_refresh_at": refresh["last_refresh_at"],
                "latency_ms": refresh["latency_ms"],
                "detail": refresh["detail"],
            },
        }


class ServerLogService:
    def __init__(self):
        self.registry = ServerRegistry()
        self.docker = DockerProxyClient()
        self.log_root = Path(settings.SERVERS_LOG_ROOT)

    def get_logs(self, server_id, tail=500):
        entry = self.registry.require(server_id)
        limit = int(tail) if tail is not None else 500
        if limit < 1:
            limit = 500
        content = self.read_file_logs(entry["id"], limit)
        if not content:
            try:
                content = self.docker.logs(entry["container_name"], tail=limit)
            except AppError:
                content = ""
        return {"content": content or ""}

    def read_file_logs(self, server_id, tail):
        directory = self.log_root / server_id
        content = ""
        if not directory.is_dir():
            return content
        candidates = [
            path
            for path in directory.rglob("*")
            if path.is_file() and path.suffix in (".log", ".txt", "")
        ]
        if not candidates:
            return content
        candidates.sort(key=lambda path: path.stat().st_mtime, reverse=True)
        text = candidates[0].read_text(encoding="utf-8", errors="replace")
        lines = text.splitlines()
        content = "\n".join(lines[-tail:])
        if content and not content.endswith("\n"):
            content = content + "\n"
        return content
