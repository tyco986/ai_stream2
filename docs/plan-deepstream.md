# DeepStream 端 — 实现计划

## 1. 定位与职责

DeepStream 端是**纯推理引擎 + 本地存储管理**，做七件事：

1. **接收视频流** — 通过内置 REST API 动态增删 RTSP / 文件流
2. **AI 推理** — 单帧检测（YOLO 等 PGIE）+ 目标跟踪（nvtracker）
3. **视频分析** — 区域入侵、越线计数、拥挤检测、方向检测（`nvdsanalytics` 原生插件，管道内全速运行）
4. **录制与截图** — 滚动录制（7×24）、事件录制（报警触发）、手动录制、手动截图（`SmartRecordConfig` + `jpegenc`）
5. **实时预览** — 带检测标注的视频流通过 RTSP 输出，经 MediaMTX 转 WebRTC 供前端播放（`nvosd` + `nvv4l2h264enc`）
6. **输出检测结果** — 通过 Kafka 将结构化元数据 + 分析事件推送给后端
7. **磁盘自保护** — 行车记录仪模型：滚动录制循环覆盖，报警录像锁定保护（`DiskGuard` 守护线程，零外部依赖）

**不做的事**：用户认证、业务逻辑、数据库操作、前端交互。
**设计原则**：管道拓扑在启动时由配置文件确定；视频源通过 REST API 动态增删。
**扩展预留**：二级分类（SGIE）、时序动作识别（SlowFast 等）作为后续扩展，初版不实现。

---

## 2. Pipeline 架构

生产环境使用 headless（无显示）+ Kafka 输出 + RTSP 预览的管道拓扑：

```
nvmultiurisrcbin ──► nvinfer (PGIE) ──► nvtracker ──► nvdsanalytics
   (内置REST :9000)       │                 │               │
   (动态增删视频流)    单帧检测           跨帧跟踪     (区域/越线/拥挤/方向)
   (SmartRecordConfig)  (YOLO等)          (NvDCF)           │
       ↑ 录制功能                                          tee
                                      ┌─────────────┬─────────────┐
                                      │             │             │
                                 queue_meta    queue_snap    queue_preview
                                      │             │             │
                            EmptyFrameFilter   valve      nvmultistreamtiler
                              (probe:丢弃     (默认关闭)    (4×4 拼接 / show-source
                               无检测空帧)        │          单路切换)
                                      │      nvvideoconvert      │
                                 nvmsgconv     (→I420)         nvosd
                                      │           │         (画检测框)
                                 nvmsgbroker  jpegenc           │
                                 (→ Kafka)  (quality=95)   nvvideoconvert
                                 (async=0)        │           (→I420)
                                              appsink           │
                                             (async=0)   nvv4l2h264enc
                                                  │        (硬件编码)
                                      ScreenshotRetriever      │
                                      (按需开阀，写 JPEG;   rtppay
                                       consume 中按            │
                                       source_id 过滤)     udpsink
                                                      (→ RTSP Server :8554)
                                                         (async=0)
                                                      → MediaMTX → WebRTC
```

### 模型类型与管道映射

| 模型类型 | DeepStream 实现 | 推理方式 | 配置键 | 示例 |
|---------|----------------|---------|--------|------|
| `detector` | nvinfer (PGIE) | 单帧，全图 | `process-mode=1` | YOLOv8、SSD |
| `tracker` | nvtracker | 多帧关联 | `ll-config-file` | NvDCF、IOU、NvSORT |
| `analytics` | nvdsanalytics | 元数据分析 | `config-file` | ROI 过滤、越线、拥挤、方向 |

管道约束：
- 恰好 **1 个** PGIE（主检测器）
- 恰好 **1 个** nvtracker
- **0~1 个** nvdsanalytics（视频分析插件，位于 tracker 之后、tee 之前）

> **扩展预留**：后续可在 tracker 与 nvdsanalytics 之间插入 0~N 个 SGIE（二级分类），
> 也可通过新增 tee 分支实现时序动作识别（SlowFast 等），均不影响已有管道。

### 各节点说明

| 节点 | 作用 | 关键配置 |
|------|------|---------|
| `nvmultiurisrcbin` | 视频源管理 + 内置 REST API | `port=9000`, `max-batch-size=16`, `drop-pipeline-eos=1` |
| `nvinfer` (PGIE) | 主检测引擎（单帧全图推理） | `process-mode=1`, YAML config |
| `nvtracker` | 多目标跟踪（跨帧 ID 保持） | NvDCF 或 IOU tracker |
| `nvdsanalytics` | 视频分析（ROI 过滤/越线/拥挤/方向） | `config-file`，按 stream-id 配置每路摄像头的分析规则 |
| `tee` | 管道分流 | 三路：Kafka 输出、截图、实时预览 |
| `queue` (×3) | 线程解耦 | tee 的每个分支**必须**有独立 queue，否则死锁 |
| `valve` (snap) | 截图流控 | 默认 `drop=True` 阻断帧流，截图时打开放行一帧 |
| `jpegenc` (snap) | JPEG 编码 | GStreamer 原生插件，管道内完成 GPU→CPU + 编码 |
| `appsink` (snap) | 截图 JPEG 字节输出 | ScreenshotRetriever 按需写入文件，无需 cv2/torch |
| `nvosd` (preview) | 绘制检测框/标签/tracker ID | GPU 渲染，读取 NvDsObjectMeta，不需要显示器 |
| `nvv4l2h264enc` (preview) | 硬件 H.264 编码 | NVENC 单元，不占推理 GPU |
| `rtppay` + `udpsink` (preview) | RTP 封包 + RTSP 输出 | 配合 GstRtspServer 提供 RTSP 端点 |
| `EmptyFrameFilter` (probe) | **丢弃无检测对象且无分析事件的空帧**，减少 Kafka 消息量 50-80% | 挂载在 queue_meta 的 src pad |
| `ScreenshotRetriever` (appsink 回调) | **截图分支：`consume()` 中按 source_id 过滤**，非目标帧跳过 | 挂载在 appsink 上 |
| `nvmsgconv` | 元数据 → JSON 消息 | `msg2p-newapi=True`（直接读取 ObjectMeta，无需手动注入 EventMsg） |
| `nvmsgbroker` | 消息推送到 Kafka | `proto-lib=libnvds_kafka_proto.so`, `conn-str=kafka;9092` |

### 为什么需要 tee + queue × 3

- `nvmsgbroker` 是 **SINK 节点**，不能有下游。必须用 tee 分流。
- tee 的每个分支**必须**通过 `queue` 解耦线程，否则 GStreamer 会在分支间竞争锁导致死锁。
- 三个分支各有职责：
  - **queue_meta → EmptyFrameFilter(probe) → nvmsgconv → nvmsgbroker**：检测结果 + 分析事件 → Kafka（空帧被 probe 丢弃，减少 50-80% 消息量）
  - **queue_snap → valve → nvvideoconvert → jpegenc → appsink**：手动截图（按需开阀，`consume()` 中按 source_id 过滤，纯 GStreamer 编码，零外部依赖）
  - **queue_preview → tiler → nvosd → nvvideoconvert → nvv4l2h264enc → RTSP**：带检测标注的实时预览视频流（tiler 支持 4×4 多画面 / `show-source` 单路切换）

### 管道链接代码

```python
# 主链路：src → PGIE → tracker → [nvdsanalytics] → tee
elements = ["src", "pgie", "tracker"]
if analytics_enabled:
    elements.append("analytics")
elements.append("tee")
pipeline.link(*elements)

# 分支 1 — 元数据: tee → queue → msgconv → msgbroker (Kafka sink)
pipeline.link(("tee", "queue_meta"), ("src_%u", ""))
pipeline.link("queue_meta", "msgconv", "msgbroker")

# 分支 2 — 截图: tee → queue → valve(默认关) → nvvideoconvert → jpegenc → appsink
pipeline.link(("tee", "queue_snap"), ("src_%u", ""))
pipeline.link("queue_snap", "snap_valve", "snap_convert", "snap_jpegenc", "snap_sink")

# 分支 3 — 实时预览: tee → queue → nvosd → nvvideoconvert → H.264 编码 → RTSP
pipeline.link(("tee", "queue_preview"), ("src_%u", ""))
pipeline.link("queue_preview", "tiler", "osd", "preview_convert", "encoder", "rtppay", "udpsink")
```

注意 tee 的 pad 用 `src_%u` 模板，禁止写死 `src_0`。
三个分支各自的 sink 节点（`msgbroker`、`snap_sink`、`udpsink`）都必须设 `async=0`。

### async=0 — 最关键的配置

使用 tee 或动态源时，**所有 sink 节点必须设置 `async=0`**，否则管道会卡在 PAUSED 状态：

```python
pipeline.add("nvmsgbroker", "msgbroker", {"sync": 0, "async": 0, ...})
pipeline.add("appsink", "snap_sink", {"sync": 0, "async": 0})
pipeline.add("udpsink", "udpsink", {"sync": 0, "async": 0})
```

症状：摄像头显示"已添加"，但无数据流过，无错误日志（静默失败）。

---

## 3. 动态视频源管理

### 内置 REST API

`nvmultiurisrcbin` 自带 HTTP 服务器，**禁止自己另建 Flask/FastAPI 服务器**。

| 端点 | 方法 | 功能 |
|------|------|------|
| `/api/v1/stream/add` | POST | 添加视频流 |
| `/api/v1/stream/remove` | POST | 移除视频流 |
| `/api/v1/stream/get-stream-info` | GET | 查询当前流信息 |
| `/api/v1/health/get-dsready-state` | GET | 管道健康检查 |

### 添加流的 Payload

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

- `camera_id` — 唯一标识，必须与后端数据库中的 Camera.uid 一致
- `camera_url` — RTSP 地址，包含认证信息：`rtsp://user:pass@ip:554/path`
- `change` — 必须包含 `"add"` 子串

### 移除流的 Payload

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

- `change` — 必须包含 `"remove"` 子串

### DynamicSourceMessage 回调

添加/移除流后，管道会发出 `DynamicSourceMessage`，用于更新性能监控和 **sensor_id ↔ source_id 映射**：

```python
source_map = {}  # sensor_id(str) → source_id(int)，全局共享给 CommandConsumer

def on_message(message):
    if isinstance(message, DynamicSourceMessage):
        if message.source_added:
            source_map[message.sensor_id] = message.source_id
            perf_monitor.add_stream(message.source_id, message.uri,
                                    message.sensor_id, message.sensor_name)
        else:
            source_map.pop(message.sensor_id, None)
            perf_monitor.remove_stream(message.source_id)
```

> **`source_map` 是关键数据结构**：CommandConsumer 收到的 Kafka 命令中 `source_id` 是 sensor_id 字符串（如 `"cam_001"`），
> 但 SmartRecord 的 `emit("start-sr", ...)` 和 tiler 的 `show-source` 需要整数 `source_id`（由 `nvmultiurisrcbin` 动态分配）。
> 此映射在 `on_message` 回调中维护，传递给 CommandConsumer 使用。

### nvmultiurisrcbin 关键配置

```python
{
    "ip-address": "0.0.0.0",
    "port": 9000,
    "max-batch-size": 16,        # 最大同时接入流数
    "batched-push-timeout": 33333, # 33ms，即使 batch 未满也推送
    "width": 1920,
    "height": 1080,
    "live-source": 1,            # 动态源必须设为 1
    "drop-pipeline-eos": 1,      # 最后一个流移除后不终止管道
    "async-handling": 1,         # 异步状态变更
    "select-rtp-protocol": 0,    # 0=UDP+TCP 自动，4=仅TCP
    "latency": 100,              # 抖动缓冲 100ms
}
```

---

## 4. 主检测推理 — PGIE (nvinfer, process-mode=1)

### 配置文件格式

使用 YAML 格式（`.yml`），section 必须叫 `property`：

```yaml
property:
  gpu-id: 0
  net-scale-factor: 0.00392156862745098   # 1/255，YOLO 模型必须
  onnx-file: /opt/models/yolov8n.onnx
  model-engine-file: /opt/models/yolov8n.onnx_b16_gpu0_fp16.engine  # 避免每次启动重编译引擎
  labelfile-path: /opt/models/labels.txt
  batch-size: 16                    # 与 max-batch-size 一致
  network-mode: 1                   # 0=FP32, 1=FP16, 2=INT8
  num-detected-classes: 80
  process-mode: 1                   # 1=主检测器
  interval: 0                       # ★ 推理间隔：每 (interval+1) 帧推理一次
  cluster-mode: 2                   # 2=NMS (v8/v11), 4=无NMS (v10/v26+)
  maintain-aspect-ratio: 1

class-attrs-all:
  topk: 20
  pre-cluster-threshold: 0.4
  nms-iou-threshold: 0.5
```

### `interval` 参数 — 关键性能调优旋钮

`interval=N` 表示每 `N+1` 帧推理一次，中间跳过的 N 帧复用 **nvtracker 的跟踪结果**（bbox 外推）。

| interval | 推理频率 | GPU 推理负载 | 适用场景 |
|----------|---------|-------------|---------|
| 0 | 每帧推理 | 100% | **调试/精度优先** |
| 1 | 每 2 帧推理 | 50% | 中等负载 |
| 2 | 每 3 帧推理 | 33% | **生产推荐**（配合 NvDCF tracker） |
| 4 | 每 5 帧推理 | 20% | 高路数/低 GPU 场景 |

> **前提**：`interval > 0` 要求管道中有 nvtracker，否则跳过帧没有检测结果。
> **最佳实践**：16 路 1080p 场景，`interval=2` + NvDCF_perf tracker 可将 PGIE GPU 负载降至 1/3，
> FPS 不变（tracker 补齐跳过帧的 bbox）。
**model-engine-file 为什么重要**：首次运行时 TensorRT 将 ONNX 编译为引擎文件，
耗时可达数分钟。指定 `model-engine-file` 后，后续启动直接加载引擎，跳过编译。
引擎文件命名规则：`<onnx文件名>_b<batch-size>_gpu<gpu-id>_<精度>.engine`。
首次运行会自动生成，无需手动编译。

### 关键注意事项

**动态 ONNX 模型必须指定 infer-dims**：

如果 ONNX 模型有动态输入维度（如 Ultralytics 导出时 `dynamic=True`），必须加：

```yaml
property:
  infer-dims: 3;640;640            # CHW 格式
```

否则 TensorRT 会因为维度为 -1 而报 `setDimensions: Error Code 3`。

**YOLO 版本与输出格式**：

| 模型 | 输出 tensor | cluster-mode | 说明 |
|------|------------|-------------|------|
| YOLOv8/v11 | `[batch, 84, 8400]` | `2` (NMS) | 原始输出，需要 NMS |
| YOLOv10/v26+ | `[batch, 300, 6]` | `4` (None) | 已做过 NMS，直接用 |

错误匹配的症状：bbox 偏移 45° 或 135°。

**推理精度选择**：

| 精度 | network-mode | 速度 | 精度 | 适用 |
|------|-------------|------|------|------|
| FP32 | 0 | 慢 | 最高 | 调试 |
| FP16 | 1 | 快 | 略低 | **生产推荐** |
| INT8 | 2 | 最快 | 需校准 | 极致性能 |

---

## 5. 目标跟踪 (nvtracker)

跟踪器为检测到的对象分配跨帧唯一 ID，是后端做"入侵计数"、"徘徊检测"等业务逻辑的基础。

### 推荐配置

```python
pipeline.add("nvtracker", "tracker", {
    "ll-lib-file": "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
    "ll-config-file": "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml",
})
```

### Tracker 类型选择

| Tracker | 精度 | 速度 | 适用 |
|---------|------|------|------|
| IOU | 低 | 最快 | 简单场景，遮挡少 |
| NvDCF (perf) | 中 | 快 | **生产推荐** |
| NvDCF (accuracy) | 高 | 中 | 高遮挡场景 |
| NvSORT | 中高 | 中 | 需要 ReID |
| DeepSORT | 高 | 慢 | 需要外观特征 |

---

## 5A. 视频分析 — nvdsanalytics

### 为什么用 nvdsanalytics 而不是后端自算

| 方式 | 执行位置 | 频率 | 延迟 |
|------|---------|------|------|
| nvdsanalytics（管道内） | GPU 端，GStreamer 线程 | 每帧（30 FPS） | 0 |
| 后端 AlertEngine 自算 | CPU 端，Python | 每 Kafka 消息（~1 FPS） | Kafka 传输延迟 |

区域入侵、越线计数、拥挤检测、方向判断等规则属于 **元数据几何运算**，不需要额外 AI 推理。
`nvdsanalytics` 在管道内以全帧率运行，直接读取 `NvDsObjectMeta`（bbox + tracker ID），
将分析结果附加到 `AnalyticsFrameMeta` 和 `AnalyticsObjInfo`，随后通过 Kafka 传给后端。

### 管道集成

```python
if analytics_config_path:
    pipeline.add("nvdsanalytics", "analytics", {
        "config-file": analytics_config_path,
    })
```

`nvdsanalytics` 放在 `nvtracker` 之后、`tee` 之前。它只读取已有元数据，不修改视频帧。

### 配置文件格式

`nvdsanalytics` 使用 INI 格式配置文件，按 `stream-id` 定义每路摄像头的分析规则：

```ini
# config/analytics_config.txt
[property]
enable=1
config-width=1920
config-height=1080
# ★ 全局最小目标尺寸过滤：去除远距离微小误检测
osd-mode=2
font-size=12

# ---- Stream 0: 大门摄像头 ----
[roi-filtering-stream-0]
enable=1
# ROI 多边形顶点（x;y 坐标对），定义禁区
roi-RF=295;643;579;634;642;913;283;921
inverse-roi=0
class-id=-1
# ★ 最小目标尺寸过滤（像素），小于此尺寸的目标不参与分析
object-min-width=40
object-min-height=40

[overcrowding-stream-0]
enable=1
roi-OC=295;643;579;634;642;913;283;921
object-threshold=5
object-min-width=40
object-min-height=40

[line-crossing-stream-0]
enable=1
# 越线：起点x;起点y;终点x;终点y
line-crossing-Entry=789;672;1084;900;987;757;1098;732
class-id=0
# extended=0：只统计穿越（默认），extended=1：统计方向

[direction-detection-stream-0]
enable=1
direction-South=284;840;360;662
direction-North=360;662;284;840

# ---- Stream 1: 停车场摄像头 ----
[roi-filtering-stream-1]
enable=1
roi-RF=100;200;400;200;400;600;100;600
class-id=2
# class-id=2 表示只分析"车辆"类别
object-min-width=60
object-min-height=60

# ... 更多 stream 按需添加
```

**`object-min-width/height` — 减少误检测**：

远距离小目标容易产生误检测（如远处行人的 bbox 仅 20×30 像素）。
设置最小尺寸后，nvdsanalytics 会忽略低于阈值的目标，减少 ROI 误触发和越线误计数。
建议值：人员场景 40×40，车辆场景 60×60，根据摄像头视角调整。

### 支持的分析类型

| 分析类型 | 配置节 | 输出元数据 | 典型场景 |
|---------|--------|-----------|---------|
| ROI 过滤 | `[roi-filtering-stream-N]` | 目标是否在 ROI 内（per-object） | 区域入侵检测 |
| 越线计数 | `[line-crossing-stream-N]` | 穿越次数 + 方向（per-frame） | 人流统计、出入口计数 |
| 拥挤检测 | `[overcrowding-stream-N]` | ROI 内目标数是否超阈值（per-frame） | 广场/通道拥挤报警 |
| 方向检测 | `[direction-detection-stream-N]` | 目标运动方向（per-object） | 逆行检测、车辆方向 |

### 分析结果如何进入 Kafka 消息

`nvdsanalytics` 将结果附加到 `NvDsFrameMeta.AnalyticsFrameMeta` 和
`NvDsObjectMeta.AnalyticsObjInfo`，但 `nvmsgconv` 是否自动序列化这些字段
在不同 DeepStream 版本中行为不一致。**采用 probe 方式确保可靠性**：

```python
from pyservicemaker import BatchMetadataOperator

class AnalyticsMetadataProbe(BatchMetadataOperator):
    """在 nvdsanalytics 之后、tee 之前挂载，
    读取分析结果并注入到 NvDsEventMsgMeta，
    确保 nvmsgconv 序列化时包含分析数据。
    """

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            analytics_frame = self._get_analytics_frame_meta(frame_meta)
            if not analytics_frame:
                continue

            # 将分析事件注入 frame 的 user metadata
            # nvmsgconv 在 msg2p-newapi 模式下会读取这些数据
            for obj_meta in frame_meta.object_items:
                analytics_obj = self._get_analytics_obj_info(obj_meta)
                if analytics_obj:
                    # 将 ROI 状态、越线状态、方向写入 obj_meta 的扩展字段
                    self._enrich_object_meta(obj_meta, analytics_obj)

            # 帧级分析（越线计数、拥挤状态）写入 frame user metadata
            self._enrich_frame_meta(frame_meta, analytics_frame)
        return True
```

**挂载方式**：

```python
analytics_probe = AnalyticsMetadataProbe("analytics-probe")
pipeline.attach("analytics", analytics_probe)
```

> **⚠️ 如果 probe 注入方式在集成时遇到 nvmsgconv 兼容问题**，
> 备选方案：用独立 Python Kafka Producer 发送分析事件到 `deepstream-analytics` topic
>（与动作识别相同模式），后端 Consumer 订阅第三个 topic。

### stream-id 与动态摄像头的映射

`nvdsanalytics` 的配置按 `stream-N` 索引，而 `nvmultiurisrcbin` 动态分配 `source_id`。
映射策略：

1. **后端 `PipelineDeployer` 生成配置时**：按 `Camera.uid` 排序，依次分配 stream-id（0, 1, 2, ...）
2. **摄像头启动时**：后端按相同排序顺序调用 DeepStream REST API 添加流，确保 source_id 与 stream-id 一致
3. **analytics 配置变更**（如修改 ROI 区域）：需重新生成配置文件并重启 DeepStream（与 PipelineProfile 部署相同流程）

> **限制**：analytics 配置是静态的，摄像头增删后 stream-id 映射可能变化。
> 初版采用"改配置 = 重部署"策略。
>
> **防漂移机制（后端侧）**：摄像头集合变更（增删）后，后端 `PipelineDeployer` 自动将
> 当前部署标记为 `analytics_config_stale=True`。前端在摄像头列表/详情页显示持久警告条：
> "摄像头集合已变更，分析区域配置可能不准确，请重新部署管道"。
> 用户确认重部署后，标记清除。这避免运维忘记重部署导致分析区域作用于错误摄像头。
>
> **运行时校验（DeepStream 侧）**：`on_message` 回调中记录 `DynamicSourceMessage` 的
> `(sensor_id, source_id)` 映射顺序。如果某路流添加失败（RTSP 超时），后续流的 `source_id`
> 会偏移，导致 analytics 规则错位。建议在 `on_message` 中校验：
> ```python
> expected_id = sorted_camera_uids.index(message.sensor_id)
> if message.source_id != expected_id:
>     logger.error("stream-id mismatch: %s got source_id=%d, expected=%d",
>                  message.sensor_id, message.source_id, expected_id)
> ```
> 校验失败时记日志告警，不中断流添加（analytics 结果可能不准，但检测和跟踪仍正常工作）。

### nvdsanalytics 配置热更新（探索方向，非初版）

"改 ROI = 重启整个管道"代价很高（中断所有视频流）。两个潜在优化方向：

**方向 1 — 运行时 property 更新**（需验证 DS 9.0 支持）：

```python
analytics_element = pipeline["analytics"]
analytics_element.set_property("config-file", "/app/config/analytics_config_v2.txt")
```

如果 `nvdsanalytics` 支持运行时重新读取 `config-file`，可在不停管道的情况下更新分析规则。
需要在集成阶段验证：设置新路径后是否立即生效，还是需要状态切换（PLAYING → READY → PLAYING）。

**方向 2 — 快速重启（保留 TensorRT 引擎缓存）**：

```python
pipeline.deactivate()
# 更新 analytics 配置文件
pipeline.activate()
```

`deactivate()` → `activate()` 不重建管道，nvinfer 的 TensorRT 引擎保持加载状态，
重启时间从"分钟级"降到"秒级"（只重新初始化 nvdsanalytics）。
仍会中断视频流，但中断时间大幅缩短。

---

## 5B. 录制与截图

### 三种录制模式 + 截图

| 功能 | 触发方式 | 实现组件 | 输出 |
|------|---------|---------|------|
| **滚动录制** | 摄像头上线后自动开始 | SmartRecordConfig（链式续录） | 连续 MP4 分段文件 |
| **事件录制** | 报警触发 | SmartRecordConfig（含预缓存） | 单个 MP4（含报警前 N 秒） |
| **手动录制** | 用户点击开始/停止 | SmartRecordConfig（手动模式） | 单个 MP4 |
| **手动截图** | 用户点击截图 | ScreenshotRetriever（appsink 分支） | JPEG 文件 |

### SmartRecordConfig — 录制核心

三种录制模式共用同一套 `SmartRecordConfig`，挂载在 `nvmultiurisrcbin` 上：

```python
from pyservicemaker import SmartRecordConfig

# 第一步：配置 SmartRecord 参数（传给 nvmultiurisrcbin 构造函数）
sr_config = SmartRecordConfig(
    smart_rec_cache=30,           # 预缓存 30 秒（事件录制包含报警前画面）
    smart_rec_container=0,        # 0=MP4, 1=MKV
    smart_rec_dir_path="/app/storage/recordings",
    smart_rec_mode=1,             # 1=仅视频（无音频）
    # ★ 原生 Kafka 通知：录制完成后 SmartRecord 自动发送事件到 Kafka
    proto_lib="/opt/nvidia/deepstream/deepstream/lib/libnvds_kafka_proto.so",
    conn_str="kafka;9092",
    topic_list="deepstream-events",
)

# 第二步：创建 nvmultiurisrcbin，将 sr_config 传入
pipeline.add("nvmultiurisrcbin", "src", {
    "port": 9000,
    "max-batch-size": 16,
    # ... 其他配置 + sr_config 参数
})

# 第三步：获取 sr_controller 引用（用于 emit("start-sr") / emit("stop-sr")）
# 具体 API 取决于 pyservicemaker 版本，集成时确认：
# 方式 A：直接从 pipeline 元素获取
sr_controller = pipeline["src"]  # nvmultiurisrcbin 元素本身响应 "start-sr" / "stop-sr" signal
# 方式 B：通过 CommonFactory 创建独立控制器（如 A 不可用时的备选）
# sr_controller = CommonFactory.create("smart_recording_action", "sr_controller")
```

**关键特性**：
- `smart_rec_cache=30`：内部维护 30 秒的循环缓冲，事件录制时自动包含**触发前 30 秒画面**
- 录制在 `nvmultiurisrcbin` 内部完成，录的是**原始源质量**（不是管道处理后的低分辨率）
- 支持**按 source_id 独立录制**（每个摄像头独立控制）
- **原生 Kafka 通知**：`proto_lib` + `conn_str` + `topic_list` 配置后，`sr-done` 信号触发时
  SmartRecord 自动通过 `libnvds_kafka_proto.so` 发送录制完成事件到 `deepstream-events` topic，
  **无需在 Python 侧额外写 Kafka Producer**。截图完成事件仍需 Python 侧发送（见 ScreenshotRetriever）

### 滚动录制（7×24 连续录制）

> **已知限制**：SmartRecordConfig 基于链式续录（`sr-done` → 立即 `start-sr`），
> 段与段之间存在 **~10-50ms 间隙**。间隙内的视频帧不会录入文件，
> 但仍正常经过 PGIE → tracker → nvdsanalytics → Kafka 链路，检测和分析不丢失。

`SmartRecordConfig` 本身是事件触发型（开始→录 N 秒→停止）。
通过**链式续录**模式实现 7×24 滚动录制：

```python
import shutil
from pathlib import Path

class RollingRecordManager:
    """滚动录制管理器 + 录像锁定（行车记录仪模型）。
    - 滚动录制文件留在 rolling/，由 DiskGuard 循环覆盖
    - 事件/手动录制文件 sr-done 后自动移入 locked/，受超龄保护
    """

    SEGMENT_DURATION = 300  # 每段 5 分钟

    def __init__(self, sr_controller, rolling_dir, locked_dir, source_map):
        self._sr_controller = sr_controller
        self._rolling_dir = Path(rolling_dir)
        self._locked_dir = Path(locked_dir)
        self._source_map = source_map  # sensor_id → source_id 映射
        self._rolling_sources = set()  # 正在滚动录制的 source_id 集合
        self._recording_type = {}      # source_id → "rolling" | "event" | "manual"

    def start_rolling(self, source_id):
        """摄像头上线时调用，开始滚动录制。"""
        self._rolling_sources.add(source_id)
        self._recording_type[source_id] = "rolling"
        self._start_segment(source_id)

    def stop_rolling(self, source_id):
        """摄像头下线时调用，停止滚动录制。"""
        self._rolling_sources.discard(source_id)
        self._recording_type.pop(source_id, None)
        self._sr_controller.emit("stop-sr", source_id)

    def start_event_recording(self, source_id, duration=20):
        """报警触发事件录制。"""
        self._recording_type[source_id] = "event"
        self._sr_controller.emit("start-sr", source_id, duration)

    def start_manual_recording(self, source_id):
        """用户手动录制。"""
        self._recording_type[source_id] = "manual"
        self._sr_controller.emit("start-sr", source_id, 0)

    def on_sr_done(self, source_id, filepath):
        """sr-done 信号回调：
        - 根据 _recording_type 映射判断录制类型（不依赖文件名前缀）
        - 事件/手动录制 → 移入 locked/ 目录（锁定保护）
        - 滚动录制 → 留在 rolling/，立即开始下一段
        """
        filepath = Path(filepath)
        rec_type = self._recording_type.get(source_id, "rolling")
        if rec_type in ("event", "manual"):
            dest = self._locked_dir / filepath.name
            shutil.move(filepath, dest)
            self._recording_type[source_id] = "rolling"  # 恢复为滚动状态
        if source_id in self._rolling_sources:
            self._recording_type[source_id] = "rolling"
            self._start_segment(source_id)

    def _start_segment(self, source_id):
        self._sr_controller.emit("start-sr", source_id, self.SEGMENT_DURATION)
```

**滚动录制数据流**：

```
摄像头上线 → start_rolling(source_id=0)
  → start-sr(source_id=0, duration=300)
  → 300 秒后 sr-done(source_id=0, file=rolling/rolling_cam001_20260405_103000.mp4)
  → 文件留在 rolling/，自动 start-sr(source_id=0, duration=300)  ← 链式续录
  → 300 秒后 sr-done(source_id=0, file=rolling/rolling_cam001_20260405_103500.mp4)
  → ... 无限循环直到 stop_rolling 或摄像头下线
  → DiskGuard 在磁盘使用率 >85% 时自动删除 rolling/ 中最老文件
```

**事件录制数据流**：

```
后端报警命令 → on_alert_command(source_id=0, duration=20)
  → start-sr(source_id=0, duration=20)
  → sr-done(source_id=0, file=rolling/event_cam001_20260405_103215.mp4)
  → on_sr_done 检测到 event_ 前缀 → shutil.move → locked/event_cam001_20260405_103215.mp4
  → Kafka 通知后端录像完成（filepath 为 locked/ 下的最终路径）
```

每段录制生成一个独立的 MP4 文件，文件名包含摄像头 ID + 时间戳。
段与段之间可能有极短间隙（毫秒级，`sr-done` → `start-sr` 的信号传递时间），生产可接受。

### 事件录制（报警触发）

```python
def on_alert_command(self, source_id, duration=20):
    """后端报警触发，录制报警前后的视频片段。
    smart_rec_cache=30 保证包含报警前 30 秒画面。
    """
    self._sr_controller.emit("start-sr", source_id, duration)
    # sr-done 后 Kafka 通知后端：录像文件路径
```

事件录制与滚动录制**可以并存**：SmartRecordConfig 支持同一 source 的多个并发录制请求。
事件录制的文件带 `event_` 前缀，`sr-done` 后自动移入 `locked/` 目录，不会被 DiskGuard 的滚动清理误删。

### 手动录制（用户控制开始/停止）

```python
def on_manual_start(self, source_id):
    """用户点击"开始录制"，duration=0 表示录到手动停止。"""
    self._sr_controller.emit("start-sr", source_id, 0)

def on_manual_stop(self, source_id):
    """用户点击"停止录制"。"""
    self._sr_controller.emit("stop-sr", source_id)
```

### 手动截图（纯 GStreamer 方案，零外部依赖）

截图通过 tee 的第二路分支实现，使用 `valve` + `jpegenc` + `appsink` 替代 cv2/torch：

```
queue_snap → valve(默认 drop=True) → nvvideoconvert(→I420) → jpegenc(quality=95) → appsink
```

- **valve 默认关闭**（`drop=True`），所有帧被丢弃，`jpegenc` 零负载
- 截图请求到达时，打开 valve 放行帧流
- **source_id 过滤在 `ScreenshotRetriever.consume()` 中完成**：valve 打开后 16 路帧都会流入 appsink，`consume()` 只对目标 source_id 写入文件，非目标帧直接跳过
- valve 打开窗口极短（~33-66ms），jpegenc 最多编码 ~32 帧（16 路 × 2 帧），截图是低频操作，开销可接受
- **不依赖 cv2、torch、Pillow**，GPU→CPU 转换和 JPEG 编码全由 GStreamer 原生完成

```python
import threading
from pyservicemaker import BufferRetriever

class ScreenshotRetriever(BufferRetriever):
    """按需截图：valve 控制帧流开关，consume() 中按 source_id 过滤。
    valve 操作全部在锁内完成，防止 request_screenshot（CommandConsumer 线程）
    和 consume（GStreamer streaming 线程）之间的竞态。
    """

    def __init__(self, storage, valve_element=None):
        super().__init__()
        self._output_dir = output_dir
        self._valve = valve_element
        self._pending = {}           # source_id → output_path
        self._lock = threading.Lock()

    def request_screenshot(self, source_id, filename):
        with self._lock:
            self._pending[source_id] = f"{self._output_dir}/{filename}"
            self._valve.set_property("drop", False)

    def consume(self, buffer):
        source_id = buffer.source_id
        with self._lock:
            output_path = self._pending.pop(source_id, None)
            should_close = len(self._pending) == 0
            if should_close:
                self._valve.set_property("drop", True)

        if output_path is None:
            return 1

        jpeg_bytes = buffer.get_data()
        with open(output_path, "wb") as f:
            f.write(jpeg_bytes)

        return 1
```

**SourceIdFilter — 在 `ScreenshotRetriever.consume()` 中按 source_id 过滤**：

> **⚠️ 不能在 probe 中 DROP 整个 buffer**。`nvmultiurisrcbin` 将 16 路帧打包到同一个 batch buffer 中，
> `Gst.PadProbeReturn.DROP` 会丢弃整个 buffer（包括目标帧），导致截图永远截不到。
>
> 正确方案：**让所有帧通过管道到达 appsink，在 `consume()` 回调中按 `source_id` 过滤**，
> 非目标帧直接 `return 1` 跳过，目标帧才写入文件。jpegenc 会编码所有帧（包括非目标帧），
> 但 valve 打开窗口极短（~33-66ms），16 路 × 2 帧 = 最多 ~32 次 JPEG 编码，
> 截图是低频操作（用户手动点击），开销完全可接受。

`ScreenshotRetriever.consume()` 已包含 source_id 过滤逻辑（见上方代码），
非目标 source_id 的帧在 `output_path = self._pending.pop(source_id, None)` 时得到 `None`，直接跳过。

> 如果未来路数极大（>64 路）且截图频繁，可评估更源头的按源选帧（如在 tee 前用 `nvstreamdemux`
> 按 source_id 分流），但实现复杂度高且改变管道拓扑，初版不需要。

管道集成：

```python
pipeline.add("queue", "queue_snap")
pipeline.add("valve", "snap_valve", {"drop": True})       # 默认关闭
pipeline.add("nvvideoconvert", "snap_convert")             # NV12(GPU) → I420(CPU)
pipeline.add("jpegenc", "snap_jpegenc", {"quality": 95})   # JPEG 编码
pipeline.add("appsink", "snap_sink", {"sync": 0, "async": 0})

screenshot_retriever = ScreenshotRetriever(
    storage=storage_manager,
    valve_element=pipeline["snap_valve"],
)
pipeline.attach("snap_sink", Receiver("snap-receiver", screenshot_retriever))
# 无需 probe：source_id 过滤在 ScreenshotRetriever.consume() 中完成
```

截图结果保存在 `storage/{camera_id}/screenshots/`，后端提供下载 API。

### Kafka 命令通道（后端 → DeepStream）

当前架构中后端只能通过 HTTP 代理 DeepStream REST API（仅支持流增删）。
录制/截图需要新增一条**反向命令通道**：

```
                    Kafka topic: deepstream-commands
Backend ─── produce ──────────────────────────→ DeepStream (CommandConsumer)
                                                   │
                    Kafka topic: deepstream-events  │
Backend ←── consume ──────────────────────────── 录制完成/截图完成通知
```

**命令消息格式**：

```json
{"action": "start_recording", "source_id": "cam_001", "duration": 20, "type": "event"}
{"action": "start_recording", "source_id": "cam_001", "duration": 0, "type": "manual"}
{"action": "stop_recording", "source_id": "cam_001"}
{"action": "screenshot", "source_id": "cam_001", "filename": "cam001_20260405_103000.jpg"}
{"action": "start_rolling", "source_id": "cam_001"}
{"action": "stop_rolling", "source_id": "cam_001"}
{"action": "switch_preview", "source_id": -1}
```

> **`source_id` 字段类型约定**：
> - 录制/截图/滚动录制命令：`source_id` 为 **sensor_id 字符串**（如 `"cam_001"`），CommandConsumer 通过 `_resolve_source_id()` 转为整数
> - `switch_preview` 命令：`source_id` 为 **整数**（`-1`=恢复多画面总览，`N`=单路全分辨率），直接传给 tiler，不经过 resolve

**事件回传消息格式**（`deepstream-events` topic）：

录制完成事件由 **SmartRecordConfig 原生 Kafka 通知**自动发送（无需 Python Producer）。
截图完成事件由 ScreenshotRetriever 通过 Python Producer 发送：

```json
{"event": "screenshot_done", "source_id": "cam_001", "filepath": "/app/storage/cam_001/screenshots/cam001_20260405_103000.jpg"}
{"event": "recording_error", "source_id": "cam_001", "error": "disk_full"}
```

> SmartRecordConfig 的原生 Kafka 通知格式由 DeepStream 内部定义（包含 source_id、filepath、duration），
> 后端 Consumer 需适配其具体 JSON schema（在集成阶段抓包确认）。

**CommandConsumer**（DeepStream 端独立线程）：

```python
import logging

logger = logging.getLogger(__name__)

class CommandConsumer:
    """消费 deepstream-commands topic，转发到录制/截图/预览组件。
    所有命令处理均有 try-except 保护，异常只记日志，不终止消费线程。
    """

    def __init__(self, rolling_manager, sr_controller, screenshot_retriever,
                 tiler_element, source_map, kafka_config):
        self._rolling = rolling_manager
        self._sr = sr_controller
        self._screenshot = screenshot_retriever
        self._tiler = tiler_element
        self._source_map = source_map  # on_message 回调维护的 sensor_id → source_id 映射
        self._shutdown = threading.Event()
        self._consumer = Consumer(kafka_config)
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        self._consumer.subscribe(["deepstream-commands"])
        while not self._shutdown.is_set():
            msg = self._consumer.poll(timeout=1.0)
            if not msg or msg.error():
                continue
            try:
                cmd = json.loads(msg.value())
                self._dispatch(cmd)
            except Exception:
                logger.exception("Failed to process command: %s",
                                 msg.value()[:500])

    def _resolve_source_id(self, sensor_id):
        """sensor_id(str, 如 "cam_001") → source_id(int)。
        映射由 on_message 的 DynamicSourceMessage 回调维护。
        """
        source_id = self._source_map.get(sensor_id)
        if source_id is None:
            raise ValueError(f"Unknown sensor_id: {sensor_id}")
        return source_id

    def _dispatch(self, cmd):
        action = cmd["action"]
        if action == "start_rolling":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._rolling.start_rolling(source_id)
        elif action == "stop_rolling":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._rolling.stop_rolling(source_id)
        elif action == "start_recording":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._sr.emit("start-sr", source_id, cmd.get("duration", 20))
        elif action == "stop_recording":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._sr.emit("stop-sr", source_id)
        elif action == "screenshot":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._screenshot.request_screenshot(source_id, cmd["filename"])
        elif action == "switch_preview":
            # switch_preview 的 source_id 直接是整数（-1=总览，N=单路），不需要 resolve
            self._tiler.set_property("show-source", int(cmd["source_id"]))
        else:
            logger.warning("Unknown command action: %s", action)
```

> **为什么用 Kafka 而不是自建 REST**：复用现有 Kafka 基础设施，不违反"禁止在 DeepStream 端
> 自建 REST 服务器"的架构规则，且录制是异步操作，Kafka 天然匹配。
> `switch_preview` 是轻量操作（只修改 tiler 属性），Kafka 延迟 ~50ms 可接受。
>
> **架构边界说明 — switch_preview 延迟**：
> `show-source` 切换本质是管道控制信号，不是业务逻辑。当前 Kafka ~50ms 延迟对预览切换
> 无体感影响（人眼反应时间 ~150ms）。如果未来需要亚百毫秒响应（如实时 PTZ 跟踪联动），
> 可评估通过 `nvmultiurisrcbin` **已有的 REST API**（`:9000`）扩展自定义端点，
> 而非新建 REST 服务。这不违反架构原则 —— 用的是 DeepStream 原生 REST 能力。
> **初版不需要此优化。**

### 磁盘空间管理（行车记录仪模型）

> **设计原则**：写文件的进程自己管磁盘，不依赖外部服务。
> 如同行车记录仪——正常行驶录像循环覆盖，碰撞录像锁定保护。
> 后端只做**录像元数据记录**（消费 Kafka 事件写数据库），不参与磁盘管理。

**Per-camera 存储架构**：

```
/app/storage/
├── recordings/              ← SmartRecord 全局缓冲区（临时）
├── {camera_id}/
│   ├── recordings/          ← 归档后的录像段（DiskGuard 自动清理）
│   │   ├── rolling_cam001_20260405_100000.mp4
│   │   ├── rolling_cam001_20260405_100500.mp4
│   │   └── ...
│   └── screenshots/         ← 截图
└── ...
```

| 目录 | 写入来源 | 清理策略 | 说明 |
|------|---------|---------|------|
| `recordings/` (buffer) | SmartRecord 直接写入 | sr-done 后立即 move 到 per-camera 目录 | 临时缓冲区 |
| `{camera_id}/recordings/` | sr-done 归档 | 磁盘使用率 > 阈值 或 总容量 > 上限时删最老文件 | 循环覆盖，最老的先删 |

**DiskGuard — 本地磁盘自保护守护线程**：

```python
import shutil
import threading
import time
from pathlib import Path

from utils.storage import StorageManager

class DiskGuard:
    """双阈值磁盘自保护：百分比 + 绝对容量，先触发者生效。
    零外部依赖——不需要 Redis、Kafka、数据库，纯本地文件系统操作。
    使用 Event.wait(timeout) 实现可中断的定时循环。
    """

    def __init__(self, storage, max_usage_percent=85,
                 max_storage_bytes=0, check_interval=60):
        self._storage = storage
        self._max_pct = max_usage_percent
        self._max_bytes = max_storage_bytes
        self._interval = check_interval
        self._shutdown = threading.Event()

    def run(self):
        while not self._shutdown.wait(timeout=self._interval):
            self._cleanup_buffer()
            self._cleanup_by_usage()
            self._cleanup_by_capacity()

    def stop(self):
        self._shutdown.set()
```

**启动方式**（`main.py` 中）：

```python
import threading

storage = StorageManager(base_dir=os.environ.get("DS_STORAGE_DIR", "/app/storage"))

disk_guard = DiskGuard(
    storage=storage,
    max_usage_percent=int(os.environ.get("DS_DISK_MAX_USAGE_PCT", "85")),
    max_storage_bytes=int(float(os.environ.get("DS_DISK_MAX_STORAGE_GB", "0")) * (1024 ** 3)),
)
threading.Thread(target=disk_guard.run, daemon=True, name="disk-guard").start()
```

**存储容量预估**：

| 参数 | 值 | 说明 |
|------|---|------|
| 摄像头数 | 16 路 |  |
| 码率 | ~4 Mbps/路 | 1080p H.264 中等质量 |
| 每小时存储 | 16 × 4 Mbps × 3600s ÷ 8 = **28.8 GB/h** |  |
| 每天存储 | **~691 GB/天** |  |
| 1TB 磁盘 | 可存 ~1.4 天 |  |
| 4TB 磁盘 | 可存 ~5.8 天 |  |

**文件命名约定**：`{type}_{sensor_id}_{timestamp}.mp4`
- `rolling_cam001_20260405_103000.mp4` — 滚动录制段（`rolling/` 目录）
- `event_cam001_20260405_103000.mp4` — 事件录制（`locked/` 目录）
- `manual_cam001_20260405_103000.mp4` — 手动录制（`locked/` 目录）

---

## 5C. 实时预览（nvosd + RTSP + MediaMTX → WebRTC）

### 架构

```
DeepStream 管道                       协议转换                    浏览器
tee → queue_preview                MediaMTX                   前端 Vue
    → nvosd (画检测框)         ┌─────────────┐          ┌─────────────┐
    → nvvideoconvert           │ RTSP :8554  │  WebRTC  │ <video> 标签│
    → nvv4l2h264enc (NVENC)  ──► 自动拉流    ├──:8889──►│ WHEP 协议   │
    → rtppay                   │ sourceOnDemand│         │ ~200ms 延迟 │
    → GstRtspServer :8554      └─────────────┘          └─────────────┘
```

### 管道实现

```python
import gi
gi.require_version("GstRtspServer", "1.0")
from gi.repository import GstRtspServer

# 预览分支元素
pipeline.add("queue", "queue_preview")
pipeline.add("nvdsosd", "osd")                           # 绘制 bbox/label/tracker ID
pipeline.add("nvvideoconvert", "preview_convert")         # NVMM → I420
pipeline.add("nvv4l2h264enc", "encoder", {
    "bitrate": 4000000,       # 4 Mbps
    "preset-level": 1,        # ultrafast（低延迟优先）
    "iframeinterval": 30,     # 关键帧间隔
    "maxperf-enable": 1,      # 最大性能模式
})
pipeline.add("rtph264pay", "rtppay", {"pt": 96})

# RTSP Server
rtsp_server = GstRtspServer.RTSPServer()
rtsp_server.props.service = str(os.environ.get("DS_RTSP_PORT", "8554"))

factory = GstRtspServer.RTSPMediaFactory()
factory.set_launch(
    "( udpsrc port=5400 caps=\"application/x-rtp,media=video,encoding-name=H264\" "
    "! rtph264depay ! h264parse ! rtph264pay name=pay0 pt=96 )"
)
factory.set_shared(True)

mount_points = rtsp_server.get_mount_points()
mount_points.add_factory("/preview", factory)
rtsp_server.attach(None)

# udpsink 发送到 RTSP Server 的 udpsrc
pipeline.add("udpsink", "preview_udpsink", {
    "host": "127.0.0.1",
    "port": 5400,
    "sync": 0,
    "async": 0,
})
```

> **nvosd 不需要显示器**。它在 GPU 显存中渲染检测标注（bbox、label、tracker ID），
> 输出仍是 NvBufSurface，后续经 nvvideoconvert + nvv4l2h264enc 编码为 H.264。
> `nvv4l2h264enc` 使用 GPU 的 **NVENC 硬件编码单元**，不占用推理 CUDA 核心。

### NVENC 并发限制与预览策略

| GPU 类型 | 最大并发 NVENC session | 预览策略 |
|---------|----------------------|---------|
| 消费级（RTX 30/40） | **3 路** | `nvmultistreamtiler` 拼接多画面 + `show-source` 切换单画面 |
| 专业级（A2000+/T4） | **无限制** | `nvstreamdemux` 每路独立 RTSP 端点 |

> **⚠️ 初版 SLA — 消费级 GPU 预览能力**：
>
> 消费级 GPU（NVENC ≤ 3 路）下，实时预览**只支持 tiler 模式**（含 `show-source` 单路切换）。
> **不支持**"每路独立 RTSP 端点"。软编码 `x264enc` 仅作为实验性回退，**不在初版 SLA 内**。
>
> 如果客户使用消费级 GPU 但要求每路独立全分辨率预览，
> 答案是"升级到专业级 GPU"，而不是"用软编码顶"。
>
> | 能力 | 消费级 GPU（初版） | 专业级 GPU |
> |------|-------------------|-----------|
> | 多画面总览（4×4 拼接） | ✅ | ✅ |
> | 单路全分辨率切换（`show-source`） | ✅ | ✅ |
> | 每路独立 RTSP 端点 | ❌ 不支持 | ✅ |
> | 软编码回退 | 实验性，不保证 | 无需 |

初版使用 `nvmultistreamtiler`，只占用 **1 路 NVENC**：

```python
pipeline.add("nvmultistreamtiler", "tiler", {
    "rows": 4, "columns": 4,
    "width": 1920, "height": 1080,
    "show-source": -1,     # -1=多画面拼接，0~N=单路全分辨率
})
# 链接：queue_preview → tiler → osd → ...
```

**`show-source` 动态切换（单摄像头放大）**：

tiler 的 `show-source` 属性支持运行时修改，前端点击某路摄像头时，
后端通过命令通道通知 DeepStream 切换到该路的全分辨率画面，无需额外 NVENC session：

```python
def switch_preview_source(self, source_id):
    """切换 tiler 显示模式。
    source_id=-1: 多画面总览（4×4 拼接）
    source_id=N:  单路全分辨率（该摄像头独占 1920×1080）
    """
    tiler = self._pipeline["tiler"]
    tiler.set_property("show-source", source_id)
```

用户体验流程：
1. 默认显示 4×4 多画面总览（每路 ~480×270）
2. 前端点击某路摄像头 → 后端发送 `switch_preview` 命令
3. tiler 切换到该路全分辨率（1920×1080），仍只占 1 路 NVENC
4. 前端点击"返回总览" → 恢复 `show-source=-1`

**软编码回退（可选，消费级 GPU 需要每路独立预览时）**：

如果消费级 GPU 需要支持多路独立 RTSP 流（超过 NVENC 3 路限制），
可以用软件编码 `x264enc` 替代 `nvv4l2h264enc`，配合降分辨率控制 CPU 开销：

```python
if use_software_encoder:
    pipeline.add("x264enc", "encoder", {
        "tune": "zerolatency",
        "bitrate": 1500,           # 1.5 Mbps（降码率）
        "speed-preset": "ultrafast",
    })
else:
    pipeline.add("nvv4l2h264enc", "encoder", {
        "bitrate": 4000000,
        "preset-level": 1,
        "maxperf-enable": 1,
    })
```

> 软编码仅用于"按需观看"的独立预览流（`sourceOnDemand=true` 保证无人看时不编码）。
> 推荐优先使用 tiler + `show-source` 方案，软编码作为特殊场景的回退方案。

### MediaMTX — RTSP → WebRTC 网关

浏览器不能直接播放 RTSP。**MediaMTX** 作为独立容器将 RTSP 转为 WebRTC：

```yaml
# docker-compose.yml
mediamtx:
  image: bluenviron/mediamtx:latest
  ports:
    - "8554:8554"   # RTSP（DeepStream → MediaMTX）
    - "8889:8889"   # WebRTC HTTP（浏览器 → MediaMTX）
  environment:
    - MTX_PROTOCOLS=tcp
  depends_on:
    - deepstream
  restart: always
```

MediaMTX 配置（`mediamtx.yml`）：

```yaml
paths:
  preview:
    source: rtsp://deepstream:8554/preview
    sourceOnDemand: true     # 有人观看时才拉流（节省带宽和 GPU）
```

### 前端播放

前端通过 WHEP 协议（WebRTC HTTP Egress Protocol）连接 MediaMTX：

```typescript
async function startPreview(previewUrl: string) {
  const pc = new RTCPeerConnection()
  pc.ontrack = (event) => {
    videoElement.srcObject = event.streams[0]
  }
  pc.addTransceiver("video", { direction: "recvonly" })

  const offer = await pc.createOffer()
  await pc.setLocalDescription(offer)

  const res = await fetch(previewUrl, {
    method: "POST",
    headers: { "Content-Type": "application/sdp" },
    body: offer.sdp,
  })
  await pc.setRemoteDescription({
    type: "answer",
    sdp: await res.text(),
  })
}
```

后端 API 返回预览地址：`GET /api/cameras/preview_url` → `{"url": "http://mediamtx:8889/preview/whep"}`。
前端不硬编码 MediaMTX 地址，通过后端获取。

### 延迟对比

| 方案 | 端到端延迟 | 兼容性 |
|------|-----------|--------|
| **WebRTC via MediaMTX（选用）** | **200-500ms** | 所有现代浏览器 |
| HLS | 3-10 秒 | 所有浏览器 |
| MJPEG over WebSocket | ~500ms | 所有浏览器，带宽高 |

---

## 6. Kafka 消息输出

### 消息链路

```
nvinfer (PGIE) 产生检测元数据
  → nvtracker 添加跟踪 ID
  → nvdsanalytics 附加分析结果（ROI/越线/拥挤/方向）
  → AnalyticsMetadataProbe 将分析结果注入可序列化元数据
  → tee 分流
  → EmptyFrameFilter (probe) 丢弃无检测对象且无分析事件的空帧
  → nvmsgconv 将元数据转为 JSON payload（含检测 + 分析结果）
  → nvmsgbroker 推送到 Kafka topic
```

### EmptyFrameFilter — Kafka 消息量优化（减少 50-80%）

nvmsgbroker 默认对**每一帧**都生成 Kafka 消息。30 FPS × 16 路 = 480 msg/s，
其中大量帧没有检测到任何对象（空帧），尤其在静态场景中空帧占比可达 80% 以上。

在 Kafka 分支的 `queue_meta` src pad 上挂载 probe，丢弃空帧：

```python
from pyservicemaker import BatchMetadataOperator

class EmptyFrameFilter(BatchMetadataOperator):
    """丢弃无检测对象且无 nvdsanalytics 事件的帧，
    减少 Kafka 消息量 50-80%，同步降低后端 Consumer 压力。
    """

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            has_objects = False
            for _ in frame_meta.object_items:
                has_objects = True
                break

            if not has_objects and not self._has_analytics_event(frame_meta):
                frame_meta.pad_index = -1  # 标记跳过（不生成消息）
        return True

    def _has_analytics_event(self, frame_meta):
        """检查帧是否有 nvdsanalytics 分析事件（越线/拥挤触发）。"""
        analytics_meta = self._get_analytics_frame_meta(frame_meta)
        if not analytics_meta:
            return False
        # 越线计数变化或拥挤触发时保留
        return (analytics_meta.objInROIcnt
                or analytics_meta.objLCCurrCnt
                or analytics_meta.ocStatus)
```

**挂载方式**：

```python
empty_filter = EmptyFrameFilter("empty-frame-filter")
pipeline.attach("queue_meta", empty_filter)
```

**效果预估**：

| 场景 | 空帧占比 | 优化后 Kafka msg/s | 后端 DB 写入/s |
|------|---------|-------------------|---------------|
| 16 路静态监控（走廊/仓库） | ~80% | 480 → ~96 | 对应降低 |
| 16 路繁忙场景（路口/广场） | ~30% | 480 → ~336 | 对应降低 |
| 16 路混合场景（典型生产） | ~60% | 480 → ~192 | 对应降低 |

> **注意**：空帧过滤后，后端 Dashboard 的"每秒帧数"统计需改为基于 FPS 性能监控数据
> 而非 Kafka 消息计数。帧级统计由 `PerfMonitor` 提供，不受过滤影响。

**必须通过的边界测试用例**：

| # | 场景 | 预期行为 | 验证重点 |
|---|------|---------|---------|
| 1 | 16 路全静态无人，持续 1 小时后突然 1 人进入 ROI | **保留该帧**（有 object） | 不会因长期空帧导致 probe 状态异常 |
| 2 | 帧中 0 个检测对象，但 `ocStatus` 报告拥挤恢复（从 triggered→normal） | **保留该帧** | `ocStatus` 在恢复时仍有值，不被误过滤 |
| 3 | 帧中 0 个检测对象，`objLCCurrCnt` 有越线累计计数但本帧无新增 | 根据实际值判断：累计计数不变 = 无新事件 → **可丢弃** | 需确认 `objLCCurrCnt` 在无新越线时是否为空 dict 还是保留旧值 |
| 4 | `nvdsanalytics` 未启用（`analytics_enabled=False`） | `_has_analytics_event()` 返回 False，只按 object 判断 | `_get_analytics_frame_meta()` 返回 None 时不崩溃 |
| 5 | 帧中有 1 个 object 但置信度极低（如 0.1）且已被 NMS 过滤 | 如果 object 仍在 `object_items` 中 → **保留** | probe 不做置信度二次过滤，交给后端处理 |
| 6 | `objInROIcnt` 为空 dict `{}` 而非 None | 空 dict 的 bool 值为 False → **可丢弃** | 确认空 dict 不是"有事件"的表示 |

> 测试用例 2 和 3 是最容易出 bug 的场景。建议集成阶段用真实管道抓包确认
> `AnalyticsFrameMeta` 在各种边界状态下的实际值，再微调 `_has_analytics_event()` 逻辑。

### nvmsgconv 配置

```python
pipeline.add("nvmsgconv", "msgconv", {
    "config": "/app/config/msgconv_config.txt",
    "payload-type": 1,       # 0=完整 schema, 1=精简 schema（推荐）
    "msg2p-newapi": True,    # 直接读取 ObjectMeta，无需注入 EventMsg
})
```

**payload-type 选择**：

| 类型 | 值 | 每条消息内容 | 适用 |
|------|---|------------|------|
| 完整 schema | 0 | 每个对象一条 JSON（含 sensor/place/analytics） | 需要完整上下文 |
| 精简 schema | 1 | 一帧所有对象合并一条 JSON | **推荐，带宽低** |

精简 schema 示例（含 nvdsanalytics 分析结果）：

```json
{
  "messageid": "uuid",
  "mdsversion": "1.0",
  "@timestamp": "2026-04-05T10:30:00.000Z",
  "sensorId": "cam_001",
  "analytics": {
    "overcrowding": {"roi_name": "entrance", "count": 7, "threshold": 5, "triggered": true},
    "lineCrossing": [
      {"name": "Entry", "in": 23, "out": 18}
    ]
  },
  "objects": [
    {
      "id": "1", "type": "person", "confidence": 0.92,
      "bbox": {"topleftx": 100, "toplefty": 200, "bottomrightx": 300, "bottomrighty": 400},
      "analytics": {"roiStatus": ["entrance"], "direction": "South"}
    },
    {
      "id": "2", "type": "car", "confidence": 0.87,
      "bbox": {"topleftx": 400, "toplefty": 300, "bottomrightx": 600, "bottomrighty": 500}
    }
  ]
}
```

**analytics 字段说明**：

| 层级 | 字段 | 来源 | 说明 |
|------|------|------|------|
| 帧级 | `analytics.overcrowding` | `AnalyticsFrameMeta` | ROI 内目标数及是否超阈值 |
| 帧级 | `analytics.lineCrossing` | `AnalyticsFrameMeta` | 各越线的累计进出计数 |
| 目标级 | `objects[].analytics.roiStatus` | `AnalyticsObjInfo` | 该目标所在 ROI 名称列表 |
| 目标级 | `objects[].analytics.direction` | `AnalyticsObjInfo` | 该目标运动方向 |

> 无 nvdsanalytics 时 `analytics` 字段不存在，后端需做空值兼容处理。

### nvmsgbroker 配置

```python
pipeline.add("nvmsgbroker", "msgbroker", {
    "proto-lib": "/opt/nvidia/deepstream/deepstream/lib/libnvds_kafka_proto.so",
    "conn-str": "kafka;9092",       # 注意！分号分隔，不是冒号
    "topic": "deepstream-detections",
    "sync": 0,
    "async": 0,                      # tee/动态源场景必须
    "config": "/app/config/kafka_broker_config.txt",
})
```

**conn-str 用分号** `kafka;9092`，不是冒号 `kafka:9092`。这是 DeepStream 特有的格式。

### Kafka Broker 配置文件

连接地址和 topic 由 `nvmsgbroker` 的 `conn-str` 和 `topic` 插件属性指定（见上），
此配置文件是 **librdkafka 调优参数**，不是连接信息：

```ini
# kafka_broker_config.txt
[message-broker]
# librdkafka 生产者调优
producer-proto-cfg = "queue.buffering.max.messages=200000;message.send.max.retries=3"
# 分区 key（精简 schema 用 sensorId，完整 schema 用 sensor.id）
partition-key = sensorId
# 同进程共享连接
share-connection = 1
```

### msgconv 传感器配置文件

```ini
# msgconv_config.txt
[sensor0]
enable=1
type=Camera
id=default
location=0.0;0.0;0.0
description=Default Camera

[place0]
enable=1
id=0
type=default
name=default

[analytics0]
enable=1
id=default
description=Object Detection
source=custom-model
version=1.0
```

---

## 7. 性能监控

### PerfMonitor — FPS 实时监控

```python
from pyservicemaker import utils

perf_monitor = utils.PerfMonitor(
    batch_size=16,
    interval=5,                     # 每 5 秒报告一次
    source_type="nvmultiurisrcbin",
    show_name=True,
)
# headless 管道无 tiler，挂到 tracker 的 src pad
perf_monitor.apply(pipeline["tracker"], "src")
```

注意：`measure_fps_probe` 不能挂到 sink 元素（会报 `RuntimeError: Probe failure`），
只能挂到处理元素（`nvinfer`、`nvtracker`、`nvosdbin`）。

### EngineFileMonitor — 模型热更新

```python
from pyservicemaker import utils

engine_monitor = utils.EngineFileMonitor(pipeline["pgie"], engine_file_path)
```

监控 TensorRT 引擎文件变化，文件更新后自动重载推理引擎，**零停机**。

适用于：A/B 测试模型、模型迭代发布。

启动时机：必须在管道进入 PLAYING 状态后启动：

```python
def on_message(message):
    if isinstance(message, StateTransitionMessage):
        if message.new_state == PipelineState.PLAYING and not engine_monitor.started:
            engine_monitor.start()
```

### GPU 显存监控

16 路 1080p + PGIE + tracker + nvdsanalytics + NVENC 编码，在消费级 GPU（12GB 显存）上可能逼近上限。
通过 `pynvml` 周期性记录显存使用情况：

```python
import pynvml
import threading
import logging

logger = logging.getLogger(__name__)

class GpuMemoryMonitor:
    def __init__(self, interval=30, gpu_index=0):
        self._interval = interval
        self._gpu_index = gpu_index

    def run(self):
        pynvml.nvmlInit()
        handle = pynvml.nvmlDeviceGetHandleByIndex(self._gpu_index)
        while True:
            info = pynvml.nvmlDeviceGetMemoryInfo(handle)
            used_mb = info.used / 1024 / 1024
            total_mb = info.total / 1024 / 1024
            pct = info.used / info.total * 100
            logger.info("GPU memory: %.0f/%.0f MB (%.1f%%)", used_mb, total_mb, pct)
            if pct > 90:
                logger.warning("GPU memory usage above 90%%!")
            time.sleep(self._interval)
```

> 在 `main.py` 中以 daemon 线程启动。Dockerfile 中需加 `pip install pynvml`。
> 如不想引入额外依赖，也可通过 shell 命令 `nvidia-smi --query-gpu=memory.used,memory.total --format=csv`
> 定期采集。

---

## 8. 管道启动模式

### 动态源管道必须用 prepare() + activate()

使用 `nvmultiurisrcbin`（动态源 + 消息回调）时，**不能**用 `pipeline.start().wait()`，
必须使用三步启动：

```python
pipeline.prepare(on_message)   # 注册消息回调（DynamicSourceMessage, StateTransitionMessage）
pipeline.activate()            # 进入 PLAYING 状态
pipeline.wait()                # 阻塞等待
```

`pipeline.start().wait()` 仅适用于无回调的简单管道。

### 必须在 multiprocessing.Process 中运行

DeepStream 管道涉及 GStreamer 主循环和 GPU 资源，**必须**在独立进程中运行，
避免与 Python 主进程的信号处理、GIL 冲突：

```python
from multiprocessing import Process

def run_pipeline():
    pipeline = build_pipeline()
    pipeline.prepare(on_message)
    pipeline.activate()
    pipeline.wait()

if __name__ == "__main__":
    process = Process(target=run_pipeline)
    process.start()
    process.join()
```

### 优雅关停

Docker 发送 SIGTERM 时，管道需要正确清理。在 `on_message` 回调中监听状态，
或在独立线程中监听信号后调用 `pipeline.deactivate()`：

```python
import signal

class GracefulShutdown:
    def __init__(self, pipeline):
        self.pipeline = pipeline
        signal.signal(signal.SIGTERM, self._handle)
        signal.signal(signal.SIGINT, self._handle)

    def _handle(self, signum, frame):
        self.pipeline.deactivate()
```

> **信号处理与子进程**：`signal.signal()` 只能在主线程注册。
> `pipeline.wait()` 阻塞主线程（运行 GLib main loop），信号处理器在同一主线程被调用，
> 因此 `GracefulShutdown` 在 `multiprocessing.Process` 的子进程中正常工作。

### 进程内线程模型

DeepStream 容器的单个 Python 进程（`main.py`）内有以下并发实体：

| 线程/组件 | 类型 | 职责 | 启动方式 |
|-----------|------|------|---------|
| **GStreamer Main Loop** | 主线程（`pipeline.wait()` 阻塞） | 管道调度、元素回调、probe 执行 | `pipeline.activate()` |
| **GstRtspServer** | attach 到 GLib Main Loop | RTSP 请求处理 | `rtsp_server.attach(None)` |
| **CommandConsumer** | daemon 线程 | 消费 `deepstream-commands` Kafka topic | `threading.Thread(daemon=True)` |
| **DiskGuard** | daemon 线程 | 磁盘使用率检查 + 清理 | `threading.Thread(daemon=True)` |
| **ScreenshotRetriever.consume()** | GStreamer streaming 线程回调 | 接收 appsink buffer、写 JPEG | 由 GStreamer 调度 |

**线程交互与共享状态**：

```
CommandConsumer 线程                  GStreamer streaming 线程
     │                                        │
     ├── sr_controller.emit("start-sr")       │  ← GStreamer signal，thread-safe
     ├── screenshot_retriever.request_screenshot()
     │        │                               │
     │        └── self._lock ──── 共享 ──── self._lock
     │                                        │
     │                           screenshot_retriever.consume()
     ├── tiler.set_property("show-source")    │  ← GObject property，thread-safe
     │                                        │
DiskGuard 线程                                │
     │                                        │
     └── 无共享状态（纯文件系统操作）            │
```

> **安全约定**：
> - `ScreenshotRetriever` 的 `_pending` dict 和 valve 操作通过 `threading.Lock` 保护
> - GStreamer 的 `set_property()` 和 GObject signal `emit()` 是 thread-safe 的
> - `source_map`（sensor_id → source_id）由 `on_message` 回调（主线程）写入，CommandConsumer 线程读取；
>   dict 的单键读写在 CPython GIL 下是原子的，不需要额外锁
> - DiskGuard 只操作文件系统，与其他线程无共享状态

---

## 9. Docker 容器化

### 基础镜像

```dockerfile
FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch
```

选择 `triton-multiarch` 是因为它包含完整的开发环境 + pyservicemaker wheel。

### deepstream/Dockerfile

构建上下文为 `deepstream/` 目录，`docker build ./deepstream` 即可独立构建：

```dockerfile
FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch

# 国内镜像加速（DeepStream 镜像基于 Ubuntu）
RUN sed -i 's|archive.ubuntu.com|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|security.ubuntu.com|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list
RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple

# 安装 pyservicemaker（容器内自带 wheel 但未预装）+ CommandConsumer 依赖
RUN pip install --break-system-packages \
    /opt/nvidia/deepstream/deepstream/service-maker/python/pyservicemaker*.whl \
    pyyaml \
    confluent-kafka \
    pynvml

# Kafka + GStreamer RTSP Server 依赖
RUN apt-get update && apt-get install -y \
    librdkafka-dev \
    libgstrtspserver-1.0-dev \
    gir1.2-gstrtspserver-1.0 \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN mkdir -p /app/recordings /app/screenshots

# GPU 视频编解码能力
ENV NVIDIA_DRIVER_CAPABILITIES=${NVIDIA_DRIVER_CAPABILITIES},video

EXPOSE 9000 8554
ENTRYPOINT ["python3", "main.py"]
```

> **注意**：DeepStream 镜像基于 **Ubuntu**（不是 Debian），apt 源文件是 `/etc/apt/sources.list`，
> 域名是 `archive.ubuntu.com` 和 `security.ubuntu.com`，与后端的 Debian slim 不同。

### 独立构建命令

```bash
# 独立构建
docker build -t ai-stream-deepstream ./deepstream

# 独立运行（需要 NVIDIA runtime）
docker run --rm --runtime=nvidia \
    -e NVIDIA_VISIBLE_DEVICES=all \
    -e KAFKA_BROKER=kafka:9092 \
    -p 9000:9000 \
    ai-stream-deepstream
```

### Docker Compose 配置

项目根目录 `docker-compose.yml` 中：

```yaml
deepstream:
  build:
    context: ./deepstream
  runtime: nvidia
  environment:
    - NVIDIA_VISIBLE_DEVICES=all
    - KAFKA_BROKER=kafka:9092
    - DS_REST_PORT=9000
    - DS_MAX_BATCH_SIZE=16
    - DS_RTSP_PORT=8554
  ports:
    - "9000:9000"
    - "8554:8554"    # RTSP 预览（供 MediaMTX 拉流）
  depends_on:
    - kafka
  healthcheck:
    test: ["CMD", "curl", "-f", "http://localhost:9000/api/v1/health/get-dsready-state"]
    interval: 30s
    timeout: 10s
    retries: 3
    start_period: 120s  # TensorRT 首次编译引擎可能需要数分钟
  restart: always

mediamtx:
  image: bluenviron/mediamtx:latest
  ports:
    - "8889:8889"    # WebRTC WHEP（供前端播放）
  environment:
    - MTX_PROTOCOLS=tcp
  volumes:
    - ./mediamtx/mediamtx.yml:/mediamtx.yml
  depends_on:
    - deepstream
  restart: always
```

### 关键依赖清单

| 依赖 | 安装方式 | 用途 |
|------|---------|------|
| pyservicemaker | pip install (容器内 wheel) | Python Pipeline API |
| pyyaml | pip install | YAML 配置解析 |
| confluent-kafka | pip install | CommandConsumer（消费 deepstream-commands topic） |
| librdkafka-dev | apt-get | Kafka 协议适配器（nvmsgbroker + confluent-kafka 共用） |
| libgstrtspserver-1.0-dev | apt-get | GStreamer RTSP Server（实时预览） |
| gir1.2-gstrtspserver-1.0 | apt-get | RTSP Server 的 Python GI 绑定 |
| libmosquitto1 | apt-get（可选） | 如果 tracker 需要 MQTT |

> **不需要 cv2 / torch / Pillow**。截图使用 GStreamer 原生 `jpegenc`，预览使用 `nvv4l2h264enc`。
> DeepStream 容器镜像体积不额外膨胀。

### NGC 镜像拉取认证

```bash
docker login nvcr.io
# Username: $oauthtoken
# Password: <NGC_API_KEY>
```

---

## 10. 配置管理

### 环境变量

所有可变配置通过环境变量注入，不硬编码：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `KAFKA_BROKER` | `kafka:9092` | Kafka 地址（注意代码中转为分号格式） |
| `KAFKA_TOPIC` | `deepstream-detections` | 检测结果 Kafka topic |
| `KAFKA_COMMAND_TOPIC` | `deepstream-commands` | 后端 → DeepStream 命令 topic |
| `KAFKA_EVENT_TOPIC` | `deepstream-events` | DeepStream → 后端 事件 topic |
| `DS_REST_PORT` | `9000` | 内置 REST API 端口 |
| `DS_MAX_BATCH_SIZE` | `16` | 最大同时接入流数 |
| `DS_PGIE_CONFIG` | `/app/config/pgie_config.yml` | PGIE 主检测配置路径 |
| `DS_TRACKER_CONFIG` | 见代码 | Tracker 配置文件路径 |
| `DS_ANALYTICS_CONFIG` | `""` | nvdsanalytics 配置文件路径（空=不启用分析） |
| `DS_NETWORK_MODE` | `1` | 推理精度 (0=FP32, 1=FP16, 2=INT8) |
| `DS_ROLLING_DIR` | `/app/recordings/rolling` | 滚动录制输出目录（DiskGuard 自动清理） |
| `DS_LOCKED_DIR` | `/app/recordings/locked` | 事件/手动录制目录（锁定保护，仅超龄清理） |
| `DS_SCREENSHOT_DIR` | `/app/screenshots` | 截图输出目录（共享卷） |
| `DS_RECORDING_SEGMENT_SEC` | `300` | 滚动录制每段时长（秒） |
| `DS_RECORDING_CACHE_SEC` | `30` | 事件录制预缓存秒数 |
| `DS_DISK_MAX_USAGE_PCT` | `85` | DiskGuard 触发 rolling/ 清理的磁盘使用率阈值（%） |
| `DS_LOCKED_MAX_AGE_DAYS` | `30` | locked/ 目录文件最大保留天数 |
| `DS_DISK_CHECK_INTERVAL` | `60` | DiskGuard 检查间隔（秒） |
| `DS_RTSP_PORT` | `8554` | RTSP 预览输出端口 |
| `DS_PREVIEW_BITRATE` | `4000000` | 预览视频码率（bps） |
| `DS_PREVIEW_TILER_ROWS` | `4` | tiler 行数（0=禁用 tiler，按流独立输出） |
| `DS_PREVIEW_TILER_COLS` | `4` | tiler 列数 |
| `DS_EMPTY_FRAME_FILTER` | `1` | 启用空帧过滤（0=关闭，发送所有帧到 Kafka） |

> 磁盘管理完全由 DeepStream 端 `DiskGuard` 负责，Backend 不参与文件清理。

### 目录结构

```
deepstream/
├── main.py                        # 入口：构建并启动管道
├── pipeline_builder.py            # 管道构建逻辑（动态组装 PGIE + tracker + analytics + snap + preview 分支）
├── preview_server.py              # GstRtspServer 封装（RTSP 预览输出）
├── analytics_probe.py             # AnalyticsMetadataProbe（nvdsanalytics 结果注入 Kafka 消息）
├── empty_frame_filter.py          # EmptyFrameFilter（空帧过滤 probe，减少 Kafka 消息量 50-80%）
├── recording_manager.py           # RollingRecordManager + SmartRecord 信号处理
├── screenshot_handler.py          # ScreenshotRetriever（按需截图，consume() 中按 source_id 过滤）
├── command_consumer.py            # CommandConsumer（消费 deepstream-commands Kafka topic）
├── config/
│   ├── pgie_config.yml            # PGIE 主检测配置（YOLO）
│   ├── tracker_config.yml         # nvtracker 配置
│   ├── analytics_config.txt       # nvdsanalytics 配置（ROI/越线/拥挤/方向，由后端 PipelineDeployer 生成）
│   ├── kafka_broker_config.txt    # Kafka broker 配置
│   └── msgconv_config.txt         # nvmsgconv 传感器/场所配置（msg2p-newapi 模式下仅启动检查用）
├── models/                        # 模型文件（挂载或 COPY）
│   ├── yolov8n.onnx               # PGIE 模型
│   └── yolov8n_labels.txt
├── disk_guard.py                  # DiskGuard（行车记录仪模型磁盘自保护守护线程）
├── recordings/
│   ├── rolling/                   # 滚动录制（DiskGuard 按磁盘使用率自动清理）
│   └── locked/                    # 事件/手动录制（锁定保护，仅按超龄清理）
├── screenshots/                   # 截图输出目录（共享卷，ScreenshotRetriever 写入）
└── Dockerfile
```

---

## 11. 踩坑预防清单

### Pipeline 构建类

| 坑 | 现象 | 解决 |
|----|------|------|
| sink 节点缺少 `async=0` | 管道卡在 PAUSED，无数据流 | tee/动态源场景，**所有** sink 设 `async=0` |
| `nvmsgbroker` 后面接了其他节点 | 链接错误 | nvmsgbroker 是 SINK，不能有下游 |
| pad 模板写死 `sink_0` | 连接失败 | 必须用 `sink_%u` 模板 |
| 忘记 `drop-pipeline-eos=1` | 最后一个流移除后管道终止 | nvmultiurisrcbin 必须设此项 |
| 忘记 `live-source=1` | 动态源添加后不工作 | 动态源场景必须设此项 |
| tee 分支缺少 `queue` | 管道死锁，无数据流 | tee 的每个分支必须有独立 queue |
| 用 `start().wait()` 启动动态源管道 | 消息回调不生效 | 必须用 `prepare()` + `activate()` + `wait()` |

### 推理类

| 坑 | 现象 | 解决 |
|----|------|------|
| ONNX 动态维度未指定 `infer-dims` | `setDimensions: Error Code 3` | 加 `infer-dims=3;640;640` |
| config section 写成 `model:` | 配置解析失败 | 必须是 `property:` (YAML) 或 `[property]` (INI) |
| YOLO v10+ 用了 `cluster-mode=2` | bbox 偏移 45° | v10/v26+ 用 `cluster-mode=4` |
| 精度设错 | 推理慢或不准 | 生产用 FP16 (`network-mode=1`) |
| 缺少 `model-engine-file` | 每次启动耗时数分钟编译引擎 | 配置中始终指定 engine 路径，首次自动生成 |
| 缺少 `net-scale-factor` | 推理结果全错 | YOLO 模型用 `0.00392156862745098` (1/255) |

### Kafka 类

| 坑 | 现象 | 解决 |
|----|------|------|
| `conn-str` 用冒号 `kafka:9092` | 连接失败 | 必须用分号 `kafka;9092` |
| 未装 `librdkafka-dev` | `unable to open shared library` | Dockerfile 中 `apt-get install librdkafka-dev` |
| `msg2p-newapi` 未设 True | nvmsgconv 静默输出 0 条消息 | 设 `msg2p-newapi=True` 或手动挂 EventMsg probe |

### 推理调优类

| 坑 | 现象 | 解决 |
|----|------|------|
| PGIE 未设 `interval` 参数 | GPU 满载但 FPS 可以更高 | `interval=2` 配合 nvtracker 可将 GPU 推理负载降至 1/3 |

### 视频分析类

| 坑 | 现象 | 解决 |
|----|------|------|
| `config-width/height` 与视频不匹配 | ROI 坐标偏移 | 必须与 `nvmultiurisrcbin` 的 `width/height` 一致（如 1920×1080） |
| `stream-N` 的 N 与实际 source_id 不对应 | 分析规则作用于错误摄像头 | 后端按 Camera.uid 排序分配 stream-id，启动流时保持同一顺序 |
| 动态增删摄像头后 stream-id 漂移 | 新摄像头获得旧摄像头的分析规则 | 修改摄像头集合后需重部署 analytics 配置（初版限制） |
| `nvmsgconv` 未序列化 analytics 元数据 | Kafka 消息中无 analytics 字段 | 使用 `AnalyticsMetadataProbe` 在 tee 之前注入可序列化元数据 |
| `SmartRecordConfig` 目录不存在 | 录像写入失败（静默） | Dockerfile 中 `mkdir -p /app/recordings/rolling /app/recordings/locked /app/screenshots`，挂载共享卷 |
| 未设 `object-min-width/height` | 远距离微小目标触发大量误报 | 按场景设最小尺寸：人员 40×40，车辆 60×60 |

### 录制与截图类

| 坑 | 现象 | 解决 |
|----|------|------|
| 滚动录制段间隙 | `sr-done` → `start-sr` 之间丢几帧 | ~10-50ms 间隙，检测不丢失（Kafka 链路不受影响），录像有微量缺口 |
| 事件录制与滚动录制冲突 | 同一 source 并发 SmartRecord 请求 | SmartRecordConfig 支持同源并发录制，但需验证 DeepStream 9.0 行为 |
| 磁盘写满导致录制静默失败 | 录像文件 0 字节或损坏 | DiskGuard 守护线程在 85% 使用率时自动清理 `rolling/`，零外部依赖 |
| 截图时 source_id 映射错误 | 截到错误摄像头的画面 | `CommandConsumer._resolve_source_id()` 用 sensor_id→source_id 映射表 |
| valve 打开后 16 路都流入 jpegenc | jpegenc 编码非目标摄像头的帧（少量 CPU 开销） | valve 窗口极短（~33-66ms），`ScreenshotRetriever.consume()` 中按 source_id 过滤只写目标帧 |
| valve 打开后未关闭 | jpegenc 持续编码每帧 | `ScreenshotRetriever.consume()` 中确保捕获后立即 `valve.set_property("drop", True)` |
| 录像文件未正确关闭 | 最后一段 MP4 损坏 | GracefulShutdown 中先 stop 所有录制再停管道 |

### 实时预览类

| 坑 | 现象 | 解决 |
|----|------|------|
| NVENC 并发超限（消费级 GPU 限 3 路） | `nvv4l2h264enc` 报错 | 用 `nvmultistreamtiler` 拼接为 1 路，或用 `x264enc` 软编码回退 |
| tiler 多画面下每路只有 480×270 | 细节丢失，无法看清 | 用 `show-source=N` 切换到单路全分辨率预览 |
| `show-source` 切换后画面黑屏 | source_id 不存在或已移除 | 切换前检查 source_id 有效性，无效时回退 `show-source=-1` |
| nvosd 未收到 ObjectMeta | 预览画面无检测框 | 确保 nvosd 在 nvdsanalytics 之后、tee → queue_preview 分支上 |
| RTSP Server 端口被占 | 启动失败 | 通过 `DS_RTSP_PORT` 环境变量配置，docker-compose 映射对应端口 |
| MediaMTX 拉不到 RTSP 流 | 前端无画面 | 确保 docker 网络中 DeepStream 和 MediaMTX 互通，RTSP 地址用服务名 `deepstream` |
| `sourceOnDemand` 未设置 | 无人观看时仍在编码和传输 | MediaMTX 配置 `sourceOnDemand: true`，按需拉流 |

### Kafka 消息量类

| 坑 | 现象 | 解决 |
|----|------|------|
| 未启用 EmptyFrameFilter | Kafka 480 msg/s，后端 Consumer 压力大 | 启用 `EmptyFrameFilter` probe 过滤空帧，减少 50-80% |
| EmptyFrameFilter 误过滤有分析事件的帧 | 越线/拥挤事件丢失 | probe 中必须检查 `AnalyticsFrameMeta`，有分析事件的帧即使无对象也要保留 |
| 空帧过滤后 Dashboard 帧数统计失真 | 显示的"每秒帧数"远低于实际 | Dashboard 帧率改用 `PerfMonitor` 数据，不依赖 Kafka 消息计数 |

### 代码类

| 坑 | 现象 | 解决 |
|----|------|------|
| 对 `object_items` 用 `len()` | `TypeError: iterator has no len()` | 遍历计数，不能用 len |
| `object_items` 遍历两次 | 第二次为空 | iterator 只能消费一次 |
| venv 中缺少 pyservicemaker | `ModuleNotFoundError` | pip install 容器内的 wheel |
| `measure_fps_probe` 挂到 sink | `RuntimeError: Probe failure` | 挂到 nvinfer 或 nvosdbin |
| `queue.Queue` 配 `multiprocessing.Process` | 数据静默丢失 | 用 `multiprocessing.Queue` |

---

## 12. 与后端的对接契约

### 后端 → DeepStream（HTTP）

后端通过 `httpx` 调用 DeepStream 内置 REST API：

```
Backend  ──HTTP POST──►  DeepStream :9000/api/v1/stream/add
Backend  ──HTTP POST──►  DeepStream :9000/api/v1/stream/remove
Backend  ──HTTP GET───►  DeepStream :9000/api/v1/stream/get-stream-info
Backend  ──HTTP GET───►  DeepStream :9000/api/v1/health/get-dsready-state
```

Docker 网络中，后端使用服务名 `deepstream` 访问。

### DeepStream → 后端（Kafka）

DeepStream 通过两个 Kafka topic 推送结果/事件：

```
nvmsgbroker ──────────────► Kafka topic: deepstream-detections ──► Backend Consumer
                            (PGIE 检测 + nvdsanalytics 分析结果)

SmartRecordConfig(原生) / ──► Kafka topic: deepstream-events ─────► Backend Consumer
ScreenshotRetriever(Python)    (录制完成、截图完成等事件)
```

后端还通过一个 Kafka topic 向 DeepStream 发送命令：

```
Backend ────────────────────► Kafka topic: deepstream-commands ──► DeepStream CommandConsumer
                              (录制开始/停止、截图、滚动录制控制、预览切换)
```

### 契约约定

- **camera_id 一致性**：后端添加流时传入的 `camera_id` 与 Kafka 消息中的 `sensorId` 对应
- **topic 名称**：`deepstream-detections`（检测+分析）、`deepstream-events`（录制/截图事件）、`deepstream-commands`（后端→DeepStream 命令），通过环境变量配置
- **消息编码**：JSON UTF-8
- **健康检查**：后端定期 GET `/api/v1/health/get-dsready-state` 检测管道状态
- **分析结果**：`nvdsanalytics` 结果附加在帧级 `analytics` 字段（越线/拥挤）和目标级 `objects[].analytics` 字段（ROI/方向）
- **分析配置**：`analytics_config.txt` 由后端 `PipelineDeployer` 生成，通过共享卷传递给 DeepStream；变更需重部署
- **录像文件**：滚动录制存储在 `/app/recordings/rolling/`，事件/手动录制 `sr-done` 后自动移入 `/app/recordings/locked/`，命名格式 `{type}_{sensor_id}_{timestamp}.mp4`
- **截图文件**：截图存储在共享卷 `/app/screenshots/`，后端提供下载 API（纯 GStreamer jpegenc 编码，无 cv2/torch 依赖）
- **命令通道**：后端通过 `deepstream-commands` topic 发送录制/截图/预览切换命令，DeepStream 通过 `deepstream-events` topic 回传完成事件
- **录制完成通知**：SmartRecordConfig 原生 Kafka 通知（`libnvds_kafka_proto.so`）自动发送到 `deepstream-events`，无需额外 Python Producer
- **磁盘管理**：DeepStream 端 `DiskGuard` 守护线程自主管理（行车记录仪模型：`rolling/` 循环覆盖，`locked/` 超龄清理），后端不参与文件清理，仅消费 Kafka 事件做元数据记录
- **空帧过滤**：`EmptyFrameFilter` probe 丢弃无检测对象且无分析事件的帧，减少 Kafka 消息量 50-80%。后端 Dashboard 帧率统计需使用 PerfMonitor 数据而非 Kafka 消息计数
- **实时预览**：DeepStream 输出带标注 RTSP 流（`:8554/preview`），MediaMTX 转为 WebRTC；后端 API 提供预览 URL，前端通过 WHEP 协议播放。前端不直连 DeepStream（连的是 MediaMTX）
- **预览切换**：tiler 的 `show-source` 属性支持运行时切换单路全分辨率/多画面总览，通过 `deepstream-commands` 的 `switch_preview` 命令控制

---

## 13. 扩展性考虑

### 多 GPU

通过 `CUDA_VISIBLE_DEVICES` 和多个容器实例实现水平扩展：

```yaml
deepstream-gpu0:
  environment:
    - NVIDIA_VISIBLE_DEVICES=0
deepstream-gpu1:
  environment:
    - NVIDIA_VISIBLE_DEVICES=1
```

后端需要知道哪个摄像头分配到哪个 DeepStream 实例（调度逻辑在后端）。

### 推理能力扩展（后续版本）

初版管道：**PGIE（单帧检测）+ nvtracker（跟踪）+ nvdsanalytics（分析）**。

后续可按需增加，均**不影响已有管道代码**（仅需新增 tee 分支或在 tracker 后串入新节点）：

| 扩展方向 | 实现方式 | 改动量 |
|---------|---------|--------|
| **SGIE 二级分类**（车型、人体属性等） | tracker 与 analytics 之间插入 `nvinfer(process-mode=2)` | 管道代码 +10 行，新增 SGIE config |
| **时序动作识别**（SlowFast / X3D） | 新增 tee 分支 → `appsink` + `BufferRetriever` + 独立推理线程 | 新增 2 个文件，管道代码 +10 行 |
| **人物级动作识别** | 在上述基础上加 person crop + per-tracker-id clip buffer | 同上，推理线程逻辑更复杂 |
| 多 PGIE 分支 | tee 在 PGIE 前分流 | 管道拓扑变化较大 |
| `nvdsanalytics` 运行时配置更新 | 探索 property 动态更新 | 避免重启管道 |
| Triton Inference Server | 替换 `nvinfer` 为 `nvinferserver` | 统一管理所有模型 |

### max-batch-size 规划

| GPU | 显存 | 推荐 max-batch-size | 推理精度 |
|-----|------|---------------------|---------|
| RTX 3060 | 12GB | 8-12 | FP16 |
| RTX 3090 | 24GB | 16-24 | FP16 |
| A100 | 40/80GB | 32-64 | FP16 |
| Jetson Orin | 8-64GB | 4-16 | FP16 |

---

## 14. API 测试脚本与 video2rtsp 工具

### 14.1 目标与范围

为 DeepStream 提供一组可独立运行的黑盒测试脚本，覆盖：

- 内置 REST API（`:9000`）
- 命令通道 API（Kafka topic: `deepstream-commands`）
- 全量测试入口脚本（一次性执行全部测试并汇总结果）

测试脚本统一使用 `requests + argparse`，不强依赖 pytest。

### 14.2 测试脚本目录与命名约定

测试脚本放在 `deepstream/test/`，文件命名统一为 `test_{api名称}.py`。

建议文件清单：

- `test_health_get_dsready_state.py`
- `test_stream_add.py`
- `test_stream_remove.py`
- `test_stream_get_stream_info.py`
- `test_command_start_rolling.py`
- `test_command_stop_rolling.py`
- `test_command_start_recording_event.py`
- `test_command_start_recording_manual.py`
- `test_command_stop_recording.py`
- `test_command_screenshot.py`
- `test_command_switch_preview.py`
- `test_all.py`

### 14.3 REST API 测试范围

覆盖以下 4 个端点：

- `GET /api/v1/health/get-dsready-state`
- `POST /api/v1/stream/add`
- `POST /api/v1/stream/remove`
- `GET /api/v1/stream/get-stream-info`

最小断言要求：

- `health`：服务可达且返回 ready 状态字段
- `stream/add`：添加后 `get-stream-info` 可见该流
- `stream/remove`：移除后 `get-stream-info` 不再包含该流
- `get-stream-info`：返回结构稳定、可解析

### 14.4 命令通道 API 测试范围

命令消息主题：`deepstream-commands`（JSON UTF-8）。

覆盖动作：

- `start_rolling`
- `stop_rolling`
- `start_recording`（`type=event`）
- `start_recording`（`type=manual`）
- `stop_recording`
- `screenshot`
- `switch_preview`

命令消息格式参考：

```json
{"action": "start_recording", "source_id": "cam_001", "duration": 20, "type": "event"}
{"action": "start_recording", "source_id": "cam_001", "duration": 0, "type": "manual"}
{"action": "stop_recording", "source_id": "cam_001"}
{"action": "screenshot", "source_id": "cam_001", "filename": "cam001_snap.jpg"}
{"action": "start_rolling", "source_id": "cam_001"}
{"action": "stop_rolling", "source_id": "cam_001"}
{"action": "switch_preview", "source_id": -1}
```

说明：

- 除 `switch_preview` 外，`source_id` 使用 `camera_id/sensor_id` 字符串（如 `cam_001`）
- `switch_preview` 使用整数 `source_id`（`-1` 表示多画面）
- 测试前需确保目标流已成功 `add`，并建立 `sensor_id -> source_id` 映射

### 14.5 test_all.py 运行策略

`test_all.py` 负责全量执行所有 `test_*.py` 脚本并汇总结果：

- 默认策略：**continue_all**（单个失败不阻断后续脚本）
- 编排策略：先一次性准备持久测试源（固定 `camera_id`），命令测试阶段复用该源，避免每个脚本重复 add/remove 导致动态源抖动
- 执行顺序：`stream/remove` 放在最后，防止在命令测试前移除唯一输入源
- 参数下发：仅命令测试脚本接收 Kafka 参数（`--kafka-broker`、`--command-topic`），REST 脚本只接收 REST 相关参数
- 最终输出：通过数、失败数、失败脚本列表、总耗时
- 退出码：有失败则返回非 0，全部通过返回 0

### 14.6 参数约定（argparse）

建议所有测试脚本支持统一参数：

- `--base-url`（默认 `http://127.0.0.1:9000`）
- `--kafka-broker`（默认 `127.0.0.1:9092`）
- `--command-topic`（默认 `deepstream-commands`）
- `--camera-id`（默认自动生成，避免冲突）
- `--camera-url`（测试流 URL）
- `--timeout`（HTTP/Kafka 等待超时）
- `--verbose`（输出调试日志）

命令测试脚本额外支持：

- `--no-prepare`：跳过该脚本内的 `prepare_camera`，用于在 `test_all.py` 中复用已准备好的持久源

补充说明：

- 命令通道中，`start/stop rolling`、`start/stop recording`、`screenshot` 的 `source_id` 字段传 `sensor_id(camera_id)` 字符串；
- `switch_preview` 仍使用整数 `source_id`（如 `-1` 多画面或指定单路）。

### 14.6.1 pyservicemaker 截图兼容说明

不同 pyservicemaker 版本在 `Buffer` 接口上存在差异（如 `source_id`、`get_data` 可能缺失）。为保证命令链路可测试：

- 若版本支持原始 JPEG 提取，则按实时帧写入截图；
- 若版本不支持原始提取接口，则写入兼容 fallback JPEG，并保持 `screenshot_done` 事件流程不变；
- 该兼容策略用于保证测试稳定性与接口契约一致性，不影响后续升级到完整帧导出实现。

### 14.7 测试数据放置规则

测试数据统一放在 `deepstream/example_data/`：

- 现有视频：
  - `deepstream/example_data/video1_bf0.mp4`
  - `deepstream/example_data/video2_bf0.mp4`
- 新增测试数据目录：
  - `deepstream/example_data/test_data/`

`test_data/` 用于存放：

- 动态生成的测试 payload 样例
- 截图文件名模板
- 运行过程中的临时测试元数据（如 camera_id）

### 14.8 video2rtsp.py（测试流发布工具）

脚本路径：`deepstream/script/video2rtsp.py`

用途：将 `example_data` 里的两个本地视频发布成 RTSP 测试流，便于 REST `stream/add` 与命令通道测试复用。

默认映射：

- `/video1` -> `video1_bf0.mp4`
- `/video2` -> `video2_bf0.mp4`

默认地址：

- `rtsp://127.0.0.1:8554/video1`
- `rtsp://127.0.0.1:8554/video2`

运行方式：

```bash
python3 deepstream/script/video2rtsp.py
```

可选参数：

```bash
python3 deepstream/script/video2rtsp.py \
  --port 8555 \
  --video1 /path/to/video_a.mp4 \
  --video2 /path/to/video_b.mp4
```

实现说明：

- 基于 `GstRtspServer` 创建多个 mount point
- 使用 `filesrc -> qtdemux -> h264parse -> rtph264pay` 发布流
- 适用于 MP4(H.264) 测试视频
