# Pose: E2E output0 → NvDsInferYoloPose（kpt 挂在 mask_params Kx3）

YoloPose = {
    "property": {
        "gpu-id": 0,
        "onnx-file": "",
        "model-engine-file": "",
        "labelfile-path": "",
        "batch-size": 1,
        "network-mode": 2,
        "network-type": 3,
        "process-mode": 1,
        "model-color-format": 0,
        "num-detected-classes": 1,
        "gie-unique-id": 1,
        "interval": 0,
        "cluster-mode": 4,
        "maintain-aspect-ratio": 1,
        "symmetric-padding": 1,
        "force-implicit-batch-dim": 0,
        "scaling-filter": 1,
        "scaling-compute-hw": 0,
        "net-scale-factor": 0.0039215697906911373,
        "infer-dims": "3;640;640",
        "custom-lib-path": "/opt/ai_stream2/servers/deepstream/libs/libnvds_infer_yolo_pose.so",
        "parse-bbox-instance-mask-func-name": "NvDsInferYoloPose",
        "output-blob-names": "output0",
        "output-instance-mask": 1,
    },
    "class-attrs-all": {
        "topk": 300,
        "pre-cluster-threshold": 0.25,
    },
}
