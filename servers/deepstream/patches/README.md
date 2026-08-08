# DeepStream patches

构建镜像时由 `Dockerfile` 自动应用（`gstnvinfer` 为 `-p0`，sahi 为 `-p1`）。改补丁后需重新跑 `scripts/1_build_dev_image.sh`。

| 补丁 | 作用 |
|------|------|
| `gstnvinfer-interval-tensor-input.patch` | `nvdspreprocess` / `nvdspostprocess` 启用时，`pgie` 的 `interval` 会被忽略；补丁让跳帧批次直接 `GST_FLOW_OK`，不跑推理。 |
| `gstnvsahipostprocess-latency-timestamp.patch` | 给 `nvsahipostprocess` 打 `nvds_set_{input,output}_system_timestamp`，使组件 latency 可被 `latency_probe` 采集为 `sahi_postprocess`。解压 deepstream-sahi 后、`install.sh` 前以 `-p1` 应用。 |
