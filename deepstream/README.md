# DeepStream 服务（实现说明）

本目录实现 **NVIDIA DeepStream 9.0** 侧的视频分析容器：动态 RTSP 源、推理与跟踪、可选 `nvdsanalytics`、Kafka 元数据输出、多路拼接预览（经 **UDP RTP** 发往**外部** MediaMTX），以及 Kafka 预览/OSD 命令。

> **MediaMTX、ffmpeg、录像、截图** 不在本镜像内；请使用 `scripts/demo/1_mediamtx/` 等独立容器。

---

## 职责

| 负责 | 不负责 |
|------|--------|
| DeepStream 管道、GPU 推理、Kafka 检测 | MediaMTX / ffmpeg 二进制、RTSP/WebRTC 服务端口 |
| 内置 REST 增删流 | 录像、截图、本地 storage |
| 预览 H.264 → **UDP** `DS_PREVIEW_RTP_HOST:DS_PREVIEW_RTP_PORT` | 在容器内启动 mediamtx |

---

## 管道拓扑

```
nvmultiurisrcbin → nvinfer (或 identity) → nvtracker → [nvdsanalytics] → tee
    ├─ queue_meta   → nvmsgconv → nvmsgbroker → Kafka
    └─ queue_preview → tiler → nvosd → encode → udpsink → 外部 MediaMTX :5400/udp
```

---

## 与外部 MediaMTX 联调

1. 启动独立 MediaMTX（示例：`bash deepstream/scripts/demo/1_mediamtx/1_build_container.sh`）。
2. DeepStream 设置：
   - `DS_PREVIEW_RTP_HOST=ai_stream2_mediamtx`（同 Docker 网络下的容器名）
   - `DS_PREVIEW_RTP_PORT=5400`（与 `mediamtx/mediamtx.yml` 中 `preview` path 的 RTP 端口一致）
3. 浏览器预览走 MediaMTX：`http://<mtx-host>:8889/preview/`（不在 DeepStream 容器上）。

摄像头 ingress 建议：`RTSP → MediaMTX → rtsp://mtx/... → DeepStream REST add_source`。

---

## Kafka 命令

| action | 说明 |
|--------|------|
| `switch_preview` | `source_id` 整数 |
| `toggle_osd` | `show` 布尔 |

---

## 环境变量（常用）

| 变量 | 默认 | 说明 |
|------|------|------|
| `DS_REST_PORT` | `9000` | 内置 REST |
| `DS_PREVIEW_RTP_HOST` | `127.0.0.1` | 预览 UDP 目标（compose 中改为 MediaMTX 服务名） |
| `DS_PREVIEW_RTP_PORT` | `5400` | 预览 UDP 端口 |
| `DS_LIGHT_PIPELINE` | `1` | `0` 启用 YOLO |
| `KAFKA_BROKER` | `kafka:9092` | Kafka |

---

## 镜像构建

`Dockerfile` **不**包含 `mediamtx`、`ffmpeg` 安装包；仓库中的 `*.tar.gz` 由 `.dockerignore` 排除，仅供 demo 脚本使用。

```bash
docker build -t ai_stream2_deepstream ./deepstream
```

仅暴露 **9000**（REST）。

---

## 本地开发

```bash
# 先起 MediaMTX + Kafka，再起 DeepStream
bash deepstream/scripts/demo/1_mediamtx/1_build_container.sh
DS_PREVIEW_RTP_HOST=ai_stream2_mediamtx RTSP1=rtsp://ai_stream2_mediamtx:8554/cam1 \
  bash deepstream/scripts/start_local_demo.sh
```

详见 `scripts/README.md`。
