#include "subelement_generator/utils/default_gie/stgcnpp.hpp"

YAML::Node default_gie::stgcnpp() {
    YAML::Node config;
    config["property"]["gpu-id"] = 0;
    config["property"]["onnx-file"] = "";
    config["property"]["model-engine-file"] = "";
    config["property"]["labelfile-path"] = "";
    config["property"]["batch-size"] = 1;
    config["property"]["network-mode"] = 2;
    config["property"]["network-type"] = 1;
    config["property"]["process-mode"] = 2;
    config["property"]["gie-unique-id"] = 4;
    config["property"]["interval"] = 0;
    config["property"]["operate-on-gie-id"] = 1;
    config["property"]["operate-on-class-ids"] = 0;
    config["property"]["force-implicit-batch-dim"] = 0;
    config["property"]["output-blob-names"] = "output";
    config["property"]["input-tensor-from-meta"] = 1;
    config["property"]["output-tensor-meta"] = 0;
    config["property"]["classifier-threshold"] = 0.0;
    return config;
}
