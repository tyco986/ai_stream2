class PipelineRunner:
    """Start and block until a pyservicemaker pipeline finishes."""

    def __init__(self, pipeline):
        self.pipeline = pipeline

    def start(self):
        self.pipeline.start().wait()

    def stop(self):
        self.pipeline.stop()