# DeepStream patches

构建镜像时由 `Dockerfile` 自动 `patch -p0` 应用。改补丁后需重新跑 `scripts/1_build_dev_image.sh`。

| 补丁 | 作用 |
|------|------|
| `gstnvinfer-interval-tensor-input.patch` | `nvdspreprocess` / `nvdspostprocess` 启用时，`pgie` 的 `interval` 会被忽略；补丁让跳帧批次直接 `GST_FLOW_OK`，不跑推理。 |
| `deepstream-yolo-rotation-angle-init.patch` | DS 9.0 新增 `rotation_angle`；DeepStream-Yolo AABB parser 未初始化会导致 OSD 画斜框。补丁将其置 `0`。仅影响标准 YOLO 检测 parser，不影响 OBB parser。 |
