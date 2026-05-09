# DeepStream Script Guide

本目录包含本地 DeepStream demo 启停、RTSP 测试流发布、Kafka 命令冒烟测试和存储链路 E2E 测试脚本。

除 `video2rtsp.py` 外，Shell 脚本都可以从任意目录运行，脚本内部会自动切换到项目根目录。

## 推荐运行顺序

### 1. 启动本地 demo 环境

```bash
bash deepstream/script/start_local_demo.sh
```

如果 DeepStream 代码或镜像内容刚修改过，建议重建：

```bash
REBUILD=1 bash deepstream/script/start_local_demo.sh
```

启动成功后会得到：

- Kafka 容器运行
- DeepStream 容器运行
- DeepStream REST API ready
- 容器内 `video2rtsp.py` 发布两路本地 RTSP 流
- 两路 demo camera 通过 REST 加入 DeepStream pipeline
- 默认不会自动启动 rolling；需要 rolling 时由 Kafka 命令或 E2E 脚本显式开启

### 2. 运行存储链路 E2E

```bash
bash deepstream/script/kafka_storage_e2e.sh
```

也可以指定摄像头：

```bash
CAMERA_ID=demo_cam2 bash deepstream/script/kafka_storage_e2e.sh
```

该脚本会验证 rolling MP4、截图、单分片 clip、跨分片 concat clip。

### 3. 运行控制命令冒烟测试

```bash
bash deepstream/script/kafka_commands_misc_e2e.sh
```

也可以指定摄像头：

```bash
CAMERA_ID=demo_cam2 bash deepstream/script/kafka_commands_misc_e2e.sh
```

该脚本会验证 rolling 开关、预览切换、OSD 开关和异常 `stop_recording` 的 `clip_failed` 事件。

### 4. 停止本地 demo 服务

```bash
bash deepstream/script/stop_local_demo.sh
```

该脚本会停止 `deepstream` 和 `kafka` 容器，并清理 stale `video2rtsp.py` PID 文件。

## 脚本说明

## `start_local_demo.sh`

一键启动本地 DeepStream demo 环境。

### 功能

- 启动 `kafka` 和 `deepstream` Docker Compose 服务。
- 默认设置 `DS_LIGHT_PIPELINE=0`，启用完整 YOLOv10 推理 pipeline。
- 默认设置 `DS_RECORDING_SEGMENT_SEC=30`，让后续 Kafka E2E 显式开启 rolling 时使用短分片。
- 等待 DeepStream REST health 返回 `ds-ready=YES`。
- 调用 `stop_local_demo.sh` 清理旧的 `video2rtsp.py` 进程。
- 在 DeepStream 容器内启动 `video2rtsp.py`，把本地 MP4 文件发布为 RTSP 测试流。
- 调用 DeepStream REST API，先 remove 再 add 两路 demo camera。
- 只负责注册 demo camera，不负责启动 rolling。

### 默认输入流

- `deepstream/example_data/video1_bf0.mp4` -> `rtsp://127.0.0.1:8554/cam1` -> `demo_cam1`
- `deepstream/example_data/video2_bf0.mp4` -> `rtsp://127.0.0.1:8554/cam2` -> `demo_cam2`

### 运行方式

```bash
bash deepstream/script/start_local_demo.sh
```

重建 DeepStream 镜像后启动：

```bash
REBUILD=1 bash deepstream/script/start_local_demo.sh
```

自定义 demo 视频文件名：

```bash
VIDEO1=deepstream/example_data/custom1.mp4 \
VIDEO2=deepstream/example_data/custom2.mp4 \
bash deepstream/script/start_local_demo.sh
```

注意：`video2rtsp.py` 在 DeepStream 容器内运行，`start_local_demo.sh` 会把 `VIDEO1` / `VIDEO2` 转成 `/app/example_data/<文件名>`。因此自定义视频需要已经存在于镜像或容器的 `/app/example_data/` 下；如果只是宿主机任意路径，脚本不会自动挂载进去。

自定义 camera id：

```bash
CAMERA_ID1=cam_a CAMERA_ID2=cam_b bash deepstream/script/start_local_demo.sh
```

### 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `DS_LIGHT_PIPELINE` | `0` | 是否使用轻量 pipeline。`0` 表示完整 demo pipeline。 |
| `DS_RECORDING_SEGMENT_SEC` | `30` | rolling 录像目标分片时长，单位秒。`start_local_demo.sh` 不会启动 rolling，该值供后续 Kafka 命令显式开启 rolling 时使用。 |
| `REBUILD` | `0` | 设为 `1` 时先执行 `docker compose build deepstream`。 |
| `DS_REST` | `http://127.0.0.1:9000` | DeepStream REST API 地址。 |
| `MEDIAMTX_RTSP_BASE` | `rtsp://127.0.0.1:8554` | MediaMTX RTSP 基础地址。 |
| `READY_TIMEOUT_SEC` | `600` | 等待 DeepStream ready 的最大秒数。 |
| `READY_POLL_SEC` | `5` | DeepStream ready 轮询间隔。 |
| `VIDEO1` | `deepstream/example_data/video1_bf0.mp4` | 第一条测试视频。脚本只使用文件名，并在容器 `/app/example_data/` 下读取。 |
| `VIDEO2` | `deepstream/example_data/video2_bf0.mp4` | 第二条测试视频。脚本只使用文件名，并在容器 `/app/example_data/` 下读取。 |
| `STREAM1` | `cam1` | 第一条 RTSP stream 名称。 |
| `STREAM2` | `cam2` | 第二条 RTSP stream 名称。 |
| `CAMERA_ID1` | `demo_cam1` | 第一条 camera id。 |
| `CAMERA_ID2` | `demo_cam2` | 第二条 camera id。 |
| `CAMERA_NAME1` | `Demo Cam 1` | 第一条 camera name。 |
| `CAMERA_NAME2` | `Demo Cam 2` | 第二条 camera name。 |
| `DEEPSTREAM_SERVICE` | `deepstream` | Docker Compose 中的 DeepStream 服务名。 |
| `PUBLISH_WAIT_SEC` | `5` | 启动 RTSP 发布后等待流可用的秒数。 |

### 输出与产物

- WebRTC preview: `http://127.0.0.1:8889/preview/`
- DeepStream REST: `http://127.0.0.1:9000`
- RTSP 发布日志: `deepstream/storage/video2rtsp.log`
- RTSP 发布 PID 文件: `deepstream/storage/.video2rtsp.pid`

## `stop_local_demo.sh`

停止 `start_local_demo.sh` 启动的本地 demo 服务。

### 功能

- 自动切换到项目根目录，因此可以从任意目录运行。
- 执行 `docker compose stop deepstream kafka`。
- 停止 DeepStream 容器时，容器内的 `video2rtsp.py` 也会随容器一起停止。
- 删除 `deepstream/storage/.video2rtsp.pid`，避免下次启动读到 stale PID。
- 不删除 `deepstream/storage/` 下的 rolling、screenshots、locked 等产物文件。

### 运行方式

```bash
bash deepstream/script/stop_local_demo.sh
```

### 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `DEEPSTREAM_SERVICE` | `deepstream` | Docker Compose 中的 DeepStream 服务名。 |

### 重要说明

该脚本会停止整个 DeepStream demo 运行环境，但不会删除 Docker 容器、镜像、volume 或 storage 产物。

等价核心命令：

```bash
docker compose stop deepstream kafka
```

## `kafka_storage_e2e.sh`

通过 Kafka 命令验证 DeepStream 存储链路。

### 功能

- 检查 DeepStream 是否 ready。
- 检查目标 camera 是否已经在 DeepStream pipeline 中。
- 发送 `start_rolling`，等待 rolling MP4 生成。
- 发送 `screenshot`，等待截图文件生成。
- 发送 `start_recording` / `stop_recording`，验证单个 rolling 分片内的手动 clip 裁剪。
- 等待第二个 rolling MP4，验证跨两个分片的手动 clip concat。
- 脚本退出时发送 `stop_rolling`，避免测试结束后继续写 rolling MP4。
- 最后列出 `rolling/`、`screenshots/`、`locked/` 目录内容。

### 前置条件

- `kafka` 和 `deepstream` 容器已启动。
- DeepStream REST `ds-ready=YES`。
- 目标 camera 已经通过 REST `stream/add` 加入 pipeline。
- 宿主机可用命令：`docker`、`python3`、`ffprobe`。
- 推荐先运行 `start_local_demo.sh`，它会默认设置 `DS_RECORDING_SEGMENT_SEC=30`。

### 运行方式

```bash
bash deepstream/script/kafka_storage_e2e.sh
```

指定 camera：

```bash
CAMERA_ID=demo_cam2 bash deepstream/script/kafka_storage_e2e.sh
```

指定更长等待时间：

```bash
WAIT_SECOND_SEGMENT_SEC=240 bash deepstream/script/kafka_storage_e2e.sh
```

### 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `CAMERA_ID` | `demo_cam1` | 要测试的 camera id。 |
| `STORAGE` | `deepstream/storage` | 宿主机上的 DeepStream storage 目录。 |
| `KAFKA_TOPIC` | `deepstream-commands` | Kafka 控制命令 topic。 |
| `WAIT_FIRST_SEGMENT_SEC` | `90` | 等待第一个 rolling MP4 的最大秒数。 |
| `WAIT_SECOND_SEGMENT_SEC` | `150` | 等待第二个 rolling MP4 的最大秒数。 |
| `WAIT_CLIP_SEC` | `180` | 等待手动 clip 生成的最大秒数。 |

### 产物目录

以 `CAMERA_ID=demo_cam1` 为例：

- Rolling 录像: `deepstream/storage/demo_cam1/rolling/`
- 截图: `deepstream/storage/demo_cam1/screenshots/`
- 手动 clip: `deepstream/storage/demo_cam1/locked/`

### 结果判断

看到以下输出表示完整通过：

```text
Manual clip (concat) OK: ...
All requested checks completed. Review directories above.
```

如果第二个 rolling MP4 超时，脚本会跳过 concat 测试并给出 warning。这通常表示 rolling 续段未生成、分片时长过长，或 DeepStream 容器不是用短分片参数创建的。

## `kafka_commands_misc_e2e.sh`

通过 Kafka 命令验证非存储类控制命令，以及异常 clip 场景。

### 功能

- 检查 DeepStream 是否 ready。
- 检查目标 camera 是否已经在 DeepStream pipeline 中。
- 从 `get-stream-info` 中解析 camera 对应的数字 `source_id`。
- 发送 `stop_rolling`，再发送 `start_rolling`。
- 发送 `switch_preview`，先切换到单路预览，再切回 mosaic。
- 发送 `toggle_osd`，关闭再打开 OSD。
- 发送没有对应 `start_recording` 的 `stop_recording`。
- 检查 DeepStream 日志中是否出现 `Published clip_failed request_id=...`。
- 脚本退出时发送 `stop_rolling`，避免测试结束后继续写 rolling MP4。

### 前置条件

- 与 `kafka_storage_e2e.sh` 相同：Kafka + DeepStream 已启动，目标 camera 已加入 pipeline。
- 宿主机可用命令：`docker`、`python3`、`curl`。

### 运行方式

```bash
bash deepstream/script/kafka_commands_misc_e2e.sh
```

指定 camera：

```bash
CAMERA_ID=demo_cam2 bash deepstream/script/kafka_commands_misc_e2e.sh
```

### 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `CAMERA_ID` | `demo_cam1` | 要测试的 camera id。 |
| `KAFKA_TOPIC` | `deepstream-commands` | Kafka 控制命令 topic。 |

### 结果判断

看到以下输出表示通过：

```text
clip_failed log line OK
=== kafka_commands_misc_e2e: all checks passed ===
```

该脚本不等待 rolling MP4 或 clip 文件落盘。存储产物请使用 `kafka_storage_e2e.sh` 验证。

## `video2rtsp.py`

把一个或多个本地视频文件通过 FFmpeg 推送为 RTSP 流。`start_local_demo.sh` 默认在 DeepStream 容器内调用它。

### 功能

- 接收一个或多个 `video_path:stream_name` 输入。
- 每个输入启动一个 FFmpeg 子进程。
- 使用 `-re -c copy -f rtsp -rtsp_transport tcp` 推送到 MediaMTX。
- 可选循环播放。
- 在 `webrtc` 模式下检查视频是否包含 B-frames；如果包含则拒绝发布。

### 运行方式

直接运行：

```bash
python3 deepstream/script/video2rtsp.py \
  --input deepstream/example_data/video1_bf0.mp4:cam1 \
  --loop \
  --mediamtx rtsp://127.0.0.1:8554 \
  --mode webrtc
```

发布多路：

```bash
python3 deepstream/script/video2rtsp.py \
  --input deepstream/example_data/video1_bf0.mp4:cam1 deepstream/example_data/video2_bf0.mp4:cam2 \
  --loop \
  --mediamtx rtsp://127.0.0.1:8554 \
  --mode webrtc
```

### 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `--input` | 必填 | 一个或多个 `video_path:stream_name`。 |
| `--loop` | 关闭 | 循环播放视频。 |
| `--mediamtx` | `rtsp://127.0.0.1:8554` | MediaMTX RTSP 基础地址。 |
| `--mode` | `webrtc` | 可选 `webrtc` 或 `hls`。`webrtc` 模式会拒绝包含 B-frames 的视频。 |

### 注意事项

- 需要 `ffmpeg` 和 `ffprobe`。
- `webrtc` 模式下使用不含 B-frames 的测试视频，例如 `video1_bf0.mp4`。
- 该脚本只负责推流，不负责向 DeepStream REST 添加 camera。添加 camera 由 `start_local_demo.sh` 完成。

## 常见工作流

### 完整本地验证

```bash
REBUILD=1 bash deepstream/script/start_local_demo.sh
bash deepstream/script/kafka_storage_e2e.sh
bash deepstream/script/kafka_commands_misc_e2e.sh
```

### 验证第二路 camera

```bash
bash deepstream/script/start_local_demo.sh
CAMERA_ID=demo_cam2 bash deepstream/script/kafka_storage_e2e.sh
CAMERA_ID=demo_cam2 bash deepstream/script/kafka_commands_misc_e2e.sh
```

### 清理本地 demo

```bash
bash deepstream/script/stop_local_demo.sh
docker compose stop deepstream kafka
```

