# DeepStream 服务（实现说明）

本目录实现 **NVIDIA DeepStream 9.0** 侧的视频分析容器：动态 RTSP 源、推理与跟踪、可选 `nvdsanalytics`、Kafka 元数据输出、SmartRecord 录像、按需截图、多路拼接预览（经 UDP RTP 对接 MediaMTX），以及通过 Kafka 的运维命令通道。

> 本文档描述**当前仓库代码行为**。设计愿景或规划见项目根目录 `docs/plan-deepstream.md`，可能与实现细节不完全一致。

---

## 1. 职责边界

| 本容器负责 | 本容器不负责 |
|-----------|-------------|
| GStreamer / DeepStream 管道、GPU 推理与跟踪 | 用户认证、业务数据库、多租户逻辑 |
| `nvmultiurisrcbin` 内置 HTTP REST（增删流、健康检查） | 自建 Flask/FastAPI 替代内置 REST |
| 向 Kafka 推送检测/分析元数据；消费命令 Topic | 由 Django 后端承担编排与持久化（见项目架构规则） |

---

## 2. 技术栈

| 类别 | 选型 |
|------|------|
| 运行时 | Python 3.12（随 DeepStream 镜像） |
| 管道 API | **pyservicemaker**（DeepStream Service Maker，wheel 随镜像提供） |
| 消息 | **confluent-kafka**（Python 侧命令消费、截图完成事件） |
| 配置 | **PyYAML**（解析 PGIE 配置以决定是否挂载 YOLO 后处理） |
| 推理后处理 | **CuPy**（YOLOv10 张量解析，GPU） |
| 截图编码 | **Pillow**（RGB → JPEG）；帧数据经 **CuPy DLPack** 从 buffer 取出 |
| 显存监控 | **pynvml**（`GpuMemoryMonitor` 守护线程） |
| SmartRecord 控制 | 自建 C 扩展 **`nvdssr_ext`**（`ext/`，对子 `nvurisrcbin` 发 `start-sr`/`stop-sr`） |
| 辅助二进制 | 静态 **ffmpeg/ffprobe**（容器内）、**mediamtx**（预览 RTSP/WebRTC） |

---

## 3. 架构总览

```
                    ┌─────────────────────────────────────────┐
                    │  nvmultiurisrcbin (内置 REST :9000)      │
                    │  SmartRecord 缓冲目录 → RollingRecord    │
                    └─────────────────┬───────────────────────┘
                                      │
                    PGIE (nvinfer) 或 identity (轻量模式)
                                      │
                                   nvtracker 或 identity
                                      │
                         [可选] nvdsanalytics + AnalyticsMetadataProbe
                                      │
                                    tee
              ┌───────────────────────┼───────────────────────┐
              │                       │                       │
        queue_meta              queue_snap              queue_preview
              │                       │                       │
    nvmsgconv→nvmsgbroker      valve→…→appsink      tiler→nvosd→…→udpsink
    或 fakesink(轻量)         ScreenshotRetriever      (RTP → MediaMTX)
              │                       │
           Kafka                   截图 JPEG
     (deepstream-detections)      + Kafka 事件

并行线程/进程：
  · CommandConsumer ← Kafka deepstream-commands
  · RollingRecordManager：缓冲目录轮询归档（见 §5.4）
  · DiskGuard（存储配额/分区占用）
  · GpuMemoryMonitor
  · 子进程：MediaMTX（main.py 在管道 activate 后拉起）
```

**入口**：`main.py` 在 **子进程**（`multiprocessing.Process`）中执行 `run_pipeline()`，避免与 GStreamer 主循环/信号处理冲突；管道使用 `prepare` → `activate` → `wait()`。

---

## 4. 管道拓扑（`pipeline/builder.py`）

主链路：

`nvmultiurisrcbin` → `nvinfer`（**或** `DS_LIGHT_PIPELINE=1` 时为 `identity`）→ `nvtracker`（或 `identity`）→ **[可选]** `nvdsanalytics` → `tee`。

三个分支（每个 tee 出口均带 `queue`）：

1. **元数据 / Kafka**  
   - 完整模式：`queue_meta` → `nvmsgconv` → `nvmsgbroker`（`libnvds_kafka_proto.so`）。  
   - 若启用 `nvdsanalytics`：在 `analytics` 元素上挂载 `AnalyticsMetadataProbe`，便于 `msg2p-newapi` 序列化分析结果。  
   - **轻量模式**（`DS_LIGHT_PIPELINE=1`）：`queue_meta` → `fakesink`，不连真实 Kafka 插件。

2. **截图**  
   `queue_snap` → `valve`（默认 `drop=True`）→ `nvvideoconvert` → `capsfilter(RGB)` → `appsink` → `ScreenshotRetriever.consume()`。

3. **预览**  
   `queue_preview` → `nvmultistreamtiler` → `nvdsosd` → `nvvideoconvert` → `nvv4l2h264enc` → `rtph264pay` → `udpsink`（默认 `127.0.0.1:DS_PREVIEW_RTP_PORT`）。  
   MediaMTX 通过 `config/mediamtx.yml` 从 UDP/RTP 拉流并对外提供 RTSP（如 `/preview`）与 WebRTC。

**YOLOv10**：当 PGIE 配置中 `property.output-tensor-meta` 为真时，在 `pgie` 上挂载 `YoloV10Postprocessor`（CuPy 解析张量并写入对象元数据）。

---

## 5. 数据流

### 5.1 视频与动态源

- 上游通过 **内置 REST** 向 `nvmultiurisrcbin` 注册 RTSP/文件等 URI（端口由 `DS_REST_PORT` 控制，默认 9000）。  
- `main.py` 中 `MessageHandler` 处理 `DynamicSourceMessage`：维护 **`sensor_id`（字符串）↔ `source_id`（整数）** 映射，供 `PerfMonitor`、滚动录像、`CommandConsumer` 解析命令使用。  
- 流加入后：`RollingRecordManager.register_source` + `start_rolling`（按 `DS_RECORDING_SEGMENT_SEC` 分段 SmartRecord）。

### 5.2 Kafka：检测结果

- Topic：`KAFKA_TOPIC`（默认 `deepstream-detections`）。  
- Broker 地址环境变量为 `host:port`，在插件属性中需为 **`host;port`**（代码中 `replace(":", ";")`）。

### 5.3 Kafka：命令与事件

- **消费**：`KAFKA_COMMAND_TOPIC`（默认 `deepstream-commands`），`CommandConsumer` 轮询 JSON。  
- **生产**：截图完成、`command_error`、裁剪完成等发往 `KAFKA_EVENT_TOPIC`（默认 `deepstream-events`；`CommandConsumer` / `ScreenshotRetriever` 与构造参数中的 event topic 需与配置一致）。

**当前已实现的 `action`**（见 `daemons/command_consumer.py`）：

| action | 说明 |
|--------|------|
| `start_rolling` | 对解析后的 `source_id` 调用 `RollingRecordManager.start_rolling` |
| `stop_rolling` | `stop_rolling` |
| `start_recording` | 登记 **UTC 时间窗起点**：`request_id`（同一任务唯一）、`start_ts`（ISO8601）；与 `stop_recording` 成对 |
| `stop_recording` | `request_id`、`end_ts`（ISO8601，须晚于 `start_ts`）；线程池异步调用 `RollingClipExtractor.extract`：按 **墙钟**（`mtime - duration` 推断每段起止）在 `rolling/` 中选重叠片段，**一段**则 `ffmpeg` trim（`-c copy`），**多段**则逐段 trim 再 **`ffmpeg` concat** 写入 `locked/`；完成后向 `KAFKA_EVENT_TOPIC` 发 `clip_ready` / `clip_failed` |
| `screenshot` | `filename` + `source_id`（sensor_id 或整数）；写入 `StorageManager.screenshots_dir(camera_id)` |
| `switch_preview` | `tiler.set({"show-source": int})`，`-1` 多画面 |
| `toggle_osd` | `OsdToggle.set_overlay`，控制 `display-bbox/text/mask` |

`start_recording` / `stop_recording` **不是**再开一段 SmartRecord；仅表示 **登记绝对时间窗 + 从已有滚动归档裁切**。未识别的 `action` 仅打日志 **`Unknown command action`**。

**事件类型（发往 `KAFKA_EVENT_TOPIC`）**：

| event | 说明 |
|-------|------|
| `clip_ready` / `clip_failed` | 手动时间窗裁剪结束；字段含 `request_id`、`sensorId`、`clip_path`（成功，相对 `DS_STORAGE_DIR`）或 `reason`（失败） |
| `screenshot_done` | 截图写入完成（`pipeline/screenshot.py`） |
| `command_error` | 某命令执行失败（如截图 API 不可用） |

仅 **`rolling/`**（及旧版每路 `recordings/`）内归档片段参与 `RollingClipExtractor`；全局缓冲 **`recordings/`** 若尚未轮询归档到 `rolling/`，该墙钟区间可能缺失。

### 5.4 存储与录像路径

根目录由 **`DS_STORAGE_DIR`**（默认 `/app/storage`）决定，`StorageManager` 布局：

```
{DS_STORAGE_DIR}/
├── recordings/                 # SmartRecord 全局缓冲（管道配置的 smart-rec-dir-path）
└── {camera_id}/
    ├── rolling/                # 分段完成后由 RollingRecordManager 归档的 MP4（7×24 滚动，可被 DiskGuard 回收）
    ├── locked/                 # 由 start_recording/stop_recording 时间窗从 rolling 裁切的片段（DiskGuard 不删除）
    └── screenshots/            # 截图 JPEG
```

旧部署的 `{camera_id}/recordings/` 若仍存在，**DiskGuard** 仍会扫描并清理其中的 `.mp4`（与 `rolling/` 同类），迁移到新布局后该目录可逐步消失。

**滚动归档如何进入 `rolling/`**：

- C 扩展 **`nvdssr_ext`** 仅实现 `start-sr` / `stop-sr`；**GStreamer `sr-done` 信号当前未接入 Python**，因此 `on_recording_done` 不会从插件侧触发。
- 实际归档由 **`RollingRecordManager` 后台线程**轮询全局缓冲 `recordings/sr_<source_id>_*.mp4`：文件 **非空**、**mtime** 超过 `DS_BUFFER_ARCHIVE_MIN_AGE_SEC`（默认 45s）、且能解析 `source_id` 与 `camera_map` 时，调用与 `_on_sr_done` 相同的 **`shutil.move`** 到 `{camera_id}/rolling/`；滚动续录仍由同逻辑链起下一段 SmartRecord。
- **`_on_sr_done` 若发现 0 字节文件**：删除占位文件，**不**写入 `rolling/`（无有效媒体时常因 RTSP 未起流）。

- **运维容量**：`DiskGuard` 读取 **`DS_DISK_MAX_USAGE_PCT`**（分区使用百分比）与 **`DS_DISK_MAX_STORAGE_GB`**（**仅** `rolling/` 与旧 `recordings/` 下归档 MP4 总字节上限，不含 `locked/`；0 或空表示不启用字节上限）；周期 **`DS_DISK_CHECK_INTERVAL`**。  
- **全局缓冲 `recordings/`**：非空且 **mtime 超过 60s** 的 `.mp4` 按「陈旧」删除；**0 字节** 且 **mtime 超过 `DS_BUFFER_EMPTY_MAX_AGE_SEC`（默认 30s）** 的占位文件单独删除（避免无码流时长期残留）。

### 5.5 浏览器里看 YOLO 预览（WebRTC，推荐）

推理 + OSD 后的画面经 UDP/RTP 送进 MediaMTX（见 `config/mediamtx.yml` 中路径 **`preview`**）。在浏览器里用 **WebRTC** 观看（不要用 RTSP 地址直接粘到地址栏）：

- **本机**：在 Chrome / Edge 地址栏打开（**推荐带尾部斜杠**）：**`http://127.0.0.1:8889/preview/`**  
  无斜杠的 **`/preview`** 会 **301** 到 **`/preview/`**。与 [MediaMTX 文档「Web browsers + WebRTC」](https://mediamtx.org/docs/read/web-browsers) 一致：端口 **`8889`**，路径名与 `mediamtx.yml` 里 **`paths.preview`** 一致。  
  **说明**：对个别路径 **`HEAD` 请求可能返回 404**，但 **`GET`（浏览器正常打开页面）为 200** 并返回内置播放器 HTML，用 `curl -I` 误判属正常现象。

**前提**：管道在跑且预览分支在推流（`Active sources` > 0）；若页面 **404** 或黑屏，先确认摄像头 RTSP 已接入，且容器已映射 **`8889:8889`**。从局域网其他机器访问时，可能需在 `mediamtx.yml` 中配置 `webrtcAdditionalHosts` / ICE，使浏览器能连上正确 IP。

---

## 6. 目录结构（源码）

| 路径 | 作用 |
|------|------|
| `main.py` | 入口：存储、管道、滚动录像、PerfMonitor、EngineFileMonitor、CommandConsumer、DiskGuard、GPU 监控、MediaMTX 子进程、优雅退出 |
| `pipeline/builder.py` | 组装完整管道与各分支 |
| `pipeline/yolo_postprocessor.py` | YOLOv10 输出解析（CuPy） |
| `pipeline/analytics_probe.py` | nvdsanalytics 后探测，辅助元数据解析/日志 |
| `pipeline/screenshot.py` | `ScreenshotRetriever`（BufferRetriever） |
| `pipeline/osd_toggle.py` | 预览 OSD 开关 |
| `recording/manager.py` | `RollingRecordManager`：缓冲轮询 + `_on_sr_done`（`shutil.move` 至 `rolling/`） |
| `recording/clip_extractor.py` | `RollingClipExtractor`：墙钟窗口、多段 trim + concat |
| `recording/smartrecord.py` | `SmartRecordController` + `nvdssr_ext` |
| `daemons/command_consumer.py` | Kafka 命令 |
| `daemons/disk_guard.py` | 磁盘与容量守护 |
| `daemons/gpu_monitor.py` | 显存日志 |
| `utils/storage.py` | `StorageManager` |
| `config/` | `pgie_yolov10_config.yml`、`mediamtx.yml`、`msgconv_config.txt`、`kafka_broker_config.txt`、可选 `analytics_config.txt` |
| `models/coco_labels.txt` | 默认类别标签 |
| `ext/` | `nvdssr_ext` C 源码与 `setup.py` |
| `test/` | 单元测试、REST/Kafka 集成、`test_clip_extraction_e2e.py`（裁剪/拼接，需本机 ffmpeg） |
| `script/video2rtsp.py` | 开发/测试：用 ffmpeg 将本地 MP4 推到 MediaMTX RTSP |
| `script/start_local_demo.sh` | 一键：`compose up` kafka+deepstream、推流、`stream/add`（见 §9） |
| `script/stop_local_demo.sh` | 停止本机 `video2rtsp` 子进程 |
| `script/kafka_storage_e2e.sh` | Kafka 命令 + 存储目录校验（滚动/截图/手动裁剪） |
| `script/kafka_commands_misc_e2e.sh` | Kafka：`stop_rolling`/`start_rolling`、`switch_preview`、`toggle_osd`、孤立 `stop_recording` → `clip_failed`（见 §9） |
| `example_data/` | 示例视频与 JSON 测试数据 |

---

## 7. 容器与镜像

- **基础镜像**：`nvcr.io/nvidia/deepstream:9.0-triton-multiarch`  
- **构建**：`docker build -t <tag> ./deepstream`（上下文为 `deepstream/`）  
- **入口**：`ENTRYPOINT ["python3", "main.py"]`  
  - 若需在容器内仅运行 Python 脚本（例如自检），需 **`docker run --entrypoint python3 ...`**，否则会执行 `main.py` 并加载 CUDA/pyservicemaker。  
- **暴露端口**：`9000`（REST）、`8554` / `8889`（与 MediaMTX 配置一致，用于 RTSP/WebRTC）  
- **环境**：`NVIDIA_DRIVER_CAPABILITIES` 含 `video`；运行需要 **NVIDIA Container Toolkit** 与 GPU。  
- **镜像与仓库同步**：`Dockerfile` 使用 `COPY . /app`，**PGIE 等配置文件在构建时打进镜像**。若你更新了 `config/pgie_yolov10_config.yml` 或默认 `DS_PGIE_CONFIG` 指向的路径，需执行 **`docker compose build deepstream`（或 `docker build ./deepstream`）** 后再启动容器；仅改主机文件而不重建镜像时，容器内 `/app/config/` 仍可能是旧内容，易导致 nvinfer 报 **`bad file: ...pgie_....yml`**。

---

## 8. 环境变量参考

| 变量 | 默认 | 说明 |
|------|------|------|
| `DS_STORAGE_DIR` | `/app/storage` | 存储根目录 |
| `DS_RECORDING_SEGMENT_SEC` | `300` | 滚动录像每段时长（秒） |
| `DS_REST_PORT` | `9000` | 内置 REST |
| `DS_MAX_BATCH_SIZE` | `16` | 批大小（与 PerfMonitor、源配置一致） |
| `DS_PIPELINE_WIDTH` / `DS_PIPELINE_HEIGHT` | `1920` / `1080` | 管道处理分辨率 |
| `DS_PGIE_CONFIG` | `/app/config/pgie_yolov10_config.yml` | PGIE 配置 |
| `DS_TRACKER_CONFIG` | NvDCF 样本路径 | nvtracker `ll-config-file` |
| `DS_ANALYTICS_CONFIG` | 空 | 非空则启用 `nvdsanalytics` |
| `DS_LIGHT_PIPELINE` | `1` | `1`：PGIE/tracker 用 identity，Kafka 用 fakesink，便于无模型调试。**要做真实 YOLO 推理时须设为 `0`**（例如在 `docker-compose.yml` 或启动命令里覆盖，勿仅依赖默认 `1`）。 |
| `DS_PREVIEW_BITRATE` | `4000000` | 预览编码码率 |
| `DS_PREVIEW_TILER_ROWS` / `COLS` | `4` / `4` | 拼接行/列 |
| `DS_PREVIEW_RTP_PORT` | `5400` | UDP RTP 发往本机 MediaMTX |
| `DS_YOLO_THRESHOLD` | `0.3` | YOLO 后处理置信度 |
| `DS_YOLO_PERSON_ONLY` | `1` | 仅保留 person 类 |
| `DS_LABELS_PATH` | `/app/models/coco_labels.txt` | 标签文件 |
| `DS_SR_CACHE_SEC` | `30` | SmartRecord 缓存 |
| `DS_SR_DEFAULT_DURATION` | `20` | 默认片段时长相关 |
| `DS_MEDIAMTX_CONFIG` | `/app/config/mediamtx.yml` | MediaMTX 配置文件路径 |
| `KAFKA_BROKER` | `kafka:9092` | Kafka 地址 |
| `KAFKA_TOPIC` | `deepstream-detections` | 检测 topic |
| `KAFKA_EVENT_TOPIC` | `deepstream-events` | 事件 topic（截图等应与 `ScreenshotRetriever` 一致） |
| `KAFKA_COMMAND_TOPIC` | `deepstream-commands` | 命令 topic |
| `DS_DISK_MAX_USAGE_PCT` | `85` | 分区占用阈值 |
| `DS_DISK_MAX_STORAGE_GB` | 空 | 归档录像总大小上限（空=不限） |
| `DS_DISK_CHECK_INTERVAL` | `60` | DiskGuard 周期（秒） |
| `DS_BUFFER_ARCHIVE_POLL_SEC` | `10` | `RollingRecordManager` 扫描全局缓冲 `recordings/` 的间隔（秒） |
| `DS_BUFFER_ARCHIVE_MIN_AGE_SEC` | `45` | 缓冲中 `.mp4` 的 **mtime** 需超过该值才尝试归档（避免正在写入） |
| `DS_BUFFER_EMPTY_MAX_AGE_SEC` | `30` | **0 字节** 缓冲 `.mp4` 超过该 **mtime** 年龄则由 DiskGuard 删除 |

---

## 9. 开发与验证脚本

### `script/video2rtsp.py`

在 **已有 MediaMTX** 监听 RTSP 的前提下，用 **ffmpeg** 将本地文件以 **copy** 方式推送到 `rtsp://<host>:<port>/<stream_name>`。

- 参数：`--input video.mp4:stream_name ...`、`--loop`、`--mediamtx` 基地址、`--mode webrtc|hls`（webrtc 模式下会拒绝含 B 帧的输入）。  
- **推荐顺序（与 DeepStream 同机 / `compose` 已映射 `8554`）**：  
  1. 启动 DeepStream 容器（`main.py` 会拉起 MediaMTX，监听 RTSP）。  
  2. 在**主机**执行本脚本，例如 `--mediamtx rtsp://127.0.0.1:8554`，`--input example_data/video1_bf0.mp4:cam1 ...`。  
  3. 调用 REST `POST /api/v1/stream/add`，其中 **`camera_url` 使用 `rtsp://127.0.0.1:8554/<stream_name>`**（MediaMTX 与管道在同一容器内，该地址对 nvmultiurisrcbin 同样有效）。  

### `script/start_local_demo.sh` / `stop_local_demo.sh`

- **start**：`docker compose up` kafka + deepstream（默认 `DS_LIGHT_PIPELINE=0`、`DS_RECORDING_SEGMENT_SEC=30` 等）、等待 `ds-ready`、后台启动 `video2rtsp`、REST 注册两路示例流。  
- **stop**：结束本机 `video2rtsp`（不停止容器）。  

### `script/kafka_storage_e2e.sh`

向 `deepstream-commands` 发送 `start_rolling`、`screenshot`、`start_recording`/`stop_recording` 等，并检查 `storage/<camera_id>/` 下 `rolling/`、`screenshots/`、`locked/`；可选覆盖「双段 concat」路径（需足够等待或第二段 rolling 已归档）。在项目根执行：`./deepstream/script/kafka_storage_e2e.sh`。

### `script/kafka_commands_misc_e2e.sh`

不等待 rolling 落盘，专门覆盖 **`kafka_storage_e2e.sh` 未测的 Kafka 行为**：`stop_rolling` → `start_rolling`；`switch_preview`（先单路数值 `source_id`，再 `-1` 多画面）；`toggle_osd`（关/开）；以及**无对应 `start_recording` 的** `stop_recording`（期望 `clip_failed`，脚本通过 **`docker compose logs deepstream`** 匹配 `Published clip_failed request_id=…`，避免 `rpk consume` 读到历史事件误判）。前置条件与 `kafka_storage_e2e.sh` 相同（`kafka` + `deepstream` 已起、流已注册）。执行：`./deepstream/script/kafka_commands_misc_e2e.sh`，可选 `CAMERA_ID=demo_cam1`。

---

## 10. 测试

| 内容 | 位置 | 依赖 |
|------|------|------|
| 单元测试：`StorageManager`、`DiskGuard`、归档逻辑、解析辅助 | `test/test_unit.py` | 仅 Python，**无需** GPU 容器 |
| 裁剪/拼接（ffmpeg） | `test/test_clip_extraction_e2e.py` | 本机 `ffmpeg`/`ffprobe`，**无需** GPU |
| REST + Kafka 集成 | `test/test_deepstream_api.py` | 运行中的 DeepStream、可达的 Kafka、测试用 RTSP |
| 共享逻辑与 CLI | `test/conftest.py`、`test/_common.py` | — |

**单元测试（主机）**：

```bash
cd deepstream
python -m pytest test/test_unit.py --noconftest -v
```

**集成测试**：需指向真实服务，示例见 `test/README.md`（`--base-url`、`--kafka-broker`、`--camera-url` 等）。

---

## 11. REST API（内置）

由 `nvmultiurisrcbin` 提供，常见路径包括（与 NVIDIA 动态源示例一致）：

- `POST /api/v1/stream/add`
- `POST /api/v1/stream/remove`
- `GET /api/v1/stream/get-stream-info`
- `GET /api/v1/health/get-dsready-state`

请求/响应体格式需与 DeepStream 文档一致；后端通常通过 HTTP 代理调用，**不要**在本仓库重复实现流状态机。

---

## 12. 运维与排障提示

- **轻量管道**（`DS_LIGHT_PIPELINE=1`）：无真实检测 Kafka 输出，预览仍可能占用编码资源。  
- **全量 PGIE（`DS_LIGHT_PIPELINE=0`）首次启动**：若 PGIE 配置里 `model-engine-file` 尚不存在，nvinfer 会从 ONNX **在线构建 TensorRT 引擎**（日志中可见 `Trying to create engine from model files`）。此阶段可能持续 **数十秒至数分钟**，期间 **`get-dsready-state` 常为 `ds-ready: NO`**，GPU 会有负载；引擎生成后会写入挂载的 `models/`（如 `yolov10n.onnx_b1_gpu0_fp16.engine`），之后冷启动会快很多。  
- **tee 多分支**：各 sink 已设 `sync=0, async=0`（见 builder）；动态源场景下若管道卡住，可优先查 sink 异步设置。  
- **截图**：依赖 CuPy 与 Pillow；多路同时 pending 时 `consume` 需能区分 `source_id`。  
- **滚动归档**：以 **`recordings/` 缓冲轮询**为主（见 §5.4）；并非依赖 GStreamer `sr-done` 回调。若长期无 `rolling/` 新文件，查 **RTSP 是否真有码流**（`Active sources`、先推流再 `stream/add`）、以及缓冲文件是否 **0 字节**（无媒体）。  
- **分段时长**：`DS_RECORDING_SEGMENT_SEC` 为**目标**时长；实际 MP4 时长可能略短（如 ~28s），与 **关键帧/GOP** 对齐有关，属常见现象。  
- **MediaMTX**：与 DeepStream 同容器内 `127.0.0.1` 互通；跨容器时需改 `udpsink` 目标或网络别名。  
- **WebRTC 预览黑屏几秒**：管道与 ICE 建立需要时间；已确认 `stream-info` 有活跃源且 `ds-ready` 为 YES 后，稍等或刷新 `/preview/`。

---

## 13. 许可证与第三方

DeepStream、CUDA、GStreamer 等遵循 NVIDIA 及相关开源许可证；生产部署前请确认 NGC 镜像与驱动版本匹配。
