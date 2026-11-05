# Patches

## fix_nvinfer_interval_ineffectiveness_caused_by_nvdspreprocess.patch

问题: nvdspreprocess（含基于 nvdspreprocess 构造的 nvsahipreprocess）被启用时会造成 nvinfer 的 interval 失效。即使 interval 为非 0 正数，nvdspreprocess + nvinfer 组合里 nvinfer 仍会逐帧推理。

修复: 在 nvinfer 消费 preprocess 张量的入口按 `interval_counter % (interval + 1)` 跳过本批，与主推理路径的跳帧条件一致。

## fix_sgie_global_interval.patch

问题: SGIE 配置文件尽管支持填写 interval，但并不会读取 interval。无论 SGIE 配置中的 interval 为任何值，SGIE 都会逐帧推理。

修复: 在 SGIE 批处理入口补上同一套 interval 计数与跳帧判断，SGIE 可以通过本补丁实现跳帧推理。

## deepstream_sahi/fix_rotation_angle_for_libnvds_infer_yolo.patch

问题: deepstream-sahi 发布的 custom parser `libnvds_infer_yolo.so`（源码 `libs/nvdsinfer_yolo/nvdsinfer_yolo.cpp`）里，`NvDsInferYoloE2E` 和 `NvDsInferYoloNMS` 在栈上创建 `NvDsInferObjectDetectionInfo` 时没有初始化 `rotation_angle`。nvinfer 把这块垃圾弧度转成 OSD 角度，框会随机旋转。

修复: 创建检测结果后显式把 `rotation_angle` 设为 `0`。

## deepstream_sahi/add_latency_to_nvsahipostprocess.patch

问题: deepstream-sahi 发布的 nvsahipostprocess 没有调用 DeepStream 的组件延迟接口，流水线的 CompLatency 里看不到这个 element 的延迟。

修复: 在 `transform_ip` 入口写入输入时间戳，在返回前写入输出时间戳。nvsahipostprocess 的延迟通过本补丁也可以显示在 CompLatency 当中。
