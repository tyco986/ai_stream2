# Default nvinfer [class-attrs-all] parameters applied to all detection classes
class_attrs_all: dict[str, int | float] = {
    # Max detections per class kept after NMS
    "topk": 20,
    # IoU threshold for NMS; boxes with higher overlap are suppressed
    "nms-iou-threshold": 0.3,
    # Confidence threshold before clustering/NMS; lower scores are discarded
    "pre-cluster-threshold": 0.4,
    # DBSCAN neighborhood radius (epsilon); used when cluster-mode is DBSCAN
    "eps": 0.0,
    # Minimum cluster confidence to keep after DBSCAN
    "dbscan-min-score": 0.0,
    # Minimum boxes required to form a valid DBSCAN cluster (camelCase, not min-boxes)
    "minBoxes": 0,
    # ROI top offset in pixels; crops the valid detection region
    "roi-top-offset": 0,
    # ROI bottom offset in pixels; crops the valid detection region
    "roi-bottom-offset": 0,
    # Minimum detection box width in pixels; smaller boxes are filtered out
    "detected-min-w": 0,
    # Minimum detection box height in pixels; smaller boxes are filtered out
    "detected-min-h": 0,
    # Maximum detection box width in pixels; larger boxes are filtered out
    "detected-max-w": 2_147_483_647,
    # Maximum detection box height in pixels; larger boxes are filtered out
    "detected-max-h": 2_147_483_647,
}

YoloDet = {
    "property": {
        "gpu-id": 0,
        "onnx-file": "",
        "model-engine-file": "",
        "labelfile-path": "",
        "batch-size": 1,
        "network-mode": 2,
        "process-mode": 1,
        "model-color-format": 0,
        "num-detected-classes": 80,
        "filter-out-class-ids": "1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20;21;22;23;24;25;26;27;28;29;30;31;32;33;34;35;36;37;38;39;40;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79",
        "gie-unique-id": 1,
        "interval": 0,
        "cluster-mode": 4,
        "maintain-aspect-ratio": 0,
        "symmetric-padding": 0,
        "net-scale-factor": 0.00392156862745098,
        "infer-dims": "3;640;640",
        "custom-lib-path": "/app/libs/libnvdsinfer_custom_impl_Yolo.so",
        "parse-bbox-func-name": "NvDsInferParseYolo",
    },
    "class-attrs-all": {
        "topk": 300,
        "pre-cluster-threshold": 0.25,
    },
}

YoloDetSahi = {
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
        "cluster-mode": 2,
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
        "pre-cluster-threshold": 0.25,
    },
}

YoloSegSahi = {
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
        "num-detected-classes": 80,
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
        "custom-lib-path": "/opt/nvidia/deepstream/deepstream/lib/libnvds_infer_yolo.so",
        "parse-bbox-instance-mask-func-name": "NvDsInferYoloMask",
        "output-blob-names": "num_dets;det_boxes;det_scores;det_classes;det_masks",
        "output-instance-mask": 1,
        "segmentation-threshold": 0.5,
    },
    "class-attrs-all": {
        "pre-cluster-threshold": 0.25,
    },
}

YoloSeg = {
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
        "num-detected-classes": 80,
        "filter-out-class-ids": "1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20;21;22;23;24;25;26;27;28;29;30;31;32;33;34;35;36;37;38;39;40;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79",
        "gie-unique-id": 1,
        "interval": 0,
        "cluster-mode": 4,
        "maintain-aspect-ratio": 1,
        "symmetric-padding": 1,
        "scaling-filter": 1,
        "scaling-compute-hw": 0,
        "net-scale-factor": 0.00392156862745098,
        "infer-dims": "3;640;640",
        "custom-lib-path": "/app/libs/libnvdsinfer_custom_impl_Yolo_seg.so",
        "parse-bbox-instance-mask-func-name": "NvDsInferParseYoloSeg",
        "output-instance-mask": 1,
        "segmentation-threshold": 0.5,
    },
    "class-attrs-all": {
        "topk": 300,
        "pre-cluster-threshold": 0.25,
    },
}

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
        "scaling-filter": 1,
        "scaling-compute-hw": 0,
        "net-scale-factor": 0.00392156862745098,
        "infer-dims": "3;640;640",
        "custom-lib-path": "/app/libs/libnvdsinfer_custom_impl_Yolo_pose.so",
        "parse-bbox-instance-mask-func-name": "NvDsInferParseYoloPose",
        "output-instance-mask": 1,
    },
    "class-attrs-all": {
        "topk": 300,
        "pre-cluster-threshold": 0.25,
    },
}
