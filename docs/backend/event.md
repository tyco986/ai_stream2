# Events（后端）

对应前端契约：[event_api.md](../frontend/event_api.md)。  
实现包：`servers/django/pages/events/`。前缀：`/ai_stream2/backend/events`。

---

## 代理的 API

无 HTTP 上游代理。写入路径为 **Kafka 消费**（本页 `EventIngestService`），非本前缀 REST。

---

## 暴露的 API

| 方法 | 路径 | 功能 |
|------|------|------|
| `GET` | `/options/events` | Event 下拉（按 pipeline 分组） |
| `GET` | `/calendar` | 月内有事件的日期 |
| `GET` | `/` | 分页列表（筛选） |
| `GET` | `/{event_id}` | 详情 + prev/next（同筛选） |
| `POST` | `/{event_id}/ack` | 单条 ack/unack |
| `POST` | `/batch/ack` | 批量 ack/unack |
| `POST` | `/export` | 筛选导出 zip（CSV+图） |
| `POST` | `/collect` | 筛选加密 zip（+ labelme/yolo） |

**无**手工创建事件 API。字段 / 错误码以 [event_api.md](../frontend/event_api.md) 为准。

权限：`events.view_event`（读 / Export）；`events.change_event`（Ack / Collect）。

固定段：`options`、`batch`、`export`、`collect`、`calendar` 先于 `/{event_id}`。

### Collect / Export 锁定决策

| 项 | 约定 |
|----|------|
| 筛选传参 | **query** 与 `GET /` 对齐 |
| 无命中 | **400** |
| Collect 口令 | 请求体 JSON **`passphrase` 必填** |
| Collect 加密 | zip + **AES**（口令派生）；**不**经 TOTP |
| Export | 明文 zip；无需 body |

---

## 技术栈

| 层 | 选用 |
|----|------|
| 框架 | Django + DRF |
| ORM | `Event` 表 |
| 消息 | Kafka 消费者（容器内；可同 django 进程或旁路 worker） |
| 打包 | `zipfile`；Collect 用支持 AES 的库（如 `pyzipper`） |
| 媒体 | 卷 `/media/events/...`；`raw_url` / `visualization_url` |
| 鉴权 | session + `has_perm` |

---

## 造轮子 vs 复用

| 能力 | 做法 |
|------|------|
| 列表筛选 / ack / prev-next | **造**（`EventQueryService`） |
| Kafka ingest | **造**（`EventIngestService`） |
| Export / Collect 打包 | **造**（`EventExportService`） |
| Event 显示名 | ingest 时写入 `event_label` 快照；options 仅在本页聚合 distinct `(event_code, event_label)` 或维护轻量词典表（**不**引用 pipelines `event_code`） |
| stream/pipeline 下拉 | 前端调 stream_api / pipeline_api |
| Site Config | **不**注册（运行时数据） |

---

## 目录结构

```
servers/django/pages/events/
  apps.py
  urls.py
  models.py               # Event
  serializers.py
  views.py
  services.py             # EventQueryService, EventAckService, EventExportService, EventIngestService
  consumers.py            # Kafka loop 入口（或 management command）
  permissions.py
  migrations/
```

禁止 import `pages.pipelines` / `pages.streams`。

---

## 类与职责

| 类 | 职责 |
|----|------|
| `Event` | 落库事件行 |
| `EventIngestService` | 解析 Kafka 消息 → 落盘图 → INSERT（快照名与 label） |
| `EventQueryService` | list / detail+neighbors / calendar / options |
| `EventAckService` | 单条/批量 ack\|unack（幂等） |
| `EventExportService` | export zip；collect AES zip（labelme/yolo） |
| Views | 薄封装 |

响应字段 `event` = 库内 `event_label`（显示名）。筛选项 `event` query = `event_code`。

---

## 数据库

### `events_event`

| 列 | 类型 | 约束 | 默认 | 说明 |
|----|------|------|------|------|
| `id` | UUID | PK | uuid4 | |
| `occurred_at` | timestamptz | 非空，索引 | — | |
| `stream_id` | UUID | 非空，索引 | — | 软引用 |
| `stream_name` | varchar | 非空 | — | **ingest 快照** |
| `pipeline_id` | UUID | 非空，索引 | — | 软引用 |
| `pipeline_name` | varchar | 非空 | — | **ingest 快照** |
| `event_code` | varchar | 非空，索引 | — | 内部码 |
| `event_label` | varchar | 非空 | — | 显示名快照 |
| `status` | varchar | 非空 | `new` | `new`\|`acked` |
| `raw_path` | varchar | 可空 | `NULL` | 卷内路径 |
| `visualization_path` | varchar | 可空 | `NULL` | |
| `payload` | JSON | 可空 | `NULL` | 可选原始载荷 |

默认排序：`occurred_at DESC, id DESC`。  
`from`/`to`：站点时区日历日闭区间。

索引建议：`(occurred_at, id)`；`(pipeline_id, event_code)`；`status`。

---

## 各请求写库明细

| 请求 | 写入 |
|------|------|
| Kafka ingest | INSERT Event + 写媒体文件 |
| `POST .../ack`、`batch/ack` | UPDATE `status`（幂等；不存在 id 跳过） |
| GET / export / collect / options / calendar | 不改状态；export/collect 只读打包 |
| Site Config | 无 |

---

## 调用顺序

```
ingest:
  Kafka message → 解析 stream/pipeline/code/ts/images
  → 快照 name/label → 落盘 → INSERT status=new

GET /:
  解析筛选 → 分页 → items（event=event_label）

GET /{id}:
  同行筛选序算 prev_id/next_id → EventDetail（URL 由 path 拼）

POST export:
  同筛选全量（合理上限可配）→ 无行 400 → zip 流

POST collect:
  body.passphrase 必填 → 同筛选 → 生成 labelme/yolo → AES zip
```

options：仅在本页按 `pipeline_id` 聚合 distinct `(event_code, event_label)`，或维护轻量词典表（仍属 events 包；**不**读 pipelines 词典）。

---

## Ingest 信封（本阶段真相源）

Kafka / `manage.py ingest_event` 消息体为 **JSON object**（非当前 DeepStream `DetMessager` 的 bbox 数组）：

```json
{
  "occurred_at": "2026-07-22T19:40:12Z",
  "stream_id": "<uuid>",
  "stream_name": "cam-01",
  "pipeline_id": "<uuid>",
  "pipeline_name": "presence-a",
  "event_code": "1",
  "event_label": "Intrusion",
  "raw_image_b64": "<optional>",
  "visualization_image_b64": "<optional>",
  "objects": [[x1, y1, x2, y2, conf, class_id, "label"], ...]
}
```

| 字段 | 说明 |
|------|------|
| `occurred_at` | ISO8601；缺省则入库用消费时刻 |
| `stream_*` / `pipeline_*` | 必填；名称写入快照列 |
| `event_code` / `event_label` | 必填；筛选用 code，API 展示用 label |
| `*_image_b64` | 可选；解码后写入 `EVENTS_MEDIA_DIR/{id}/` |
| `objects` | 可选；原样进 `payload.objects`，Collect 生成 LabelMe/YOLO |

**现状**：DeepStream `DetMessager` 仍只发 detection 数组，**尚未**发本信封；`consume_events` 对非信封消息跳过并打日志。闭环待后续改 messager / `event_coder`。冒烟用 `manage.py ingest_event --file envelope.json`。

---

## 依赖边界

| 依赖 | 说明 |
|------|------|
| Kafka | topic 默认 `deepstream-detections`；信封见上；DS 未对齐前以命令注入为准 |
| 媒体卷 | raw/viz；URL 前缀 `/ai_stream2/media/events/` |
| streams / pipelines | 仅 UUID + ingest 快照；下拉前端直调对方 API |
| recordings | Playback 深链仅前端 |
| Site Config | **不**注册 |
