#include "subelement_generator/utils/default_gie/rtmpose.hpp"

YAML::Node default_gie::rtmPose() {
    YAML::Node config;
    config["property"]["gpu-id"] = 0;
    config["property"]["onnx-file"] = "";
    config["property"]["model-engine-file"] = "";
    config["property"]["labelfile-path"] = "";
    config["property"]["batch-size"] = 1;
    config["property"]["network-mode"] = 2;
    config["property"]["network-type"] = 100;
    config["property"]["process-mode"] = 2;
    config["property"]["model-color-format"] = 0;
    config["property"]["gie-unique-id"] = 2;
    config["property"]["interval"] = 0;
    config["property"]["operate-on-gie-id"] = 1;
    config["property"]["operate-on-class-ids"] = 0;
    config["property"]["maintain-aspect-ratio"] = 1;
    config["property"]["symmetric-padding"] = 1;
    config["property"]["force-implicit-batch-dim"] = 0;
    config["property"]["net-scale-factor"] = 0.017124753831663668;
    config["property"]["offsets"] = "123.675;116.28;103.53";
    config["property"]["infer-dims"] = "3;256;192";
    config["property"]["output-blob-names"] = "keypoints";
    config["property"]["output-tensor-meta"] = 1;
    config["property"]["input-object-min-width"] = 32;
    config["property"]["input-object-min-height"] = 32;
    return config;
}
