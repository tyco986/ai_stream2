class NvdspreprocessRtmposeGenerator:
    CUSTOM_LIB_PATH = (
        "/opt/ai_stream2/servers/deepstream/libs/libnvds_rtmpose_preprocess.so"
    )
    UNIQUE_ID = 5
    TARGET_UNIQUE_IDS = 2
    OPERATE_ON_GIE_ID = 1
    TENSOR_NAME = "input"
    PADDING = 1.25
    PIXEL_NORMALIZATION_FACTOR = 0.017124753831663668
    OFFSETS = "123.675;116.28;103.53"
    OPERATE_ON_CLASS_IDS = "0"

    def __init__(
        self,
        batch_size: int,
        infer_width: int,
        infer_height: int,
        channels: int,
        stream_count: int,
        tensor_name: str | None = None,
    ) -> None:
        self.batch_size = batch_size
        self.infer_width = infer_width
        self.infer_height = infer_height
        self.channels = channels
        self.stream_count = stream_count
        self.tensor_name = tensor_name or self.TENSOR_NAME

    def render(self) -> str:
        src_ids = ";".join(str(index) for index in range(self.stream_count))
        roi_lines = "\n".join(
            f"roi-params-src-{index}=0;0;100;100"
            for index in range(self.stream_count)
        )
        network_input_shape = (
            f"{self.batch_size};{self.channels};{self.infer_height};{self.infer_width}"
        )
        text = (
            "# nvdspreprocess for RTMPose: expand object crop, letterbox, NCHW RGB.\n"
            "\n"
            "[property]\n"
            "enable=1\n"
            f"unique-id={self.UNIQUE_ID}\n"
            f"target-unique-ids={self.TARGET_UNIQUE_IDS}\n"
            f"operate-on-gie-id={self.OPERATE_ON_GIE_ID}\n"
            "network-input-order=0\n"
            f"network-input-shape={network_input_shape}\n"
            "process-on-frame=0\n"
            f"processing-width={self.infer_width}\n"
            f"processing-height={self.infer_height}\n"
            "scaling-buf-pool-size=6\n"
            "tensor-buf-pool-size=6\n"
            "network-color-format=0\n"
            "tensor-data-type=0\n"
            f"tensor-name={self.tensor_name}\n"
            "scaling-pool-memory-type=0\n"
            "scaling-pool-compute-hw=1\n"
            "scaling-filter=0\n"
            "maintain-aspect-ratio=0\n"
            "symmetric-padding=0\n"
            f"custom-lib-path={self.CUSTOM_LIB_PATH}\n"
            "custom-tensor-preparation-function=CustomTensorPreparation\n"
            "\n"
            "[user-configs]\n"
            f"padding={self.PADDING}\n"
            f"infer-width={self.infer_width}\n"
            f"infer-height={self.infer_height}\n"
            f"pixel-normalization-factor={self.PIXEL_NORMALIZATION_FACTOR}\n"
            f"offsets={self.OFFSETS}\n"
            "\n"
            "[group-0]\n"
            f"src-ids={src_ids}\n"
            "custom-input-transformation-function=CustomTransformation\n"
            "process-on-roi=1\n"
            "process-on-all-objects=1\n"
            f"{roi_lines}\n"
            "input-object-min-width=32\n"
            "input-object-min-height=32\n"
            f"operate-on-class-ids={self.OPERATE_ON_CLASS_IDS}\n"
            "draw-roi=0\n"
        )
        return text
