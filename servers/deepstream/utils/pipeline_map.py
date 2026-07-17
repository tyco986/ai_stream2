PIPELINE_GENERATOR = {
    "DetVisRTSPPipeline": "DetVisRTSPGenerator",
    "SegVisRTSPPipeline": "SegVisRTSPGenerator",
    "DetSahiVisRTSPPipeline": "DetSahiVisRTSPGenerator",
    "SegSahiVisRTSPPipeline": "SegSahiVisRTSPGenerator",
}

PIPELINE_MAP = PIPELINE_GENERATOR

GENERATOR_PIPELINE = {generator: pipeline for pipeline, generator in PIPELINE_GENERATOR.items()}
