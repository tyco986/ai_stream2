import json

from utils.probe.utils.logger.det_logger import DetLogger


class PresenceLogger(DetLogger):
    def log_detection(self, result: dict) -> None:
        pad_index = int(result["pad_index"])
        counter = self.counters.get(pad_index, 0)
        if self.interval == 0 or counter % self.runtime_interval == 0:
            logger = self.get_logger(pad_index)
            logger.info("%s", json.dumps(self.payload(result), ensure_ascii=False))
            logger.info("%s", json.dumps(result["event"], ensure_ascii=False))
            self.pending_times.add(self.times_key(result))
            counter = 0
        self.counters[pad_index] = counter + 1
