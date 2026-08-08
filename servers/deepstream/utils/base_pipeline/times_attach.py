from pyservicemaker import Probe

from utils.probe.times_probe import TimesProbe


class LatencyTimesAttach:
    """Attach latency_probe then TimesProbe on the same element (latency first)."""

    def attach_latency_and_times(self, logger, target="nvvideoconvert"):
        self.attach_latency_probe(target)
        self.pipeline.attach(target, Probe("times", TimesProbe(logger)))

    def attach_latency_and_times_indexed(self, logger, indices, target="nvvideoconvert"):
        for index in indices:
            name = f"{target}{index}"
            self.pipeline.attach(name, "latency_probe", f"latency{index}")
            self.pipeline.attach(name, Probe(f"times{index}", TimesProbe(logger)))
