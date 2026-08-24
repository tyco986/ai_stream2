import json

from utils.tool.logger.det_logger import DetLogger


class PresenceLogger(DetLogger):
    def log_detection(self, result: dict) -> None:
        pad_index = int(result["pad_index"])
        counter = self.counters.get(pad_index, 0)
        if self.interval == 0 or counter % self.runtime_interval == 0:
            logger = self.get_logger(pad_index)
            logger.info("%s", json.dumps(self.payload(result), ensure_ascii=False))
            logger.info("%s", json.dumps(result["event"], ensure_ascii=False))
            key = self.times_key(result)
            self.pending_times.add(key)
            drawer_ms = result.get("drawer")
            if drawer_ms is not None:
                self.pending_drawer[key] = drawer_ms
            counter = 0
        self.counters[pad_index] = counter + 1
