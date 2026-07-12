PIPELINE_GENERATOR = {
    "DetRTSPPipeline": "DetVisRTSPGenerator",
    "SegRTSPPipeline": "SegVisRTSPGenerator",
    "PoseRTSPPipeline": "PoseVisRTSPGenerator",
    "DetSahiRTSPPipeline": "DetSahiVisRTSPGenerator",
}

PIPELINE_MAP = PIPELINE_GENERATOR

GENERATOR_PIPELINE = {generator: pipeline for pipeline, generator in PIPELINE_GENERATOR.items()}
