#include "subelement_generator/utils/default_gie/yolo_pose.hpp"

YAML::Node default_gie::yoloPose() {
    YAML::Node config;
    config["property"]["gpu-id"] = 0;
    config["property"]["onnx-file"] = "";
    config["property"]["model-engine-file"] = "";
    config["property"]["labelfile-path"] = "";
    config["property"]["batch-size"] = 1;
    config["property"]["network-mode"] = 2;
    config["property"]["network-type"] = 3;
    config["property"]["process-mode"] = 1;
    config["property"]["model-color-format"] = 0;
    config["property"]["num-detected-classes"] = 1;
    config["property"]["gie-unique-id"] = 1;
    config["property"]["interval"] = 0;
    config["property"]["cluster-mode"] = 4;
    config["property"]["maintain-aspect-ratio"] = 1;
    config["property"]["symmetric-padding"] = 1;
    config["property"]["force-implicit-batch-dim"] = 0;
    config["property"]["scaling-filter"] = 1;
    config["property"]["scaling-compute-hw"] = 0;
    config["property"]["net-scale-factor"] = 0.0039215697906911373;
    config["property"]["infer-dims"] = "3;640;640";
    config["property"]["custom-lib-path"] = "/opt/ai_stream2/servers/deepstream/libs/libnvds_infer_yolo_pose.so";
    config["property"]["parse-bbox-instance-mask-func-name"] = "NvDsInferYoloPose";
    config["property"]["output-blob-names"] = "output0";
    config["property"]["output-instance-mask"] = 1;
    config["class-attrs-all"]["topk"] = 300;
    config["class-attrs-all"]["pre-cluster-threshold"] = 0.25;
    return config;
}
