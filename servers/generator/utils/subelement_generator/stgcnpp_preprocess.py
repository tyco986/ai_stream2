class StgcnppPreprocessGenerator:
    CUSTOM_LIB_PATH = (
        "/opt/ai_stream2/servers/deepstream/libs/libnvds_stgcnpp_preprocess.so"
    )
    UNIQUE_ID = 3
    TARGET_UNIQUE_IDS = 4
    POSE_GIE_ID = 2

    def __init__(
        self,
        batch_size: int,
        clip_len: int,
        num_joints: int,
        num_person: int,
        infer_width: int,
        infer_height: int,
        tensor_name: str,
        stream_count: int,
    ) -> None:
        self.batch_size = batch_size
        self.clip_len = clip_len
        self.num_joints = num_joints
        self.num_person = num_person
        self.infer_width = infer_width
        self.infer_height = infer_height
        self.tensor_name = tensor_name
        self.stream_count = stream_count

    def render(self) -> str:
        src_ids = ";".join(str(index) for index in range(self.stream_count))
        roi_lines = "\n".join(
            f"roi-params-src-{index}=0;0;100;100"
            for index in range(self.stream_count)
        )
        network_input_shape = (
            f"{self.batch_size};{self.num_person};{self.clip_len};"
            f"{self.num_joints};3"
        )
        text = (
            "# nvdspreprocess for PYSKL ST-GCN++: input (N, M, T, V, C)\n"
            "\n"
            "[property]\n"
            "enable=1\n"
            f"unique-id={self.UNIQUE_ID}\n"
            f"target-unique-ids={self.TARGET_UNIQUE_IDS}\n"
            "network-input-order=2\n"
            f"network-input-shape={network_input_shape}\n"
            "process-on-frame=0\n"
            f"processing-width={self.num_joints}\n"
            f"processing-height={self.clip_len}\n"
            "scaling-buf-pool-size=6\n"
            "tensor-buf-pool-size=6\n"
            "network-color-format=0\n"
            "tensor-data-type=0\n"
            f"tensor-name={self.tensor_name}\n"
            "scaling-pool-memory-type=0\n"
            "scaling-pool-compute-hw=1\n"
            "scaling-filter=0\n"
            f"custom-lib-path={self.CUSTOM_LIB_PATH}\n"
            "custom-tensor-preparation-function=CustomTensorPreparation\n"
            "\n"
            "[user-configs]\n"
            f"frames-sequence-length={self.clip_len}\n"
            f"num-joints={self.num_joints}\n"
            f"num-person={self.num_person}\n"
            f"pose-gie-id={self.POSE_GIE_ID}\n"
            f"infer-width={self.infer_width}\n"
            f"infer-height={self.infer_height}\n"
            "\n"
            "[group-0]\n"
            f"src-ids={src_ids}\n"
            "custom-input-transformation-function=CustomTransformation\n"
            "process-on-roi=1\n"
            "process-on-all-objects=1\n"
            f"{roi_lines}\n"
            "input-object-min-width=32\n"
            "input-object-min-height=32\n"
            "draw-roi=0\n"
        )
        return text
