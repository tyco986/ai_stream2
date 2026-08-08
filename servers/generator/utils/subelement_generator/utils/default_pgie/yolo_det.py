# Detect: images/output0 → deepstream-sahi NvDsInferYoloE2E (e2e / non-e2e / sahi 共用)

YoloDet = {
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
        "num-detected-classes": 80,
        "gie-unique-id": 1,
        "interval": 0,
        "cluster-mode": 4,
        "maintain-aspect-ratio": 1,
        "symmetric-padding": 1,
        "force-implicit-batch-dim": 0,
        "net-scale-factor": 0.0039215697906911373,
        "infer-dims": "3;640;640",
        "custom-lib-path": "/opt/nvidia/deepstream/deepstream/lib/libnvds_infer_yolo.so",
        "parse-bbox-func-name": "NvDsInferYoloE2E",
        "output-blob-names": "output0",
    },
    "class-attrs-all": {
        "topk": 300,
        "pre-cluster-threshold": 0.25,
    },
}
