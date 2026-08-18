# DeepStream patches

构建镜像时由 `Dockerfile` 自动应用（`gstnvinfer` 为 `-p0`，sahi 为 `-p1`）。改补丁后需重新跑 `scripts/1_build_dev_image.sh`。

| 补丁 | 作用 |
|------|------|
| `gstnvinfer-interval-tensor-input.patch` | `interval` 在 `process_full_frame`（PGIE）里生效，但 `process_objects`（SGIE `process-mode: 2`）和 `process_tensor_input`（SAHI）都不读它。补丁让这两条路径同样按 `interval_counter % (interval + 1)` 跳帧，空 object list 时直接 `GST_FLOW_OK`。 |
| `gstnvsahipostprocess-latency-timestamp.patch` | 给 `nvsahipostprocess` 打 `nvds_set_{input,output}_system_timestamp`，使组件 latency 可被 `latency_probe` 采集为 `sahi_postprocess`。解压 deepstream-sahi 后、`install.sh` 前以 `-p1` 应用。 |
