# DeepStream 内置 REST API

Source: [DeepStream 9.0 REST API Server](https://docs.nvidia.com/metropolis/deepstream/9.0/text/DS_RestServer.html)

REST 服务内建于 `nvmultiurisrcbin`（底层 `nvds_rest_server` + Civetweb），用于运行时动态增删视频源、调整 pipeline 组件参数、查询健康与指标。**不要**为流管理单独实现 Flask/FastAPI 服务。

## 启用方式

在 pipeline 的 `nvmultiurisrcbin` 元素上配置：

| 属性 | 说明 | 典型值 |
|------|------|--------|
| `ip-address` | 监听地址 | `0.0.0.0` |
| `port` | REST 端口，`0` 表示禁用 | `9000` |
| `live-source` | 动态源必需 | `1` |
| `drop-pipeline-eos` | 移除所有源后 pipeline 不退出 | `1` |
| `async-handling` | 异步状态变更 | `1` |

默认 Base URL：`http://<host>:9000/api/v1`

本项目后端通过 `DeepStreamClient`（`backend/services/deepstream_client.py`）代理其中 4 个流管理与健康检查端点。

---

## 端点总览

| 端点 | 方法 | 组件 | 说明 |
|------|------|------|------|
| `/stream/add` | POST | nvmultiurisrcbin | 添加视频源 |
| `/stream/remove` | POST | nvmultiurisrcbin | 移除视频源 |
| `/stream/get-stream-info` | GET | — | 查询当前流列表 |
| `/health/get-dsready-state` | GET | — | DeepStream pipeline 就绪状态 |
| `/metrics` | GET | — | FPS、延迟、GPU/CPU 指标 |
| `/live` | GET | — | 存活探测（也可用 `/api/v1/live`） |
| `/ready` | GET | — | 就绪探测（也可用 `/api/v1/ready`） |
| `/startup` | GET | — | 启动状态（也可用 `/api/v1/startup`） |
| `/app/quit` | POST | Application | 退出应用实例 |
| `/roi/update` | POST | nvdspreprocess | 更新 ROI |
| `/dec/drop-frame-interval` | POST | nvv4l2decoder | 设置丢帧间隔 |
| `/dec/skip-frames` | POST | nvv4l2decoder | 设置跳帧模式 |
| `/infer/set-interval` | POST | nvinfer | 设置推理间隔 |
| `/inferserver/set-interval` | POST | nvinferserver | 设置推理间隔 |
| `/nvtracker/config-path` | POST | nvdstracker | 切换 tracker 配置文件 |
| `/enc/force-idr` | POST | nvv4l2encoder | 强制 IDR 帧 |
| `/enc/force-intra` | POST | nvv4l2encoder | 强制 Intra 帧 |
| `/enc/bitrate` | POST | nvv4l2encoder | 设置码率 |
| `/enc/iframe-interval` | POST | nvv4l2encoder | 设置 I 帧间隔 |
| `/mux/batched-push-timeout` | POST | nvstreammux | 设置 batch 推送超时 |
| `/conv/srccrop` | POST | nvvideoconvert | 源裁剪 |
| `/conv/destcrop` | POST | nvvideoconvert | 目标裁剪 |
| `/conv/flip-method` | POST | nvvideoconvert | 翻转/旋转 |
| `/conv/interpolation-method` | POST | nvvideoconvert | 插值方法 |
| `/osd/process-mode` | POST | nvdsosd | CPU/GPU 绘制模式 |
| `/analytics/reload-config` | POST | nvdsanalytics | 热加载 analytics 配置 |

> 注：encoder 相关端点需在 `deepstream-server-app` 的 `dsserver_config.yml` 中 `encoder.enable: 1` 才生效。部分端点的 `stream_id` 目前对所有活跃流生效，而非单路隔离。

---

## 流管理

### POST `/stream/add`

添加视频源。`value.change` 必须包含 `"add"` 子串。

**请求体（标准格式）：**

```json
{
  "key": "sensor",
  "value": {
    "camera_id": "unique_sensor_id",
    "camera_name": "front_door",
    "camera_url": "rtsp://192.168.1.100/stream",
    "change": "camera_add",
    "metadata": {
      "resolution": "1920x1080",
      "codec": "h264",
      "framerate": 30
    }
  },
  "headers": {
    "source": "vst",
    "created_at": "2021-06-01T14:34:13.417Z"
  }
}
```

**必填字段：** `value.camera_id`、`value.camera_url`、`value.change`

**curl：**

```bash
curl -X POST 'http://localhost:9000/api/v1/stream/add' \
  -H 'Content-Type: application/json' \
  -d '{
    "key": "sensor",
    "value": {
      "camera_id": "cam_001",
      "camera_name": "Front Door",
      "camera_url": "rtsp://192.168.1.100/stream",
      "change": "camera_add"
    }
  }'
```

**备用格式（`event` 替代 `value`，`change: camera_streaming`）：** 仅 `nvmultiurisrcbin` 模式支持。

---

### POST `/stream/remove`

移除视频源。`value.change` 必须包含 `"remove"` 子串。`camera_id` 须与添加时一致。

**请求体：**

```json
{
  "key": "sensor",
  "value": {
    "camera_id": "cam_001",
    "camera_name": "Front Door",
    "camera_url": "rtsp://192.168.1.100/stream",
    "change": "camera_remove"
  }
}
```

**curl：**

```bash
curl -X POST 'http://localhost:9000/api/v1/stream/remove' \
  -H 'Content-Type: application/json' \
  -d '{
    "key": "sensor",
    "value": {
      "camera_id": "cam_001",
      "camera_url": "rtsp://192.168.1.100/stream",
      "change": "camera_remove"
    }
  }'
```

---

### GET `/stream/get-stream-info`

查询当前活跃流。

**curl：**

```bash
curl -X GET 'http://localhost:9000/api/v1/stream/get-stream-info'
```

**响应示例：**

```json
{
  "reason": "GET_LIVE_STREAM_INFO_SUCCESS",
  "status": "HTTP/1.1 200 OK",
  "stream-info": {
    "stream-count": 1,
    "stream-info": [
      {
        "camera_id": "UniqueSensorId1",
        "camera_name": "UniqueSensorName1"
      }
    ]
  }
}
```

---

## 健康与指标（GET）

### GET `/health/get-dsready-state`

Pipeline 是否就绪。

```bash
curl -X GET 'http://localhost:9000/api/v1/health/get-dsready-state'
```

```json
{
  "health-info": { "ds-ready": "YES" },
  "reason": "GET_DS_READINESS_INFO_SUCCESS",
  "status": "HTTP/1.1 200 OK"
}
```

### GET `/metrics`

FPS、延迟、GPU/CPU/RAM 用量。支持 OpenTelemetry 运行时配置（请求头）：

| Header | 说明 |
|--------|------|
| `X-REFRESH-PERIOD` | 推送间隔（ms）；`-1` 禁用 exporter |
| `X-OTLP-URL` | OTLP collector 地址，如 `http://192.168.1.100:4318` |

```bash
curl -X GET 'http://localhost:9000/api/v1/metrics'
```

```json
{
  "metrics-info": {
    "stream-count": 2,
    "stream-stats": [
      {
        "fps": 29.8,
        "frame_number": 332,
        "latency_ms": 301.88,
        "sensor_id": "UniqueSensorId1",
        "sensor_name": "UniqueSensorName1",
        "source_id": 0
      }
    ],
    "system-stats": {
      "GPU_gb": 1.28,
      "RAM_gb": 10.21,
      "cpu_util": 3.13,
      "gpu_util": 0.0
    }
  },
  "reason": "GET_METRICS_INFO_SUCCESS",
  "status": "HTTP/1.1 200 OK"
}
```

### GET `/live` 或 `/api/v1/live`

存活探测。

```json
{ "live-info": { "ds-liveness": "YES" }, "reason": "GET_LIVE_INFO_SUCCESS", "status": "HTTP/1.1 200 OK" }
```

### GET `/ready` 或 `/api/v1/ready`

就绪探测。

```json
{ "ready-info": { "ds-ready": "YES" }, "reason": "GET_READY_INFO_SUCCESS", "status": "HTTP/1.1 200 OK" }
```

### GET `/startup` 或 `/api/v1/startup`

启动状态。

```json
{ "startup-info": { "ds-startup": "YES" }, "reason": "GET_STARTUP_INFO_SUCCESS", "status": "HTTP/1.1 200 OK" }
```

---

## 应用控制

### POST `/app/quit`

退出 DeepStream 应用实例（等效于通过 REST 请求 graceful quit）。

```bash
curl -X POST 'http://localhost:9000/api/v1/app/quit' \
  -H 'Content-Type: application/json' \
  -d '{"stream": {"app_quit": 1}}'
```

> 这是**停止整个应用**的 REST 方式，与 OS 信号（SIGTERM）不同路径，但目的一致。需 pipeline 以 server 模式运行且注册了对应 callback。

---

## 运行时组件配置（POST）

所有 POST 配置端点均使用 JSON body，响应含 `status` 与 `reason` 字段。DeepStream 7.0+ 遵循 OpenAPI 错误格式。

### nvdspreprocess — `/roi/update`

```json
{
  "stream": {
    "stream_id": "0",
    "roi_count": 2,
    "roi": [
      { "roi_id": "0", "left": 100, "top": 300, "width": 400, "height": 400 },
      { "roi_id": "1", "left": 550, "top": 300, "width": 500, "height": 500 }
    ]
  }
}
```

### nvv4l2decoder

**`/dec/drop-frame-interval`** — `drop_frame_interval` 范围 0–30

```json
{ "stream": { "stream_id": "0", "drop_frame_interval": 2 } }
```

**`/dec/skip-frames`** — `skip_frames`: `0` 全解码 / `1` 非参考帧 / `2` 仅关键帧

```json
{ "stream": { "stream_id": "0", "skip_frames": 2 } }
```

### nvinfer — `/infer/set-interval`

```json
{ "stream": { "stream_id": "0", "interval": 2 } }
```

跳过连续 batch 的推理次数。需禁用 pgie config 中的 `input-tensor-meta` 才能看到效果。

### nvinferserver — `/inferserver/set-interval`

```json
{ "stream": { "stream_id": "0", "interval": 2 } }
```

### nvdstracker — `/nvtracker/config-path`

```json
{ "stream": { "stream_id": "0", "config_path": "trackerReset.yaml" } }
```

### nvv4l2encoder

| 端点 | 字段 | 说明 |
|------|------|------|
| `/enc/force-idr` | `force_idr: 1` | 强制 IDR |
| `/enc/force-intra` | `force_intra: 1` | 强制 Intra |
| `/enc/bitrate` | `bitrate: 2000000` | 码率（bps） |
| `/enc/iframe-interval` | `iframeinterval: 50` | I 帧间隔 |

### nvstreammux — `/mux/batched-push-timeout`

```json
{ "stream": { "batched_push_timeout": 100000 } }
```

单位：微秒。适用于旧版 nvstreammux。

### nvvideoconvert

| 端点 | 字段 | 格式 |
|------|------|------|
| `/conv/srccrop` | `src_crop` | `"left:top:width:height"` |
| `/conv/destcrop` | `dest_crop` | `"left:top:width:height"` |
| `/conv/flip-method` | `flip_method` | 0–7（none / 90° / 180° / flip 等） |
| `/conv/interpolation-method` | `interpolation_method` | 0–6 |

### nvdsosd — `/osd/process-mode`

```json
{ "stream": { "stream_id": "0", "process_mode": 0 } }
```

`0` = CPU，`1` = GPU。

### nvdsanalytics — `/analytics/reload-config`

```json
{
  "stream": {
    "config_file_path": "/path/to/config_nvdsanalytics.txt"
  }
}
```

---

## 与本项目的对应关系

| DeepStream REST | 本项目 |
|-----------------|--------|
| `POST /stream/add` | `DeepStreamClient.add_stream` → 摄像头 `start-stream` |
| `POST /stream/remove` | `DeepStreamClient.remove_stream` → 摄像头 `stop-stream` |
| `GET /stream/get-stream-info` | `DeepStreamClient.get_streams` → `/api/v1/deepstream/streams/` |
| `GET /health/get-dsready-state` | `DeepStreamClient.health_check` → `/api/v1/deepstream/health/` |
| 其余端点 | 暂未封装，可直接 curl 调用 |

---

## 注意事项

1. **动态源 + sink 必须 `async=0`**，否则元素卡在 PAUSED，REST 加流成功但无画面。
2. **REST 不能替代 OS 信号**：除 `/app/quit` 外，多数端点是调参而非停 pipeline；容器级停止仍用 `docker stop`（SIGTERM）。
3. **HTTPS 未内置**，需自行扩展 `nvds_rest_server` 源码。
4. **官方参考应用**：`/opt/nvidia/deepstream/deepstream/sources/apps/sample_apps/deepstream-server`
5. **库源码**：`/opt/nvidia/deepstream/deepstream/sources/libs/nvds_rest_server/`

---

## 参考链接

- [DeepStream REST API Server (DS 9.0)](https://docs.nvidia.com/metropolis/deepstream/9.0/text/DS_RestServer.html)
- [Gst-nvmultiurisrcbin](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvmultiurisrcbin.html)
- [nvds_rest_server.h API](https://docs.nvidia.com/metropolis/deepstream/9.0/sdk-api/9_80_2sources_2libs_2nvds__rest__server_2nvds__rest__server_8h.html)
