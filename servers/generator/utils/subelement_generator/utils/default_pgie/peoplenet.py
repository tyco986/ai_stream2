# DetectNet_v2 PeopleNet + ONNX EfficientNMS_TRT → output0 for NvDsInferYoloE2E

PeopleNet = {
    "property": {
        "gpu-id": 0,
        "onnx-file": "",
        "model-engine-file": "",
        "labelfile-path": "",
        "batch-size": 1,
        "network-mode": 2,
        "network-type": 0,
        "process-mode": 1,
        "model-color-format": 0,
        "num-detected-classes": 3,
        "gie-unique-id": 1,
        "interval": 0,
        "cluster-mode": 4,
        "maintain-aspect-ratio": 0,
        "net-scale-factor": 0.0039215697906911373,
        "offsets": "0.0;0.0;0.0",
        "infer-dims": "3;544;960",
        "custom-lib-path": "/opt/nvidia/deepstream/deepstream/lib/libnvds_infer_yolo.so",
        "parse-bbox-func-name": "NvDsInferYoloE2E",
        "output-blob-names": "output0",
    },
    "class-attrs-all": {
        "topk": 300,
        "pre-cluster-threshold": 0.25,
    },
}
