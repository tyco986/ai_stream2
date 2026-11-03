from copy import deepcopy

# nvsahipreprocess config-file (INI). Slice geometry is set on the GStreamer element, not here.
# Based on deepstream-sahi python_test/deepstream-test-sahi/config/preprocess/preprocess_640.txt

nvsahipreprocess_default_config = [
    '[property]',
    'enable=1',
    'target-unique-ids=1',
    '# 0=NCHW, 1=NHWC',
    'network-input-order=0',
    'maintain-aspect-ratio=1',
    'symmetric-padding=1',
    'processing-width=640',
    'processing-height=640',
    'scaling-buf-pool-size=6',
    'tensor-buf-pool-size=6',
    # network-input-shape: B;C;H;W
    'network-input-shape=16;3;640;640',
    # 0=RGB, 1=BGR, 2=GRAY
    'network-color-format=0',
    # 0=FP32, 1=UINT8, 2=INT8, 3=UINT32, 4=INT32, 5=FP16
    'tensor-data-type=0',
    'tensor-name=images',
    # 0=DEFAULT, 1=CUDA_PINNED, 2=CUDA_DEVICE, 3=CUDA_UNIFIED, 4=SURFACE_ARRAY
    'scaling-pool-memory-type=0',
    # 0=Default, 1=GPU, 2=VIC(Jetson)
    'scaling-pool-compute-hw=0',
    # 0=Nearest, 1=Bilinear
    'scaling-filter=0',
    'custom-lib-path=/opt/nvidia/deepstream/deepstream/lib/gst-plugins/libcustom2d_preprocess.so',
    'custom-tensor-preparation-function=CustomTensorPreparation',

    '[user-configs]',
    'pixel-normalization-factor=0.003921568'
]


class NvsahipreprocessGenerator:
    """Write nvsahipreprocess ``config-file`` (INI) for SAHI tensor preparation.

    Slice geometry (``slice-width``, ``overlap-*``) is set on the GStreamer element in
    ``pipeline.yml``; this file only covers tensor layout and preprocessing for nvinfer.
    """

    TENSOR_DATA_TYPE_MAP = {
        "fp32": 0,
        "uint8": 1,
        "int8": 2,
        "uint32": 3,
        "int32": 4,
        "fp16": 5,
    }

    def __init__(
        self,
        network_input_shape="16;3;640;640",
        target_unique_ids: int = 1,
        tensor_data_type: int = 0,
        tensor_name: str = "input",
    ):
        self.config = deepcopy(nvsahipreprocess_default_config)
        self.target_unique_ids = target_unique_ids
        assert tensor_data_type in self.TENSOR_DATA_TYPE_MAP.values(), (
            f"unsupported tensor_data_type: {tensor_data_type}"
        )
        self.tensor_data_type = tensor_data_type
        self.tensor_name = tensor_name
        self.network_input_shape = network_input_shape
        shape_parts = [int(value) for value in str(network_input_shape).split(";")]
        self.config[self.config.index('network-input-shape=16;3;640;640')] = f'network-input-shape={self.network_input_shape}'
        self.config[self.config.index('processing-height=640')] = f'processing-height={shape_parts[2]}'
        self.config[self.config.index('processing-width=640')] = f'processing-width={shape_parts[3]}'
        self.config[self.config.index('tensor-data-type=0')] = f'tensor-data-type={self.tensor_data_type}'
        self.config[self.config.index('tensor-name=images')] = f'tensor-name={self.tensor_name}'
        self.config[self.config.index('target-unique-ids=1')] = f'target-unique-ids={self.target_unique_ids}'

    def write(self, save_path):
        with open(save_path, "w", encoding="utf-8") as handle:
            handle.write("\n".join(self.config))

if __name__ == "__main__":
    nvsahipreprocess_generator = NvsahipreprocessGenerator(
        network_input_shape="16;3;640;640",
        target_unique_ids=1,
        tensor_data_type=0,
        tensor_name="images",
    )
    print(nvsahipreprocess_generator.config)

