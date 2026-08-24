from pyservicemaker import Probe

from utils.base_pipeline.utils.call_probe import CallProbe


class LatencyTimesAttach:
    def attach_latency_and_times(self, logger, target="nvvideoconvert"):
        self.attach_latency_probe(target)
        self.pipeline.attach(target, Probe("times", CallProbe(logger.log_times_batch)))

    def attach_latency_and_times_indexed(self, logger, indices, target="nvvideoconvert"):
        for index in indices:
            name = f"{target}{index}"
            self.pipeline.attach(name, "latency_probe", f"latency{index}")
            self.pipeline.attach(name, Probe(f"times{index}", CallProbe(logger.log_times_batch)))
