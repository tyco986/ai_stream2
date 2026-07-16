# Pose RTSP 已知问题

相对 `pose_video`（probe 挂在 `pgie`），`pose_rtsp` 的 probe 挂在 **demux 之后** 的分支上，曾踩到下面三个问题。

## 1. demux 后 `mask_params` 悬空

- **现象**：读 `mask.threshold` / `mask_array` 立刻 segfault（exit 139）
- **原因**：pose 把关键点塞进 instance-mask 缓冲（`mask_size` 与 `W×H` 不一致）；`nvstreamdemux` 后 wrapper 失效。video 无 demux，故正常。
- **处理**：demux 前在 `pgie` 挂 `PoseKptsCacheProbe` 解析并缓存 kpts；demux 后的 `PoseRTSPProbe` 只读缓存，不再碰 `mask_params`。

## 2. demux 后 `pipeline_width/height == 0`

- **现象**：`pose_frame_transform` 除零（`ZeroDivisionError` → abort）
- **原因**：demux 后 `frame_meta.pipeline_width/height` 为 0；`source_width/height` 仍有效（如 1920×1080）。
- **处理**：画关键点时用 `pipeline_* or source_*`。

## 3. `display_meta` 耗尽导致 segfault

- **现象**：能跑几十帧后 segfault；去掉关键点绘制则稳定。
- **原因**：`next_display_meta` 在 `element_count` 达上限后未归零，且用 `meta in list` 判断 pybind 对象不可靠，重复 `acquire`/`append` 耗尽 pool。
- **处理**：acquire 新 meta 时重置 count，且仅在 acquire 时加入 `display_metas`。
