# ST-GCN++ classifier: skeleton tensor from nvdspreprocess (no RGB infer-dims)

Stgcnpp = {
    "property": {
        "gpu-id": 0,
        "onnx-file": "",
        "model-engine-file": "",
        "labelfile-path": "",
        "batch-size": 1,
        "network-mode": 2,
        "network-type": 1,
        "process-mode": 2,
        "gie-unique-id": 4,
        "interval": 0,
        "operate-on-gie-id": 1,
        "operate-on-class-ids": 0,
        "force-implicit-batch-dim": 0,
        "output-blob-names": "output",
        "input-tensor-from-meta": 1,
        "output-tensor-meta": 0,
        "classifier-threshold": 0.51,
        "custom-lib-path": "/opt/ai_stream2/servers/deepstream/libs/libnvds_stgcnpp_classifier_parse.so",
        "parse-classifier-func-name": "NvDsInferClassiferParseCustomStgcnpp",
    },
}
