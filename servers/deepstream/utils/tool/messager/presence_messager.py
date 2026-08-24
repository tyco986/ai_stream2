from typing import Any

from utils.tool.messager.det_messager import DetMessager


class PresenceMessager(DetMessager):
    def format_message(self, result: dict) -> dict:
        message = {
            "objects": [list[Any](item["object"]) for item in result["objects"]],
            "event": result["event"],
        }
        return message
