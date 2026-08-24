from pyservicemaker import Probe

from utils.base_pipeline.utils.call_probe import CallProbe


class DetectionAttach:
    def fade_drawer_params(self) -> dict:
        return dict(self.drawer)

    def handle_detections(self, batch_meta):
        for result in self.parser(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)

    def attach_detections(self, name, target="nvdsanalytics"):
        self.pipeline.attach(target, Probe(name, CallProbe(self.handle_detections)))

    def attach_handler(self, target, name, handler):
        self.pipeline.attach(target, Probe(name, CallProbe(handler)))
