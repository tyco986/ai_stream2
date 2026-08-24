from utils.tool.logger.det_logger import DetLogger


class StgcnppLogger(DetLogger):
    def payload(self, result: dict) -> dict:
        record = super().payload(result)
        record["actions"] = [item["action"] for item in result["objects"]]
        return record
