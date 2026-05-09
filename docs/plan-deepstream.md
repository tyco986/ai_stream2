# DeepStream 端 — AI Agent 实现计划（与参考实现对齐）

> **文档定位**：供 **从零实现** DeepStream 推理容器的 AI Agent 使用的**规范、约束与检查清单**。  
> **参考实现**：本仓库 `deepstream/` 目录（实现细节以该目录代码为准）。  
> **与 `deepstream/README.md` 的关系**：README 偏运维与排障；本文偏**架构、契约与实现技巧**，篇幅更长。  
> **标注**：文中 **`[技巧]`** 表示参考实现中的关键 workaround，新实现须复现或显式替代。

---

## 0. 使用本文档的约定

1. **必须遵守**：职责边界、管道启动方式（`prepare`/`activate`/`wait`）、`async=0` 的 sink、Kafka `conn-str` 分号格式、存储目录语义、命令 Topic JSON 契约（见 §15–§18）。  
2. **禁止**：在 DeepStream 容器内自建业务数据库、用户体系；用 Flask/FastAPI **替代** `nvmultiurisrcbin` 内置 REST。  
3. **可选扩展**：空帧过滤（§14）、GstRtspServer 额外封装（参考实现**未使用**）、后端 `PipelineDeployer` 生成 analytics 配置的自动化（本文只描述 DeepStream 侧消费方式）。  
4. **实现顺序建议**：`StorageManager` → `PipelineBuilder`（含轻量模式）→ `RollingRecordManager` + `SmartRecordController` + 缓冲轮询 **`[技巧]`** → `CommandConsumer` + `RollingClipExtractor` → `ScreenshotRetriever` → `DiskGuard` → `main.py` 进程模型 → MediaMTX 子进程 → Dockerfile（含 `nvdssr_ext` 编译）。

---

## 1. 定位与职责

DeepStream 容器是 **纯推理与媒体处理引擎**，负责：

| 负责 | 不负责 |
|------|--------|
| GStreamer / DeepStream 管道、GPU 推理、跟踪、可选 `nvdsanalytics` | 用户认证、多租户、业务数据库 |
| `nvmultiurisrcbin` **内置 HTTP REST**（默认 `:9000`）增删流、健康检查 | 自建独立 HTTP 服务替代内置 REST |
| 向 Kafka 推送检测（及分析）元数据；消费命令 Topic | Django 编排、检测结果持久化（由 Backend 完成） |
| SmartRecord 滚动分段、本地文件布局、`DiskGuard` | 云端对象存储、跨节点录像调度 |

**扩展预留**（初版可不实现）：SGIE、时序动作识别、EmptyFrameFilter 等，见 §24。

---

## 2. 技术栈（参考实现）

| 类别 | 选型 |
|------|------|
| 基础镜像 | `nvcr.io/nvidia/deepstream:9.0-triton-multiarch` |
| 管道 API | **pyservicemaker**（DeepStream Service Maker，wheel 随镜像路径安装） |
| 消息 | **confluent-kafka**（Python：`Consumer`/`Producer`） |
| 配置 | **PyYAML**（读 PGIE YAML，判断是否挂载 YOLO 后处理） |
| 推理后处理 | **CuPy**（YOLOv10 张量 → bbox，`YoloV10Postprocessor`） |
| 截图 | **Pillow** + **CuPy DLPack**（从 `Buffer` 取 RGB 张量写 JPEG）——**非**规划文档常见的纯 `jpegenc` 路径 **`[技巧]`** |
| 显存 | **pynvml**（`GpuMemoryMonitor` 守护线程） |
| SmartRecord 控制 | 自建 C 扩展 **`nvdssr_ext`**（`ext/`，对子 `nvurisrcbin` 发 `start-sr`/`stop-sr`） **`[技巧]`** |
| 裁剪拼接 | 静态 **ffmpeg/ffprobe**（`/usr/local/bin`，容器内下载 static build） |
| 预览网关 | **mediamtx** 二进制（独立进程，非 Python GstRtspServer） **`[技巧]`** |

---

## 3. 进程与生命周期模型 **`[技巧]`**

### 3.1 管道必须在 `multiprocessing.Process` 中运行

- 子进程内：`pipeline.prepare(msg_handler)` → `pipeline.activate()` → `pipeline.wait()`。  
- **禁止**对动态源管道使用 `pipeline.start().wait()`（消息回调不生效）。  
- 主进程仅 `Process(target=run_pipeline).start(); join()`（见 `main.py`）。

### 3.2 信号与优雅退出

- 在**管道子进程**主线程注册 `SIGTERM`/`SIGINT`：`pipeline.deactivate()`；并在回调中先停 `CommandConsumer`、再 `RollingRecordManager.shutdown()`、终止 MediaMTX 子进程（见 `GracefulShutdown`、`ShutdownActions`）。

### 3.3 MediaMTX 子进程

- **不要**在 GStreamer 管道内嵌 **GstRtspServer** 作为预览输出（参考实现未采用）。  
- 管道预览分支使用 **`udpsink`** 将 RTP 发到 `127.0.0.1:DS_PREVIEW_RTP_PORT`（默认 **5400**）。  
- **`subprocess.Popen(["mediamtx", config_path])`** 在 **`pipeline.activate()` 之后**启动，避免与初始化竞态。  
- MediaMTX 配置使用 **`udp+rtp://127.0.0.1:5400`**（或等价 SDP）拉取 DeepStream 推送的 H.264 RTP，再对外提供 **RTSP `:8554`** 与 **WebRTC HTTP `:8889`**（见 `config/mediamtx.yml`）。

---

## 4. 管道拓扑（精确到元素）

### 4.1 主链

`nvmultiurisrcbin` → `nvinfer`(PGIE) **或** `identity`（轻量）→ `nvtracker` **或** `identity` → 可选 `nvdsanalytics` → `tee`。

`nvmultiurisrcbin` 属性须包含（与参考实现对齐）：

- 动态源：`live-source=1`、`drop-pipeline-eos=1`、`async-handling=1`、`file-loop=1`（测试 MP4 循环）等。  
- SmartRecord：**数字键**形式（pyservicemaker 传参）：`smart-record=2`、`smart-rec-dir-path=<buffer>`、`smart-rec-file-prefix=sr`、`smart-rec-cache`、`smart-rec-default-duration`、`smart-rec-container=0`（MP4）。

### 4.2 轻量模式 `DS_LIGHT_PIPELINE=1`

- `pgie`、`tracker` 均为 **`identity`**。  
- Kafka 分支：`tee` → `queue_meta` → **`fakesink`**（**不**连 `nvmsgbroker`），用于无 Kafka 插件调试。  
- 预览与截图分支仍可运行（便于联调 MediaMTX / 截图链路）。

### 4.3 元数据分支（完整模式）

`tee` → `queue_meta` → `nvmsgconv` → `nvmsgbroker`。

- `nvmsgconv`：`msg2p-newapi=True`，`config=msgconv_config.txt`，`payload-type=1`。  
- `nvmsgbroker`：`proto-lib=libnvds_kafka_proto.so`，**`conn-str` = `host;port`（分号）**，`topic=KAFKA_TOPIC`，`sync=0`，`async=0`，`config=kafka_broker_config.txt`。  
- 若启用 `nvdsanalytics`：在 **`analytics` 元素**上挂 `Probe("analytics-probe", AnalyticsMetadataProbe())`（见 `pipeline/analytics_probe.py`），用于遍历 analytics 元数据，辅助序列化与调试日志。

> **注意**：参考实现 **未** 实现规划中的 **EmptyFrameFilter**；若需减 Kafka 量，可在 `queue_meta` 前增加 probe（§14）。

### 4.4 截图分支（与常见 jpegenc 方案不同） **`[技巧]`**

`tee` → `queue_snap` → `valve`(drop=True) → `nvvideoconvert` → `capsfilter`（**`video/x-raw(memory:NVMM),format=RGB`**）→ `appsink`。

- **无** `jpegenc`：在 `ScreenshotRetriever.consume()` 中 `buffer.extract(0).clone()` → **CuPy `from_dlpack`** → `asnumpy` → **Pillow** `Image.save(..., JPEG)`。  
- `valve` 打开后多路帧仍会进入 `appsink`；在 `consume()` 内按 **`source_id`** 匹配 `_pending`，只写目标路；全部请求完成后 `valve` 回到 `drop=True`。  
- 截图完成事件：Python **`confluent_kafka.Producer`** 发往 `KAFKA_EVENT_TOPIC`，JSON 含 `event: screenshot_done`、`source_id`、`filepath`（见 `pipeline/screenshot.py`）。

### 4.5 预览分支

`tee` → `queue_preview` → **`nvmultistreamtiler`** → **`nvdsosd`** → `nvvideoconvert` → **`nvv4l2h264enc`** → `rtph264pay` → **`udpsink`**（`host=127.0.0.1`，`port=DS_PREVIEW_RTP_PORT`，`sync=0`，`async=0`）。

- **OSD 开关**：`OsdToggle` 对 `nvdsosd` 调用 **`set({"display-bbox","display-text","display-mask"})`**，由 Kafka `toggle_osd` 触发。  
- **单路/多画面**：`nvmultistreamtiler` 的 `show-source`：**`-1`** 为 mosaic，**≥0** 为单路；Kafka `switch_preview` 使用 **`tiler.set({"show-source": int})`**（pyservicemaker 封装，而非裸 `set_property` 字符串）。

### 4.6 tee 与 async

- **所有 sink**：`msgbroker`、`snap_sink`、`preview_udpsink`（及轻量 `fakesink`）均 **`async=0`**，否则动态源 + tee 易卡 **PAUSED**。

---

## 5. 动态源与内置 REST

与 NVIDIA 动态源示例一致：

| 端点 | 说明 |
|------|------|
| `POST /api/v1/stream/add` | Payload：`key: sensor`，`value`: `camera_id`, `camera_name`, `camera_url`, `change` 含 `add` |
| `POST /api/v1/stream/remove` | `change` 含 `remove` |
| `GET /api/v1/stream/get-stream-info` | 查询 source 列表 |
| `GET /api/v1/health/get-dsready-state` | TensorRT 首启可能长时间 **NO** |

### DynamicSourceMessage

- 维护全局 **`source_map: sensor_id(str) → source_id(int)`**。  
- **流加入**：`register_source` + **自动 `start_rolling`**（参考实现于 `MessageHandler` 内；Kafka `start_rolling` 用于重试或显式重启滚动）。  
- **流移除**：`stop_rolling`、`unregister_source`。

### REST Payload 示例

**添加流**：

```json
{
  "key": "sensor",
  "value": {
    "camera_id": "cam_001",
    "camera_name": "Front Door",
    "camera_url": "rtsp://192.168.1.100:554/stream",
    "change": "camera_add"
  }
}
```

- `change` 必须包含 **`add`** 子串。  
- `camera_url` 可含用户名密码。

**移除流**：

```json
{
  "key": "sensor",
  "value": {
    "camera_id": "cam_001",
    "camera_url": "rtsp://192.168.1.100:554/stream",
    "change": "camera_remove"
  }
}
```

- `change` 必须包含 **`remove`**。

### nvmultiurisrcbin 关键属性（参考 `pipeline/builder.py`）

除 `port`、`max-batch-size`、`width`、`height` 外，须包含：

| 属性 | 典型值 | 说明 |
|------|--------|------|
| `ip-address` | `0.0.0.0` | REST 绑定 |
| `batched-push-timeout` | `33333` | 未满 batch 也推送（微秒级） |
| `live-source` | `1` | 动态源必选 |
| `drop-pipeline-eos` | `1` | 末路流移除不杀管道 |
| `async-handling` | `1` | |
| `select-rtp-protocol` | `0` | |
| `latency` | `100` | 抖动缓冲 ms |
| `file-loop` | `1` | 文件源循环（测试用） |
| `smart-record` | `2` | 启用 SmartRecord |
| `smart-rec-dir-path` | 指向全局缓冲目录 | 与 `StorageManager.buffer_dir` 一致 |
| `smart-rec-file-prefix` | `sr` | 生成 `sr_<id>_*.mp4` |
| `smart-rec-cache` | `DS_SR_CACHE_SEC` | |
| `smart-rec-default-duration` | `DS_SR_DEFAULT_DURATION` | |
| `smart-rec-container` | `0` | MP4 |

---

## 6. PGIE（nvinfer）与 YOLOv10

### 6.1 配置文件格式

使用 **YAML**，顶层 section 必须叫 **`property:`**（写成 `model:` 会导致解析失败）。

```yaml
property:
  gpu-id: 0
  net-scale-factor: 0.00392156862745098   # 1/255，YOLO 常用
  onnx-file: /app/models/yolov10n.onnx
  model-engine-file: /app/models/yolov10n.onnx_b1_gpu0_fp16.engine
  labelfile-path: /app/models/coco_labels.txt
  batch-size: 16
  network-mode: 1                   # 0=FP32, 1=FP16, 2=INT8
  num-detected-classes: 80
  process-mode: 1
  interval: 0                       # 每 (interval+1) 帧推理一次；0=每帧
  cluster-mode: 4                   # YOLOv10/v26+ 常为 4；v8/v11 多为 2
  maintain-aspect-ratio: 1
  output-tensor-meta: 1             # 为 1 时挂 YoloV10Postprocessor

class-attrs-all:
  topk: 20
  pre-cluster-threshold: 0.4
  nms-iou-threshold: 0.5
```

动态 ONNX 若维度为 -1，必须增加例如：`infer-dims: 3;640;640`（CHW），否则 TensorRT 报 `setDimensions: Error Code 3`。

**YOLO 版本与 cluster-mode**：

| 模型族 | cluster-mode | 说明 |
|--------|-------------|------|
| YOLOv8/v11 | `2` (NMS) | 原始输出需 NMS |
| YOLOv10/v26+ | `4` (None) | 已 NMS，错用 `2` 可致框角度异常 |

### 6.2 `interval` 与性能

| interval | 推理占比 | 说明 |
|----------|---------|------|
| 0 | 100% | 调试 / 精度优先 |
| 2 | ~33% | 常配合 NvDCF，跳帧由 tracker 外推 |

`interval>0` 需要真实 **nvtracker**；轻量管道下 tracker 为 identity 时无此收益。

### 6.3 TensorRT 引擎

首次运行若 `model-engine-file` 不存在，nvinfer 在线编译引擎，**可能数分钟**，此期间 **`get-dsready-state` 常为 NO**。引擎生成后写入挂载目录，冷启动复用。

### 6.4 `YoloV10Postprocessor`

- 当 YAML 中 **`output-tensor-meta: true`**，`PipelineBuilder` 在 `pgie` 上挂 **`Probe("yolo-postprocess", YoloV10Postprocessor(...))`**。  
- 使用 **CuPy** 解析张量，将结果写回对象元数据；环境变量 **`DS_YOLO_THRESHOLD`**、**`DS_YOLO_PERSON_ONLY`** 控制过滤。

### 6.5 EngineFileMonitor

- `pyservicemaker.utils.EngineFileMonitor(pgie_element, engine_path)`：在 **`StateTransitionMessage` 进入 PLAYING** 后 **`start()`**；引擎文件变更时重载 PGIE。

---

## 7. nvtracker

完整模式示例（路径以容器内为准）：

```python
pipeline.add("nvtracker", "tracker", {
    "ll-lib-file": "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
    "ll-config-file": os.environ.get(
        "DS_TRACKER_CONFIG",
        "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml",
    ),
})
```

| Tracker 风格 | 适用 |
|-------------|------|
| NvDCF perf | 默认、平衡 |
| IOU / NvSORT 等 | 按场景替换 `ll-config-file` |

轻量模式：`pipeline.add("identity", "tracker")`。

---

## 8. nvdsanalytics

- 环境变量 **`DS_ANALYTICS_CONFIG`** 非空时：`pipeline.add("nvdsanalytics", "analytics", {"config-file": path})`，位于 **tracker 与 tee 之间**。  
- 配置为 **INI**，按 **`stream-0`、`stream-1`…** 与 **`source_id`** 对应；动态源场景下 **摄像头添加顺序须与配置索引一致**，否则 ROI/越线会错位（后端部署流程需保证，DeepStream 侧可打日志校验）。  
- **`AnalyticsMetadataProbe`**：挂在 **`analytics`** 上的 `Probe`，`handle_metadata` 内遍历 **`frame_meta.nvdsanalytics_frame_items`** / **`obj_meta.nvdsanalytics_obj_items`**，调用 `as_nvdsanalytics_frame()` / `as_nvdsanalytics_obj()`，促使懒加载元数据在进 **nvmsgconv** 前就绪（见 `pipeline/analytics_probe.py`）。  
- **热更新**：参考实现不支持不停机换 analytics 文件；需替换挂载文件并 **重启管道或容器**。

### 8.1 配置片段示例（INI）

```ini
[property]
enable=1
config-width=1920
config-height=1080
osd-mode=2

[roi-filtering-stream-0]
enable=1
roi-RF=295;643;579;634;642;913;283;921
inverse-roi=0
class-id=-1
object-min-width=40
object-min-height=40

[line-crossing-stream-0]
enable=1
line-crossing-Entry=789;672;1084;900;987;757;1098;732
class-id=0
```

**`object-min-width/height`**：抑制远处极小误检，按场景调整。

---

## 9. 存储抽象 — `StorageManager`

根路径 **`DS_STORAGE_DIR`**（默认 `/app/storage`）：

```
{DS_STORAGE_DIR}/
├── recordings/                 # SmartRecord 全局缓冲（仅文件名 sr_<source_id>_*.mp4）
└── {camera_id}/
    ├── rolling/                # 滚动归档（DiskGuard 可删）
    ├── locked/                 # 时间窗裁切输出（DiskGuard 不删）
    └── screenshots/
```

- 旧版 **`{camera_id}/recordings/`** 若仍存在，`DiskGuard` 与 `RollingClipExtractor` 的 legacy 路径逻辑可兼容（见代码）。

---

## 10. SmartRecord、`RollingRecordManager`、C 扩展与缓冲轮询 **`[技巧]`**

### 10.1 为何需要 `nvdssr_ext`

- pyservicemaker 对 `nvmultiurisrcbin` 子 `nvurisrcbin` 的 SmartRecord **信号控制**在部分版本不便直接用 Python 调用。  
- 参考实现用 C 扩展 **`nvdssr_ext`**：`extract_gst_element` → `find_child_nvurisrcbin(source_id)` → **`start_recording`/`stop_recording`**（ctypes + `libgstreamer` 等待 **PLAYING**）。  
- Dockerfile：`cd ext && python3 setup.py build_ext --inplace`，`.so` 拷到 `/app`。

### 10.2 `RollingRecordManager` 行为

- **`start_rolling(source_id, uri)`**：注册 `nvurisrcbin`，`SmartRecordController.start(duration=DS_RECORDING_SEGMENT_SEC)`（环境变量覆盖默认 300）。  
- **链式续录**：段结束后若该 source 仍在 `_rolling_sources`，再次 **`start`** 同长度段。  
- **`_on_sr_done`**：从回调 **`dict`** 取 `filename`/`dirpath`，若文件 **>0 字节** 则 `shutil.move` 到 `{camera_id}/rolling/`；若 **0 字节**则 **删除**（无媒体时常因 RTSP 未起流）。  
- **`[技巧]` — `sr-done` 未进 Python**：GStreamer 的 **`sr-done` 未接到 `on_recording_done`** 时，归档依赖 **后台线程** 轮询 **`recordings/sr_<source_id>_*.mp4`**：  
  - 周期 **`DS_BUFFER_ARCHIVE_POLL_SEC`**（默认 10s）；  
  - 仅当 **mtime** 年龄 > **`DS_BUFFER_ARCHIVE_MIN_AGE_SEC`**（默认 45s）、文件非空、且能解析 `source_id`/`camera_map` 时，调用与 `_on_sr_done` 相同逻辑。  
- **`stop_rolling`**：`SmartRecordController.stop`（按 session）。

### 10.3 不设 SmartRecord 插件级 Kafka

- 参考实现 **未** 将 `proto_lib`/`conn_str`/`topic_list` 配进 bin 做 sr-done 自动发 Kafka；录像就绪以 **Python 侧** `clip_ready`/`clip_failed` 与文件路径为准（见 §15）。

### 10.4 分段时长

- **`DS_RECORDING_SEGMENT_SEC`** 为**目标**时长；实际 MP4 可能略短（GOP/关键帧对齐）。

---

## 11. 时间窗证据片段 — `RollingClipExtractor`

- **不是**再启一段 SmartRecord；从已有 **`rolling/`**（及 legacy）MP4 用 **ffprobe** 读 **duration**，用文件 **mtime** 推断墙钟 **`[wall_start, wall_end]`**。  
- 与请求 **`[window_start, window_end)`**（UTC ISO8601）求交集；单段 **`ffmpeg` trim**（`-c copy`），多段先 trim 再 **concat**。  
- 输出 **`locked/clip_<request_id>.mp4`**，`request_id` 做文件名净化。  
- 失败原因通过 Kafka **`clip_failed`** 返回（见 §15）。

---

## 12. `CommandConsumer`（Kafka 命令）

- **Consumer** 订阅 **`KAFKA_COMMAND_TOPIC`**；**Producer** 发 **`KAFKA_EVENT_TOPIC`**。  
- **`source_id` 解析**：支持 **sensor_id 字符串**、**整数**、**数字字符串**（见 `_resolve_source_id` / `_resolve_camera_id`）。  
- **`start_recording`**：登记 **`request_id` + `start_ts`（UTC）**；**`stop_recording`**：带 **`end_ts`**，校验后与 `start` 配对，**`ThreadPoolExecutor`** 异步 **`RollingClipExtractor.extract`**。  
- **`screenshot`**：`request_screenshot(**int** source_id, **str** camera_id, **str** filename)`；`camera_id` 来自 `_resolve_camera_id`，与 rolling 目录名一致。  
- 其他 action：`start_rolling` / `stop_rolling`、`switch_preview`、`toggle_osd`。  
- 未知 action 打日志 **`Unknown command action`**。

### 事件 JSON（Python 侧）

| event | 说明 |
|-------|------|
| `clip_ready` | `request_id`, `sensorId`, `clip_path`（相对 `DS_STORAGE_DIR`） |
| `clip_failed` | `request_id`, `sensorId`, `reason` |
| `screenshot_done` | `source_id`, `filepath` |
| `command_error` | 截图 API 等失败 |

---

## 13. `DiskGuard`

- 周期 **`DS_DISK_CHECK_INTERVAL`**。  
- **分区使用率** > `DS_DISK_MAX_USAGE_PCT`：按 **mtime** 最旧优先删除 **`rolling/`** 与 legacy 归档中的 `.mp4`（**不删 `locked/`**）。  
- **总字节** > `DS_DISK_MAX_STORAGE_GB` 换算值：同上，仅统计可清理目录。  
- **全局缓冲 `recordings/`**：  
  - **0 字节** 且 mtime 超过 **`DS_BUFFER_EMPTY_MAX_AGE_SEC`** → 删除；  
  - **非空**且超过 **60s** 未修改 → 视为陈旧删除（防止残留缓冲）。

---

## 14. 可选：EmptyFrameFilter（参考实现未包含）

- 若实现：在 **`queue_meta` 前** 挂 probe，丢弃「无 object 且无 analytics 事件」的帧；**必须**充分测试越线/拥挤仅帧级事件场景，避免误丢。  
- Dashboard 帧率勿用 Kafka 条数代替，应用 **`PerfMonitor`**。

---

## 15. Kafka 检测链路小结

- Topic：**`KAFKA_TOPIC`**（默认 `deepstream-detections`）。  
- 轻量模式：**不向 Kafka 发真实检测**（fakesink）。  
- **`conn-str` 分号**、**`async=0`** 为硬性要求。

---

## 16. 性能与其它守护线程

- **`PerfMonitor`**：挂在 **`tracker`** 的 **`src`** pad（或文档允许的其它非 sink probe 点）。  
- **`GpuMemoryMonitor`**：daemon 线程，管道 **activate 之后**再启动，避免与 CUDA 初始化冲突。  

---

## 17. 环境变量（完整表）

| 变量 | 默认 | 说明 |
|------|------|------|
| `DS_STORAGE_DIR` | `/app/storage` | 存储根 |
| `DS_RECORDING_SEGMENT_SEC` | `300` | 滚动每段目标秒数 |
| `DS_REST_PORT` | `9000` | 内置 REST |
| `DS_MAX_BATCH_SIZE` | `16` | batch |
| `DS_PIPELINE_WIDTH` / `HEIGHT` | `1920`/`1080` | 处理分辨率 |
| `DS_PGIE_CONFIG` | `/app/config/pgie_yolov10_config.yml` | PGIE YAML |
| `DS_TRACKER_CONFIG` | NvDCF 样本路径 | tracker |
| `DS_ANALYTICS_CONFIG` | 空 | 非空启用 nvdsanalytics |
| `DS_LIGHT_PIPELINE` | `1` | `1` 轻量；**生产推理设为 `0`** |
| `DS_PREVIEW_BITRATE` | `4000000` | NVENC 码率 |
| `DS_PREVIEW_TILER_ROWS` / `COLS` | `4`/`4` | tiler 网格 |
| `DS_PREVIEW_RTP_PORT` | `5400` | **udpsink** UDP 端口（非 REST 端口） |
| `DS_YOLO_THRESHOLD` | `0.3` | 后处理阈值 |
| `DS_YOLO_PERSON_ONLY` | `1` | 仅 person |
| `DS_LABELS_PATH` | `/app/models/coco_labels.txt` | 标签 |
| `DS_SR_CACHE_SEC` | `30` | SmartRecord cache |
| `DS_SR_DEFAULT_DURATION` | `20` | 默认段长相关 |
| `DS_MEDIAMTX_CONFIG` | `/app/config/mediamtx.yml` | MediaMTX 配置路径 |
| `KAFKA_BROKER` | `kafka:9092` | 给 Python；插件里改 `;` |
| `KAFKA_TOPIC` | `deepstream-detections` | 检测 topic |
| `KAFKA_EVENT_TOPIC` | `deepstream-events` | 事件 topic |
| `KAFKA_COMMAND_TOPIC` | `deepstream-commands` | 命令 topic |
| `DS_DISK_MAX_USAGE_PCT` | `85` | 分区阈值 |
| `DS_DISK_MAX_STORAGE_GB` | 空 | 归档字节上限 |
| `DS_DISK_CHECK_INTERVAL` | `60` | DiskGuard 周期 |
| `DS_BUFFER_ARCHIVE_POLL_SEC` | `10` | 缓冲轮询周期 |
| `DS_BUFFER_ARCHIVE_MIN_AGE_SEC` | `45` | 缓冲文件最小年龄才归档 |
| `DS_BUFFER_EMPTY_MAX_AGE_SEC` | `30` | 缓冲 0 字节文件删除年龄 |

---

## 18. 源码目录约定（参考实现）

```
deepstream/
├── main.py
├── pipeline/
│   ├── builder.py          # PipelineBuilder
│   ├── screenshot.py       # ScreenshotRetriever
│   ├── osd_toggle.py       # OsdToggle
│   ├── yolo_postprocessor.py
│   └── analytics_probe.py
├── recording/
│   ├── manager.py          # RollingRecordManager
│   ├── smartrecord.py      # SmartRecordController + nvdssr_ext
│   └── clip_extractor.py # RollingClipExtractor
├── daemons/
│   ├── command_consumer.py
│   ├── disk_guard.py
│   └── gpu_monitor.py
├── utils/
│   └── storage.py          # StorageManager
├── config/                 # pgie_yolov10_config.yml, mediamtx.yml, msgconv, kafka_broker
├── ext/                    # nvdssr_ext C 源码与 setup.py
├── models/
├── test/
└── script/                 # video2rtsp.py, start_local_demo.sh, kafka_*_e2e.sh
```

---

## 19. Dockerfile 要点

- `pip install`：**pyservicemaker wheel**、`pyyaml`、`confluent-kafka`、`Pillow`、`nvidia-ml-py`（按需）。  
- **静态 ffmpeg** 解压到 `/usr/local/bin`。  
- **mediamtx** 二进制到 `/usr/local/bin`。  
- 构建 **`nvdssr_ext`**。  
- `mkdir -p /app/storage/recordings`；`EXPOSE 9000 8554 8889`。  
- **`NVIDIA_DRIVER_CAPABILITIES` 含 `video`**。

---

## 20. 与后端 / 前端的契约摘要

- **REST**：仅通过 **Backend 代理**调用 DeepStream（前端不直连）。  
- **Kafka**：检测 **`deepstream-detections`**；命令 **`deepstream-commands`**；事件 **`deepstream-events`**。  
- **预览 URL**：浏览器访问 **`http://<host>:8889/preview/`**（WebRTC），由 MediaMTX 提供；HEAD 可能 404、GET 200 属正常现象。  
- **录像元数据**：以 **`clip_ready`/`clip_failed`** 与文件路径为准。

---

## 21. 测试与辅助脚本

- **单元测试**：`pytest test/test_unit.py --noconftest`（Storage、DiskGuard、解析逻辑等）。  
- **集成测试**：`test/test_deepstream_api.py` + `conftest.py`（需真实 REST/Kafka/RTSP）。  
- **clip 逻辑**：`test/test_clip_extraction_e2e.py`（本机 ffmpeg）。  
- **`script/video2rtsp.py`**：用 **ffmpeg** 推本地 MP4 到 **MediaMTX RTSP**（**非** GstRtspServer）。  
- **`kafka_storage_e2e.sh` / `kafka_commands_misc_e2e.sh`**：宿主机对容器内 Kafka 发命令并校验目录或日志。

---

## 22. 踩坑清单（对齐实现）

### 管道 / 链接

| 问题 | 现象 | 处理 |
|------|------|------|
| 任一 sink 未设 `async=0` | 卡 PAUSED | tee + 动态源下 **所有** sink：`msgbroker`、`appsink`、`udpsink`、`fakesink` |
| `nvmsgbroker` 后接元素 | 链接错误 | msgbroker 是 **sink**，无下游 |
| tee 分支无 `queue` | 死锁 | 每分支独立 `queue` |
| 使用 `pipeline.start().wait()` | 回调不触发 | 动态源用 `prepare` → `activate` → `wait` |
| pad 模板写死 | 连接失败 | 使用 `src_%u` 等模板连接 tee |

### 推理 / 配置

| 问题 | 处理 |
|------|------|
| ONNX 动态维未设 `infer-dims` | `setDimensions` 错误 |
| `cluster-mode` 与模型族不匹配 | 框错位或角度异常 |
| 未配置 `model-engine-file` | 每次启动编译引擎数分钟 |
| PGIE YAML 仅改宿主机未 **重建镜像** | 容器内仍旧文件 → `bad file: ...yml` |

### Kafka

| 问题 | 处理 |
|------|------|
| `conn-str` 使用 `kafka:9092` | 必须用 **`kafka;9092`**（分号） |
| `msg2p-newapi` 未开启 | 元数据序列化异常 | 设 `msg2p-newapi=True` |

### 录像 / 存储

| 问题 | 处理 |
|------|------|
| `rolling/` 长期无新文件 | 先保证 RTSP 有码流再 `stream/add`；看 **Active sources** |
| 缓冲目录只有 0 字节 mp4 | 无视频；DiskGuard / 归档逻辑删除空文件 |
| 仅依赖 GStreamer `sr-done` 回调 | 参考实现 **`[技巧]`** 以 **缓冲轮询** 为主 |

### 预览

| 问题 | 处理 |
|------|------|
| WebRTC 黑屏 / 404 | 等 TensorRT 就绪；映射 **8889**；浏览器用 **`/preview/`** 尾部斜杠 |
| NVENC 路数 | 单路 NVENC：`nvmultistreamtiler` + `show-source` |

### Python / 元数据

| 问题 | 处理 |
|------|------|
| 对 `object_items` 调 `len()` 或遍历两次 | 迭代器约束；只能单次遍历 |
| `measure_fps_probe` 挂 sink | 可能失败 | 挂到 tracker/pg 等非 sink |

---

## 23. 扩展路线（可选）

- **EmptyFrameFilter** 降低 Kafka 带宽。  
- **报警触发独立 SmartRecord**：与滚动并发策略需单独设计（当前以 **`rolling` + 时间窗** 为主）。  
- **SGIE / 时序模型**：在 tracker 后插入 `nvinfer` 或新 tee 分支。  
- **多 GPU**：多容器 + `CUDA_VISIBLE_DEVICES`，调度在后端。

---

## 24. 命令 JSON 示例（以实现为准）

```json
{"action":"start_rolling","source_id":"cam_001"}
{"action":"stop_rolling","source_id":"cam_001"}
{"action":"start_recording","source_id":"cam_001","request_id":"<uuid>","start_ts":"2026-01-01T12:00:00Z"}
{"action":"stop_recording","source_id":"cam_001","request_id":"<uuid>","end_ts":"2026-01-01T12:05:00Z"}
{"action":"screenshot","source_id":"cam_001","filename":"snap.jpg"}
{"action":"switch_preview","source_id":-1}
{"action":"toggle_osd","show":true}
```

---

## 附录 A — `nvmsgconv` / `nvmsgbroker` 配置文件（参考仓库）

**`config/msgconv_config.txt`**（节选结构；具体以仓库为准）：

```ini
[sensor0]
enable=1
type=Camera
id=default
...

[place0]
enable=1
...

[analytics0]
enable=1
id=default
source=default
description=default analytics
```

**`config/kafka_broker_config.txt`**：

```ini
[message-broker]
producer-proto-cfg = "queue.buffering.max.messages=200000;message.send.max.retries=3"
partition-key = sensorId
share-connection = 1
```

Kafka **broker 地址**只写在 **`nvmsgbroker`** 的 **`conn-str`**（分号格式）与 **`topic`**；本文件仅 **librdkafka** 调优。

---

## 附录 B — MediaMTX 与参考 `mediamtx.yml`

参考实现将 **MediaMTX** 作为**子进程**启动，与 DeepStream **同容器**；**非**独立 docker service。

- **`webrtcAddress: :8889`**：浏览器访问 `http://<host>:8889/preview/`。  
- **`paths.preview.source: udp+rtp://127.0.0.1:5400`**（与 `DS_PREVIEW_RTP_PORT` 默认一致）：从 **UDP/RTP** 收 DeepStream `udpsink` 推送的 H.264。  
- **`rtspAddress: :8554`**：对外 RTSP（测试推流、其它客户端）；与 **预览 RTP** 是两条路径。  
- 局域网 WebRTC 可能需配置 ICE / `webrtcAdditionalHosts`（见 MediaMTX 文档）。

---

## 附录 C — 性能监控与线程模型

```python
from pyservicemaker import utils

perf_monitor = utils.PerfMonitor(
    batch_size=int(os.environ.get("DS_MAX_BATCH_SIZE", "16")),
    interval=5,
    source_type="nvmultiurisrcbin",
    show_name=True,
)
perf_monitor.apply(pipeline["tracker"], "src")
```

并发实体（单进程内）：

| 组件 | 说明 |
|------|------|
| GStreamer 主循环 | `pipeline.wait()` |
| `CommandConsumer` | daemon 线程，Kafka 消费 |
| `RollingRecordManager` 缓冲归档线程 | daemon，轮询 `recordings/` |
| `DiskGuard` | daemon，文件清理 |
| `GpuMemoryMonitor` | daemon，`pipeline.activate()` **之后**启动 |
| `ScreenshotRetriever.consume` | GStreamer 推流线程 |
| `ThreadPoolExecutor` | `RollingClipExtractor` 异步任务 |

`ScreenshotRetriever` 中 **`threading.Lock`** 保护 `_pending` 与 valve；**`source_map`** 由 `DynamicSourceMessage` 写、CommandConsumer 读，单键更新在 CPython 下可接受；valve/`tiler.set` 由 pyservicemaker 封装为线程安全调用。

---

## 附录 D — 可选：EmptyFrameFilter（参考实现未包含）

若实现：在 **`queue_meta`** 的 src pad 上挂 probe，丢弃「无 `object_items` 且无 analytics 事件」的 batch。**必须**覆盖「仅有分析事件、无 object」的帧，避免越线/拥挤丢失。实现后 Dashboard 帧率须用 **PerfMonitor**，不能用 Kafka 条数。完整边界用例见历史规划文档中的 probe 测试表，集成前需在真实管道验证 `AnalyticsFrameMeta` 字段。

---

## 附录 E — `video2rtsp.py` 与测试流

- 使用 **ffmpeg** 将本地 MP4 **copy** 推到 **`rtsp://<mediamtx>:8554/<path>`**（MediaMTX 先监听，ffmpeg 推流）。  
- **不是**用 Python GstRtspServer 在 DeepStream 进程里再开一路（与预览路径独立）。  
- WebRTC 模式需 **无 B 帧** 或接受脚本校验失败。

---

## 附录 F — Docker / Compose 要点（参考项目）

- `deepstream` 服务 **`build: ./deepstream`**，`COPY . /app` 把 **PGIE YAML 等打入镜像**；改配置需 **`docker compose build deepstream`**。  
- **GPU**：`deploy.resources.reservations.devices` 或等效 `nvidia` runtime。  
- **端口**：`9000` REST；`8554` MediaMTX RTSP；`8889` WebRTC HTTP。  
- **卷**：`./deepstream/storage:/app/storage`、`./deepstream/models:/app/models`。

---

## 25. 实现验收清单（Agent 自检）

下列条目按 **模块** 分组。实现完成后应能逐项勾选（`[ ]` → `[x]`）；**阻塞级**（标 **P0**）未通过则不应视为可交付。

### 25.1 工程与构建（P0）

- [ ] 目录存在：`pipeline/`、`recording/`、`daemons/`、`utils/`、`config/`、`ext/`（含 `nvdssr_ext` 源码与 `setup.py`）、`test/`、`script/`。
- [ ] `Dockerfile`：安装 pyservicemaker wheel、静态 ffmpeg/ffprobe、mediamtx、**编译并复制 `nvdssr_ext*.so`**，`mkdir` 存储缓冲目录，`EXPOSE 9000 8554 8889`，`ENTRYPOINT` 指向 `main.py`。
- [ ] 修改 PGIE 等 **镜像内配置** 后需 **`docker compose build deepstream`** 才能生效（验收：改 YAML 不重建则容器内仍为旧文件）。

### 25.2 入口与进程模型（P0）

- [ ] `main.py` 使用 **`multiprocessing.Process` 子进程**运行 `run_pipeline()`，主进程仅 `join`。
- [ ] 子进程内顺序：`PipelineBuilder.build()` → 组装 `RollingRecordManager`、`CommandConsumer`、`DiskGuard` 等 → `GracefulShutdown` 注册 SIGTERM/SIGINT → `pipeline.prepare` → **`pipeline.activate`** → **`GpuMemoryMonitor` 在 activate 之后启动** → **`subprocess` 启动 mediamtx** → `pipeline.wait()`。
- [ ] 关停时：**先** `CommandConsumer.stop`、**`RollingRecordManager.shutdown`**（含归档线程 join）、再 `pipeline.deactivate()`、终止 MediaMTX。

### 25.3 管道拓扑（P0）

- [ ] **轻量模式** `DS_LIGHT_PIPELINE=1`：`pgie`/`tracker` 为 `identity`，Kafka 分支为 **`fakesink`**，非 `nvmsgbroker`。
- [ ] **完整模式**：`tee` 三分支均带 **`queue`**；所有 sink（`msgbroker`、`snap_sink`、`preview_udpsink` / `fakesink`）**`async=0`**（及文档要求的 `sync`）。
- [ ] **`nvmsgbroker`**：`conn-str` 为 **`host;port`**（分号），非冒号。
- [ ] **截图分支**：`valve` → `nvvideoconvert` → **`capsfilter` RGB** → `appsink`，**无** `jpegenc`；JPEG 在 `ScreenshotRetriever.consume` 用 **CuPy + Pillow** 写出。
- [ ] **预览分支**：`tiler` → `nvdsosd` → … → **`udpsink` 指向 `127.0.0.1:DS_PREVIEW_RTP_PORT`**。
- [ ] 若 `DS_ANALYTICS_CONFIG` 非空：`nvdsanalytics` 在 tracker 与 tee 之间，且 **`AnalyticsMetadataProbe` 挂在 `analytics` 上**。

### 25.4 存储与路径（P0）

- [ ] `StorageManager`：`{base}/recordings/` 全局缓冲、`{base}/{camera_id}/rolling|locked|screenshots/` 语义与参考一致。
- [ ] `DiskGuard`：**不删除** `locked/`；清理 **rolling** 与 legacy；缓冲目录 **0 字节**文件按 **`DS_BUFFER_EMPTY_MAX_AGE_SEC`** 删除；非空陈旧缓冲按实现约定删除。

### 25.5 SmartRecord 与滚动（P0）

- [ ] `SmartRecordController` 通过 **`nvdssr_ext`** 对子 `nvurisrcbin` **`start_recording`/`stop_recording`**，`register_source` 含重试等待 **PLAYING**。
- [ ] `RollingRecordManager`：**流添加时自动 `start_rolling`**（若与参考一致）；链式续录在段结束后对仍在 rolling 集合的 source 再次 `start`。
- [ ] **`[技巧]`**：实现 **缓冲目录轮询**（`DS_BUFFER_ARCHIVE_POLL_SEC` / `DS_BUFFER_ARCHIVE_MIN_AGE_SEC`），在 **`sr-done` 未接入 Python** 时仍能 **`move` 非空 mp4 至 `rolling/`**；**0 字节**段删除不归档。

### 25.6 时间窗裁剪（P0）

- [ ] `RollingClipExtractor`：用 **ffprobe** 时长 + 文件 **mtime** 推断墙钟区间；与 UTC 窗口求交；单段 **trim**、多段 **trim + concat**；输出 **`locked/clip_<request_id>.mp4`**。
- [ ] `CommandConsumer`：`start_recording` / `stop_recording` 使用 **`request_id` + `start_ts` / `end_ts`**，异步线程池执行 extract；成功/失败发 **`clip_ready` / `clip_failed`**。

### 25.7 Kafka 命令与事件（P0）

- [ ] 订阅 **`KAFKA_COMMAND_TOPIC`**，生产 **`KAFKA_EVENT_TOPIC`**。
- [ ] 支持 action：`start_rolling`、`stop_rolling`、`start_recording`、`stop_recording`、`screenshot`、`switch_preview`、`toggle_osd`；`switch_preview` 使用 **`tiler.set({"show-source": int})`**；`screenshot` 调用 **`request_screenshot(source_id, camera_id, filename)`**。
- [ ] `screenshot_done` / `command_error` 等行为与参考 JSON 字段一致（至少含 `event` 与关键 id/path）。

### 25.8 动态源与 REST（P0）

- [ ] `DynamicSourceMessage` 维护 **`sensor_id → source_id`**，供 `CommandConsumer` 解析字符串/整数 `source_id`。
- [ ] 四接口可用：`stream/add`、`stream/remove`、`get-stream-info`、`get-dsready-state`。

### 25.9 MediaMTX 与预览（P0）

- [ ] `config/mediamtx.yml` 中 **`paths.preview.source`** 与 **`DS_PREVIEW_RTP_PORT`**（默认 5400）一致；浏览器可访问 **`http://<host>:8889/preview/`**（建议带尾部 `/`）。

### 25.10 测试与脚本（建议）

- [ ] `pytest test/test_unit.py --noconftest` 通过（Storage、DiskGuard、解析逻辑等）。
- [ ] 具备 `script/video2rtsp.py`（ffmpeg 推流到 MediaMTX RTSP）。
- [ ] （可选）`kafka_storage_e2e.sh` / `kafka_commands_misc_e2e.sh` 类脚本：Kafka 命令 + 目录或日志校验。

### 25.11 手工烟测（P0）

- [ ] 一路 RTSP 入流后，`get-stream-info` 可见 **`camera_id` 与 `source_id`**，`get-dsready-state` 在引擎就绪后为 **YES**。
- [ ] **`rolling/`** 下出现非空分段 mp4（或缓冲目录先出现再归档）；**Kafka `screenshot`** 后 **`screenshots/`** 有非空 jpg。
- [ ] **`stop_recording` 无对应 `start_recording`** 时，事件 topic 或日志中出现 **`clip_failed`**（理由含无匹配 `request_id` 等）。

---

**文档结束。** 实现时请交叉验证 `deepstream/` 下源码与 `deepstream/README.md`；二者与本文冲突时，以 **Git 当前代码** 为最终裁决。
